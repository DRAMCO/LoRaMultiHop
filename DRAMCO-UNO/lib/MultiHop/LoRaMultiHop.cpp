#include "LoRaMultiHop.h"
#include "Dramco-UNO-Sensors.h"

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
}

bool LoRaMultiHop::begin(){
#ifdef DEBUG
    Serial.print(F("Initializing flood buffer... "));
#endif

    this->floodBuffer.init(8);
    // prefill buffer (otherwise "CircBuffer::find()"" fails miserably and I've yet to fix it)
    MsgInfo_t dummy;
    for(uint8_t i=0; i<8; i++){
        this->floodBuffer.put(dummy);
    }
#ifdef DEBUG
    Serial.println(F("done."));
    Serial.print(F("Enabling 3V3 regulator... "));
#endif// Dramco uno - enable 3v3 voltage regulator

    pinMode(PIN_ENABLE_3V3, OUTPUT);
    digitalWrite(PIN_ENABLE_3V3, HIGH);

#ifdef DEBUG
    Serial.println(F("done."));

    Serial.print(F("Initializing modem... "));
#endif
    if(!rf95.init()){
#ifdef DEBUG
        Serial.println(F("failed."));
        #endif
return false;
    }

    this->reconfigModem();
#ifdef DEBUG
    Serial.println(F("done"));
#endif

#ifdef DEBUG
    Serial.print(F("Setting node UID: "));
#endif

    uint32_t r = rf95.random() * DramcoUno.random();
    randomSeed(r);  
    if(this->type == GATEWAY){
        this->uid = GATEWAY_UID;
    }
    else{
        this->uid = (Node_UID_t) r;
    }
#ifdef DEBUG
    Serial.println(uid, HEX);
#endif

    return true;
}

void LoRaMultiHop::loop(void){
    rf95.sleep();

#ifdef VERY_LOW_POWER
#ifdef DEBUG
    Serial.flush(); // This takes time! Adjust in sleep
    Serial.end();
#endif

    DramcoUno.sleep(random(10,PREAMBLE_DURATION-45), DRAMCO_UNO_3V3_DISABLE); // First sleep with 3v3 off
    DramcoUno.sleep(30, DRAMCO_UNO_3V3_ACTIVE); // Let 3v3 get stabilised
    rf95.init(false);
    this->reconfigModem();

#ifdef DEBUG
    Serial.begin(115200);
#endif
#endif

#ifndef VERY_LOW_POWER
    DramcoUno.sleep(random(10,PREAMBLE_DURATION-5), true);
#endif

#ifdef DEBUG_LED
    digitalWrite(DRAMCO_UNO_LED_NAME, HIGH);
#endif
    rf95.setModeCad(); // listen for channel activity
    
    if(!this->waitCADDone()){
#ifdef DEBUG
        Serial.println("CAD failed");
#endif
    };

    // if channel activity has been detected during the previous CAD - enable RX and receive message
    if(rf95.cadDetected()){
        rf95.setModeRx();
        rf95.waitAvailableTimeout(random(3*PREAMBLE_DURATION,6*PREAMBLE_DURATION));
        uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
        if(rf95.recv(this->rxBuf, &len)){
#ifdef DEBUG
            Serial.println(F("Message received."));
#endif
            if(this->handleMessage(this->rxBuf, len)){
                if(mrc != NULL){
                    mrc(this->rxBuf+HEADER_PAYLOAD_OFFSET, this->rxBuf[HEADER_PAYLOAD_LEN_OFFSET]);
                }
            }
        }
        else{
#ifdef DEBUG
            //Serial.println(F("Message receive failed"));
#endif
        }
        this->txTime += random(150,300); // If CAD detected, add 150-300ms to pending schedule time of next message
    }
    else{
        // handle any pending tx
        if(txPending){
            if(DramcoUno.millisWithOffset() > this->txTime){
                /*Serial.print("T now [ms]: ");
                Serial.println(DramcoUno.millisWithOffset());*/
                this->txMessage(txLen);
            }
        }

        rf95.sleep();
    }
}

bool LoRaMultiHop::waitCADDone( void ){
    DramcoUno.fastSleep();
    return rf95.cadDone();
}

bool LoRaMultiHop::waitRXAvailable(uint16_t timeout){
    unsigned long starttime = DramcoUno.millisWithOffset();
    while ((DramcoUno.millisWithOffset() - starttime) < timeout){
        DramcoUno.fastSleep();
        if (rf95.available()){
           return true;
	}
	YIELD;
    }
    return false;
}

void LoRaMultiHop::setMsgReceivedCb(MsgReceivedCb cb){
    mrc = cb;
}

bool LoRaMultiHop::sendMessage(String str, MsgType_t type){
    uint8_t pLen = str.length();
    if(pLen > RH_RF95_MAX_MESSAGE_LEN - HEADER_SIZE){
        #ifdef DEBUG
        Serial.println("Payload is too long.");
        #endif
        return false;
    }

    // copy payload to tx Buffer
    uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
    for(uint8_t i=0; i<pLen; i++){
        *pPtr++ = (uint8_t)str.charAt(i);
    }

    this->sendMessage(NULL, pLen, type);

    return true;
}

bool LoRaMultiHop::sendMessage(uint8_t * payload, uint8_t len, MsgType_t type){
    if(len > RH_RF95_MAX_MESSAGE_LEN - HEADER_SIZE){
#ifdef DEBUG
        Serial.println("Payload is too long.");
#endif
        return false;
    }

    // generate message uid
    MsgInfo_t mInfo;
    Msg_UID_t mUID = (uint16_t) random();
    mInfo.nodeUid = this->uid;
    mInfo.msgUid = mUID;

    // add message info to flood buffer
    floodBuffer.put(mInfo);

    this->initHeader(mUID);
    this->txBuf[HEADER_TYPE_OFFSET] = type;
    this->txBuf[HEADER_PAYLOAD_LEN_OFFSET] = len;

    switch(type) {
        case GATEWAY_BEACON:{
            this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(BROADCAST_UID >> 8);
            this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(BROADCAST_UID & 0x00FF);
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
        } break;

        case DATA_ROUTED:{
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
            this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(this->shortestRoute.viaNode >> 8);
            this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(this->shortestRoute.viaNode & 0x00FF);
        } break;
    
        default: return false;
    }

    if(payload != NULL){
        // copy payload to tx Buffer
        uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
        memcpy(pPtr, payload, len);
    }

#ifdef DEBUG
    Serial.println("RAW payload:");
    for(uint8_t i=0; i<len+HEADER_SIZE; i++){
        Serial.print(" ");
        if(this->txBuf[i]<16){
            Serial.print("0");
        }
        Serial.print(this->txBuf[i], HEX);
    }
    Serial.println();
#endif

    // transmit
    txMessage(len + HEADER_SIZE);

    return true;
}

void LoRaMultiHop::initHeader(Msg_UID_t mUid){
    // set hop count to 0
    this->txBuf[HEADER_NODE_UID_OFFSET] = (uint8_t)(this->uid >> 8);
    this->txBuf[HEADER_NODE_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
    this->txBuf[HEADER_MESG_UID_OFFSET] = (uint8_t)(mUid >> 8);
    this->txBuf[HEADER_MESG_UID_OFFSET+1] = (uint8_t)(mUid & 0x00FF);
    
    this->txBuf[HEADER_HOPS_OFFSET] = 0;
} 

void LoRaMultiHop::updateHeader(uint8_t * buf, uint8_t pLen){
    // set hop count to 0
    this->txBuf[HEADER_HOPS_OFFSET]++;

    // The following if statement is only needed when HEADER_NEXT_UID_OFFSET == HEADER_PREVIOUS_UID_OFFSET
    // otherwise, the order of operations does not matter
    if(this->txBuf[HEADER_TYPE_OFFSET]==GATEWAY_BEACON){
        this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(this->shortestRoute.viaNode >> 8);
        this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(this->shortestRoute.viaNode & 0x00FF);
        this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
        this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
    }
    else{
        this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
        this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
        this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(this->shortestRoute.viaNode >> 8);
        this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(this->shortestRoute.viaNode & 0x00FF);
    }
} 


bool LoRaMultiHop::handleMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }

    switch(buf[HEADER_TYPE_OFFSET]){
        // Gateway beacons are used to determine the shortest path (least hops) to the gateway
        case GATEWAY_BEACON:{
            Node_UID_t receivedFrom = this->getNodeUidFromBuffer(buf, PREVIOUS_NODE);
            uint8_t hops = buf[HEADER_HOPS_OFFSET];
            Msg_UID_t msgId = this->getMsgUidFromBuffer(buf);
            bool routeUpdated = false;
            if(this->shortestRoute.lastGatewayBeacon != msgId){
                // new gateway beacon -> route to gateway info needs to be updated
                this->shortestRoute.lastGatewayBeacon = msgId;
                this->shortestRoute.hopsToGateway = hops;
                this->shortestRoute.viaNode = receivedFrom;
                routeUpdated = true;
            }
            else{
                if(hops < this->shortestRoute.hopsToGateway){
                    // we've found a faster route
                    this->shortestRoute.hopsToGateway = hops;
                    this->shortestRoute.viaNode = receivedFrom;
                    routeUpdated = true;
                }
            }
            if(routeUpdated){
                Serial.println(F("Shortest path info updated"));
                Serial.print(F(" - gateway via: 0x"));
                Serial.println(this->shortestRoute.viaNode, HEX);
                Serial.print(F(" - in: "));
                Serial.print(this->shortestRoute.hopsToGateway);
                Serial.println(F(" hops"));
            }
            else{
                Serial.println("Route not updated.");
            }
            this->forwardMessage(buf, len);
            return false; // no user callback
        } break;

        // sensor 
        case DATA_BROADCAST:{
            // todo
            return false;
        } break;

        case DATA_ROUTED:{
            Node_UID_t sentTo = this->getNodeUidFromBuffer(buf, NEXT_NODE);
            if(sentTo == this->uid){
                Serial.println(F("Message sent to this node"));
                if(sentTo == GATEWAY_UID){
                    // end of the line -> user cb
                    Serial.println(F("Arrived at gateway -> user cb."));
                    return true;
                }
                else{ // data needs to be forwarded
                    Serial.println(F("Data needs to be forwarded"));
                    this->forwardMessage(buf, len);
                }
            }
            return false;
        } break;

        default:{
            return false;
        } break;
    }

    return false;
}

bool LoRaMultiHop::forwardMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }
    MsgInfo_t mInfo;
    mInfo.nodeUid = getNodeUidFromBuffer(buf);
    mInfo.msgUid = getMsgUidFromBuffer(buf);

    if(this->floodBuffer.find(mInfo) == CB_SUCCESS){
#ifdef DEBUG
        Serial.println(F("Duplicate. Message not forwarded."));
#endif
        return false;
    }
    else{
#ifdef DEBUG
        Serial.println(F("Message will be forwarded."));
#endif
    }

    // add message info to flood buffer
    this->floodBuffer.put(mInfo);
    
    // copy complete message to tx Buffer
    uint8_t * pPtr = (this->txBuf);
    memcpy(pPtr, buf, len);

    // update header 
    this->updateHeader(buf, len);

    // schedule tx
    uint8_t backoff =  random(PREAMBLE_DURATION,3*PREAMBLE_DURATION);
    this->txTime = DramcoUno.millisWithOffset() + backoff;
#ifdef DEBUG
    /*Serial.print("Backoff [ms]: ");
    Serial.println(backoff);
    Serial.print("TX sched [ms]: ");
    Serial.println(this->txTime);*/
#endif
    this->txPending = true;
    this->txLen = len;

    return true;
}

void LoRaMultiHop::txMessage(uint8_t len){
    rf95.send(this->txBuf, len);
    rf95.waitPacketSent(1000);
#ifdef DEBUG
    Serial.print("TX MSG: ");
    for(uint8_t i=0; i<len; i++){
        if(this->txBuf[i] < 16){
            Serial.print("0");
        }
        Serial.print(this->txBuf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
#endif
    this->txPending = false;
}

void LoRaMultiHop::reconfigModem(void){
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    rf95.setFrequency(868);

    ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
    rf95.setPreambleLength(PREAMBLE_DURATION*390/100); // 100 ms cycle for wakeup
    // From my understanding of the datasheet, preamble length in symbols = cycletime * BW / 2^SF. So,
    // for 100ms  cycle at 500 kHz and SF7, preamble = 0.100 * 500000 / 128 = 390.6 symbols.
    // Max value 65535

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
