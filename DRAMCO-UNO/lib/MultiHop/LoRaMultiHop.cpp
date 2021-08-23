#include <Adafruit_SleepyDog.h>
#include "LoRaMultiHop.h"

#define GATEWAY_UID                 0x0000
#define BROADCAST_UID               0xFFFF

#define NODE_UID_SIZE               sizeof(Node_UID_t)
#define MESG_UID_SIZE               sizeof(Msg_UID_t)
#define MESG_TYPE_SIZE              sizeof(Msg_Type_t)
#define MESG_HOPS_SIZE              1
#define MESG_PAYLOAD_LEN_SIZE       1

#define HEADER_NODE_UID_OFFSET      0
#define HEADER_MESG_UID_OFFSET      (HEADER_NODE_UID_OFFSET + NODE_UID_SIZE)
#define HEADER_HOPS_OFFSET          (HEADER_MESG_UID_OFFSET + MESG_UID_SIZE)
#define HEADER_TYPE_OFFSET          (HEADER_HOPS_OFFSET + MESG_HOPS_SIZE)
#define HEADER_NEXT_UID_OFFSET      (HEADER_TYPE_OFFSET + MESG_TYPE_SIZE)
#define HEADER_PREVIOUS_UID_OFFSET  (HEADER_TYPE_OFFSET + MESG_TYPE_SIZE)
//#define HEADER_PREVIOUS_UID_OFFSET  (HEADER_NEXT_UID_OFFSET + MESG_TYPE_SIZE) // saves 2 bytes
#define HEADER_PAYLOAD_LEN_OFFSET   (HEADER_PREVIOUS_UID_OFFSET + NODE_UID_SIZE)
#define HEADER_PAYLOAD_OFFSET       (HEADER_PAYLOAD_LEN_OFFSET + MESG_PAYLOAD_LEN_SIZE)
#define HEADER_SIZE                 HEADER_PAYLOAD_OFFSET

// Singleton instance of the radio driver
RH_RF95 rf95(PIN_MODEM_SS, PIN_MODEM_INT);

MsgReceivedCb mrc = NULL;

LoRaMultiHop::LoRaMultiHop(NodeType_t nodeType){
    this->type = nodeType;
    this->shortestRoute.hopsToGateway=0;
}

bool LoRaMultiHop::begin(){
    Serial.print(F("Setting uid: "));
    unsigned long av = analogRead(A1);
    randomSeed(av*av);

    if(this->type == GATEWAY){
        this->uid = GATEWAY_UID;
    }
    else{
        this->uid = (uint16_t) random();
    }
    Serial.println(uid, HEX);

    Serial.print(F("Initializing flood buffer... "));
    this->floodBuffer.init(8);
    // prefill buffer (otherwise "CircBuffer::find()"" fails miserably and I've yet to fix it)
    MsgInfo_t dummy;
    for(uint8_t i=0; i<8; i++){
        this->floodBuffer.put(dummy);
    }
    Serial.println(F("done."));

    Serial.print(F("Enabling 3V3 regulator... "));
    // Dramco uno - enable 3v3 voltage regulator
    pinMode(PIN_ENABLE_3V3, OUTPUT);
    digitalWrite(PIN_ENABLE_3V3, HIGH);
    Serial.println(F("done."));

    Serial.print(F("Initializing modem... "));
    if(!rf95.init()){
        Serial.println(F("failed."));
        return false;
    }

    this->reconfigModem();

    return true;
}

void LoRaMultiHop::loop(void){
    Serial.flush();
    rf95.sleep();

#ifndef TOGGLE_TIME
    Watchdog.sleep(100);
#else
    delay(90);
#endif


    rf95.setModeCad(); // listen for channel activity
#ifndef TOGGLE_TIME
    Watchdog.sleep(10); // cad done will wake us again
#else
    while(!rf95.cadDone());
#endif

    // if channel activity has been detected during the previous CAD - enable RX and receive message
    if(rf95.cadDetected()){
        //LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
        rf95.setModeRx();
        rf95.waitAvailableTimeout(1000);

        uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
        if(rf95.recv(this->rxBuf, &len)){
            Serial.println(F("Message received."));
            if(this->handleMessage(this->rxBuf, len)){
                if(mrc != NULL){
                    mrc(this->rxBuf+HEADER_PAYLOAD_OFFSET, this->rxBuf[HEADER_PAYLOAD_LEN_OFFSET]);
                }
            }
        }
        else{
            Serial.println(F("Message receive failed"));
        }
    }
    else{
        rf95.sleep();
    }

    // handle any pending tx
    if(txPending){
        if(txTime > millis()){
            this->txMessage(txLen);
        }
    }
}

void LoRaMultiHop::setMsgReceivedCb(MsgReceivedCb cb){
    mrc = cb;
}

bool LoRaMultiHop::sendMessage(String str){
    uint8_t pLen = str.length();
    if(pLen > RH_RF95_MAX_MESSAGE_LEN - HEADER_SIZE){
        Serial.println("Payload is too long.");
        return false;
    }

    // copy info to tx buffer and update flood buffer
    this->initMsgInfo(pLen);
    
    // copy payload to tx Buffer
    uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
    for(uint8_t i=0; i<pLen; i++){
        *pPtr++ = (uint8_t)str.charAt(i);
    }

    // transmit
    txMessage(pLen + HEADER_SIZE);

    return true;
}

bool LoRaMultiHop::sendMessage(uint8_t * buf, uint8_t len){
    if(len > RH_RF95_MAX_MESSAGE_LEN - HEADER_SIZE){
        Serial.println("Payload is too long.");
        return false;
    }

    // copy info to tx buffer and update flood buffer
    this->initMsgInfo(len);
    
    // copy payload to tx Buffer
    uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
    memcpy(pPtr, buf, len);

    // transmit
    txMessage(len + HEADER_SIZE);

    return true;
}

void LoRaMultiHop::initMsgInfo(MsgInfo_t info, uint8_t pLen){
    // add message info to flood buffer
    floodBuffer.put(info);

    // fill tx buffer with info
    this->txBuf[HEADER_NODE_UID_OFFSET] = (uint8_t)(info.nodeUid >> 8);
    this->txBuf[HEADER_NODE_UID_OFFSET+1] = (uint8_t)(info.nodeUid & 0x00FF);
    this->txBuf[HEADER_MESG_UID_OFFSET] = (uint8_t)(info.msgUid >> 8);
    this->txBuf[HEADER_MESG_UID_OFFSET+1] = (uint8_t)(info.msgUid & 0x00FF);

    // set hop count to 0
    this->txBuf[HEADER_HOPS_OFFSET] = 0;
    this->txBuf[HEADER_PAYLOAD_LEN_OFFSET] = pLen;
}

void LoRaMultiHop::initMsgInfo(uint8_t pLen){
    MsgInfo_t mInfo;
    mInfo.nodeUid = this->uid;
    mInfo.msgUid = (uint16_t) random();

    // add message info to flood buffer
    floodBuffer.put(mInfo);

    // fill tx buffer with info
    this->txBuf[HEADER_NODE_UID_OFFSET] = (uint8_t)(mInfo.nodeUid >> 8);
    this->txBuf[HEADER_NODE_UID_OFFSET+1] = (uint8_t)(mInfo.nodeUid & 0x00FF);
    this->txBuf[HEADER_MESG_UID_OFFSET] = (uint8_t)(mInfo.msgUid >> 8);
    this->txBuf[HEADER_MESG_UID_OFFSET+1] = (uint8_t)(mInfo.msgUid & 0x00FF);

    // set hop count to 0
    this->txBuf[HEADER_HOPS_OFFSET] = 0;
    this->txBuf[HEADER_PAYLOAD_LEN_OFFSET] = pLen;
}

bool LoRaMultiHop::handleMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }

    switch(buf[HEADER_TYPE_OFFSET]){
        case GATEWAY_BEACON:{
            Node_UID_t receivedFrom = this->getNodeUidFromBuffer(buf, PREVIOUS_NODE);
            uint8_t hops = buf[HEADER_HOPS_OFFSET];
            Msg_UID_t msgId = this->getMsgUidFromBuffer(buf);
            if(this->shortestRoute.lastGatewayBeacon != msgId){
                // new gateway beacon -> route to gateway info needs to be updated
                this->shortestRoute.lastGatewayBeacon = msgId;
                this->shortestRoute.hopsToGateway = hops+1;
                this->shortestRoute.viaNode = receivedFrom;
            }
        } break;
        
        case DATA_BROADCAST:{
            // todo
        } break;

        case DATA_ROUTED:{
            // todo
        } break;

        default:{
            // todo
        } break;
    }

    return true;
}

bool LoRaMultiHop::forwardMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }
    MsgInfo_t mInfo;
    mInfo.nodeUid = getNodeUidFromBuffer(buf);
    mInfo.msgUid = getMsgUidFromBuffer(buf);

    if(this->floodBuffer.find(mInfo) == CB_SUCCESS){
        Serial.println(F("Duplicate. Message not forwarded."));
        return false;
    }
    else{
        Serial.println(F("Message will be forwarded."));
    }

    this->initMsgInfo(mInfo, buf[HEADER_PAYLOAD_LEN_OFFSET]);
    this->txBuf[HEADER_HOPS_OFFSET] = buf[HEADER_HOPS_OFFSET] + 1;

    // copy payload to tx Buffer
    uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
    memcpy(pPtr, buf+HEADER_PAYLOAD_OFFSET, buf[HEADER_PAYLOAD_LEN_OFFSET]);

    // schedule tx
    uint16_t backoff = (uint16_t)(random() % 1000);
    this->txTime = millis() + backoff;
    this->txPending = true;
    this->txLen = len;

    return true;
}

void LoRaMultiHop::txMessage(uint8_t len){
    Serial.print("TX MSG: ");
    for(uint8_t i=0; i<len; i++){
        if(this->txBuf[i] < 16){
            Serial.print("0");
        }
        Serial.print(this->txBuf[i], HEX);
        Serial.print(" ");
    }

    rf95.send(this->txBuf, len);
    rf95.waitPacketSent();
    Serial.println("| OK");
    this->txPending = false;
}

void LoRaMultiHop::reconfigModem(void){
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    rf95.setFrequency(868);

    ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
    rf95.setPreambleLength(390); // 100 ms cycle for wakeup
    //From my understanding of the datasheet, preamble length in symbols = cycletime * BW / 2^SF. So, for 1 second cycle at 500 kHz and SF7, preamble = 1 * 500000 / 128 = 3906 symbols.

    // lowest transmission power possible
    rf95.setTxPower(5, false);
}

Node_UID_t LoRaMultiHop::getNodeUidFromBuffer(uint8_t * buf, NodeID_t which){
    switch (which){
        case NEXT_NODE: {
            return (Node_UID_t)(buf[HEADER_NEXT_UID_OFFSET]<<8 | buf[HEADER_NEXT_UID_OFFSET+1]);
        } break;
        
        case PREVIOUS_NODE: {
            return (Node_UID_t)(buf[HEADER_PREVIOUS_UID_OFFSET]<<8 | buf[HEADER_PREVIOUS_UID_OFFSET+1]);
        } break;
        
        default: {
            return (Node_UID_t)(buf[HEADER_NODE_UID_OFFSET]<<8 | buf[HEADER_NODE_UID_OFFSET+1]);
        } break;
    }
    return 0x0000;
}

Msg_UID_t LoRaMultiHop::getMsgUidFromBuffer(uint8_t * buf){
    return (Msg_UID_t)(buf[HEADER_MESG_UID_OFFSET]<<8 | buf[HEADER_MESG_UID_OFFSET+1]);
}