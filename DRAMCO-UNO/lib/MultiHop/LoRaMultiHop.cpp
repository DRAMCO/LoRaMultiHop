#include "LoRaMultiHop.h"
#include "Dramco-UNO-Sensors.h"

// Singleton instance of the radio driver
RH_RF95 rf95(PIN_MODEM_SS, PIN_MODEM_INT);

MsgReceivedCb mrc = NULL;

void printBuffer(uint8_t * buf, uint8_t len){
    for(uint8_t i; i<len; i++){
        if(buf[i]<16) Serial.print(" 0");
        else Serial.print(' ');
        Serial.print(buf[i], HEX);
    }
}

LoRaMultiHop::LoRaMultiHop(NodeType_t nodeType){
    this->latency = PRESET_MIN_LATENCY;
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

    if(this->type == GATEWAY){
        this->uid = GATEWAY_UID;
    }
    else{
#ifdef NODE_UID
        this->uid = NODE_UID;
#else
        uint32_t r = rf95.random() * DramcoUno.random();
        randomSeed(r);  
        this->uid = (Node_UID_t) r;
#endif
    }
#ifdef DEBUG
    Serial.println(uid, HEX);
#endif

    this->shortestRoute.lastSnr = -128;
    
    return true;
}

void LoRaMultiHop::loop(void){

    bool lateForCad = false; 

    if(!lateForCad){
        rf95.sleep();

#ifdef VERY_LOW_POWER
#ifdef DEBUG
        Serial.flush(); // This takes time! Adjust in sleep
        Serial.end();
#endif
    
        DramcoUno.sleep(random(CAD_DELAY_MIN,CAD_DELAY_MAX), DRAMCO_UNO_3V3_DISABLE); // First sleep with 3v3 off
        DramcoUno.sleep(CAD_STABILIZE, DRAMCO_UNO_3V3_ACTIVE); // Let 3v3 get stabilised
        rf95.init(false);
        this->reconfigModem();

#ifdef DEBUG
        Serial.begin(115200);
#endif
#endif

#ifndef VERY_LOW_POWER
        DramcoUno.sleep(random(10,PREAMBLE_DURATION-5), true);
#endif
    }
#ifdef DEBUG_LED
    digitalWrite(DRAMCO_UNO_LED_NAME, HIGH);
#endif
    rf95.setModeCad(); // listen for channel activity
    
    if(!this->waitCADDone()){
#ifdef DEBUG
        Serial.println(F("CAD failed"));
#endif
    };

    // if channel activity has been detected during the previous CAD - enable RX and receive message
    if(rf95.cadDetected()){
        rf95.setModeRx();
        rf95.waitAvailableTimeout(random(3*PREAMBLE_DURATION,6*PREAMBLE_DURATION));
        uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
        lateForCad = true; 
        if(rf95.recv(this->rxBuf, &len)){
#ifdef DEBUG
            Serial.println(F("Message received."));
#endif
            if(this->handleMessage(this->rxBuf, len)){
                if(mrc != NULL){
                    //mrc(this->rxBuf+HEADER_NODE_UID_OFFSET, len-HEADER_NODE_UID_OFFSET);
                    mrc(this->rxBuf, len);
                }
            }
        }
        else{
#ifdef DEBUG
            //Serial.println(F("Message receive failed"));
#endif
        }
        // Reschedule TX and PresetSend to allow for backoff 
        // This is TX: just for breacon and broadcast
        if(this->txPending && DramcoUno.millisWithOffset() > this->txTime-TX_BACKOFF_MAX){
            this->txTime += random(TX_BACKOFF_MIN,TX_BACKOFF_MAX); // If CAD detected, add 150-300ms to pending schedule time of next message
        }
        // This is presetSend: only for routed/appended algorithm
        if(!this->presetSent && DramcoUno.millisWithOffset() > this->presetTime-TX_BACKOFF_MAX){
           this->presetTime += random(TX_BACKOFF_MIN,TX_BACKOFF_MAX); // If CAD detected, add 150-300ms to pending schedule time of next message
        }
    }
    else{
        // handle any pending tx (beacon or broadcast)
        if(this->txPending && DramcoUno.millisWithOffset() > this->txTime){
            this->txMessage(txLen);
            lateForCad = true; 
        }
        // handle any preset payload (routed/appended algorithm)
        if(!this->presetSent && DramcoUno.millisWithOffset() > this->presetTime){
            this->sendPresetPayload();
            lateForCad = true; 
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
        Serial.println(F("Payload is too long."));
        #endif
        return false;
    }

    // copy payload to tx Buffer
    uint8_t * pPtr = (this->txBuf + HEADER_SIZE + PAYLOAD_DATA_OFFSET);
    for(uint8_t i=0; i<pLen; i++){
        *pPtr++ = (uint8_t)str.charAt(i);
    }
    this->txBuf[HEADER_SIZE+PAYLOAD_NODE_UID_OFFSET] = this->uid;
    this->txBuf[HEADER_SIZE+PAYLOAD_LEN_OFFSET] = pLen;

    this->sendMessage(NULL, pLen+MESG_PAYLOAD_LEN_SIZE+NODE_UID_SIZE, type);
    
    return true;
}

bool LoRaMultiHop::sendMessage(uint8_t * payload, uint8_t len, MsgType_t type){
    if(len > RH_RF95_MAX_MESSAGE_LEN - HEADER_SIZE){
#ifdef DEBUG
        Serial.println(F("Payload is too long."));
#endif
        return false;
    }

    // generate message uid
    MsgInfo_t mInfo;
    Msg_UID_t mUID = (uint16_t) random();

    // add message info to flood buffer
    floodBuffer.put(mInfo);

    // build header
    this->txBuf[HEADER_MESG_UID_OFFSET] = (uint8_t)(mUID >> 8);
    this->txBuf[HEADER_MESG_UID_OFFSET+1] = (uint8_t)(mUID & 0x00FF);
    this->txBuf[HEADER_TYPE_OFFSET] = type;
    this->txBuf[HEADER_HOPS_OFFSET] = 0;

    switch(type) {
        case GATEWAY_BEACON:{
            if(sizeof(Node_UID_t)>1){
                this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(BROADCAST_UID >> 8);
                this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(BROADCAST_UID & 0x00FF);
                this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
                this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
            }
            else{
                this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(BROADCAST_UID);
                this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = this->uid;
            }
        } break;

        case DATA_ROUTED:{
            if(sizeof(Node_UID_t)>1){
                this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
                this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
                this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(this->shortestRoute.viaNode >> 8);
                this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(this->shortestRoute.viaNode & 0x00FF);
            }
            else{
                this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = this->uid;
                this->txBuf[HEADER_NEXT_UID_OFFSET] = this->shortestRoute.viaNode;
            }
        } break;

        case DATA_BROADCAST:{

        } break;

        default: return false;
    }

    if(payload != NULL){
        // copy payload to tx Buffer
        uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
        memcpy(pPtr, payload, len);
    }

    // transmit
    txMessage(len + HEADER_SIZE);

    return true;
}


void LoRaMultiHop::updateHeader(uint8_t * buf, uint8_t pLen){
    // set hop count to 0
    this->txBuf[HEADER_HOPS_OFFSET]++;

    // The following if statement is only needed when HEADER_NEXT_UID_OFFSET == HEADER_PREVIOUS_UID_OFFSET
    // otherwise, the order of operations does not matter
    if(this->txBuf[HEADER_TYPE_OFFSET]==GATEWAY_BEACON){
        if(sizeof(Node_UID_t)>1){
            this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(this->shortestRoute.viaNode >> 8);
            this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(this->shortestRoute.viaNode & 0x00FF);
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
        }
        else{
            this->txBuf[HEADER_NEXT_UID_OFFSET] = this->shortestRoute.viaNode;
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = this->uid;
        }
    }
    else{
        if(sizeof(Node_UID_t)>1){
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = (uint8_t)(this->uid >> 8);
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
            this->txBuf[HEADER_NEXT_UID_OFFSET] = (uint8_t)(this->shortestRoute.viaNode >> 8);
            this->txBuf[HEADER_NEXT_UID_OFFSET+1] = (uint8_t)(this->shortestRoute.viaNode & 0x00FF);
        }
        else{
            this->txBuf[HEADER_PREVIOUS_UID_OFFSET] = this->uid;
            this->txBuf[HEADER_NEXT_UID_OFFSET] = this->shortestRoute.viaNode;
        }
    }
} 


bool LoRaMultiHop::handleMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }

    Serial.print("Type: ");
    Serial.print(buf[HEADER_TYPE_OFFSET]);
    switch(buf[HEADER_TYPE_OFFSET]){
        // Gateway beacons are used to determine the shortest path (least hops) to the gateway
        case GATEWAY_BEACON:{
            Serial.println(F(" = BEACON"));
            if(this->uid == GATEWAY_UID){
                return false; // we got our own beacon back
            }
            Node_UID_t receivedFrom = this->getNodeUidFromBuffer(buf, PREVIOUS_NODE);
            uint8_t hops = buf[HEADER_HOPS_OFFSET];
            Msg_UID_t msgId = this->getMsgUidFromBuffer(buf);
            bool routeUpdated = false;
            if(this->shortestRoute.lastGatewayBeacon != msgId){
                // new gateway beacon -> route to gateway info needs to be updated
                this->shortestRoute.lastGatewayBeacon = msgId;
                this->shortestRoute.hopsToGateway = hops;
                this->shortestRoute.viaNode = receivedFrom;
                this->shortestRoute.lastSnr = rf95.lastSnr();
                this->shortestRoute.lastRssi = rf95.lastRssi();
                routeUpdated = true;
            }
            else{
                bool adjustRoute = false;
                // If new route hops is only 1 hop shorter / longer than previous one, check snr
                if(abs(this->shortestRoute.hopsToGateway - hops) == 1 && this->shortestRoute.lastSnr - rf95.lastSnr() >= -10){
                    adjustRoute = true; 
                }
                // If new route has the same hop count, look for the better snr value
                else if(this->shortestRoute.hopsToGateway == hops && this->shortestRoute.lastSnr < rf95.lastSnr()){
                    adjustRoute = true;
                }
                // If more than 1 hop difference, just look at the number of hops
                else if(this->shortestRoute.hopsToGateway > hops){
                    adjustRoute = true; 
                }

                if(adjustRoute){
                    // we've found a faster route
                    this->shortestRoute.hopsToGateway = hops;
                    this->shortestRoute.viaNode = receivedFrom;
                    this->shortestRoute.lastSnr = rf95.lastSnr();
                    this->shortestRoute.lastRssi = rf95.lastRssi();
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
                Serial.println(F("Route not updated."));
            }
            Serial.print(F("RSSI: "));
            Serial.println(rf95.lastRssi());
            Serial.print(F("SNR: "));
            Serial.println(rf95.lastSnr());
            
            this->forwardMessage(buf, len);
            return false; // no user callback
        } break;

        // sensor 
        case DATA_BROADCAST:{
            Serial.println(F(" = DATA BROODKAST"));
            if(this->uid == GATEWAY_UID){
                Serial.println(F("Arrived at gateway -> user cb."));
                return true;
            }
            this->forwardMessage(buf, len);
            return false;
        } break;

        case DATA_ROUTED:{
            Serial.println(F(" = DATA ROUTED"));
            Node_UID_t sentTo = this->getNodeUidFromBuffer(buf, NEXT_NODE);
            Node_UID_t sentFrom = this->getNodeUidFromBuffer(buf, SOURCE_NODE);
            Serial.print("To: 0x");
            Serial.println(sentTo, HEX);
            Serial.print("From: 0x");
            Serial.println(sentFrom, HEX);
            if(sentTo == this->uid){
                Serial.println(F("Message sent to this node"));
                if(sentTo == GATEWAY_UID){
                    // end of the line -> user cb
                    Serial.println(F("Arrived at gateway -> user cb."));
                    Serial.print(F("RSSI: "));
                    Serial.println(rf95.lastRssi());
                    Serial.print(F("SNR: "));
                    Serial.println(rf95.lastSnr());
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
    
    if(buf[HEADER_TYPE_OFFSET] == DATA_ROUTED){
        // 1. update latency
        this->latency += PRESET_LATENCY_UP_STEP;
        if(this->latency > PRESET_MAX_LATENCY) this->latency = PRESET_MAX_LATENCY;
#ifdef DEBUG
        Serial.print(F("Latency updated: "));
        Serial.println(this->latency);
#endif
        // 2. append payload to preset payload
        uint8_t payloadLen = (len - HEADER_SIZE);
        // isolate node_uid and other stuff
        this->presetForwardPayload(buf+HEADER_PAYLOAD_OFFSET, payloadLen);

        return true;
    }

    // in case of beacon or data broadcast
    // copy complete message to tx Buffer
    uint8_t * pPtr = (this->txBuf);
    memcpy(pPtr, buf, len);
    
    // update header 
    this->updateHeader(buf, len);

    // schedule tx
    uint8_t backoff =  random(PREAMBLE_DURATION,3*PREAMBLE_DURATION);
    this->txTime = DramcoUno.millisWithOffset() + backoff;
    this->txPending = true; // Will only be true for beacon!
    this->txLen = len;

    return true;
}

void LoRaMultiHop::txMessage(uint8_t len){
    rf95.send(this->txBuf, len);
    rf95.waitPacketSent(1000);
#ifdef DEBUG
    Serial.print(F("TX MSG: "));
    for(uint8_t i=0; i<len; i++){
        if(this->txBuf[i] < 16){
            Serial.print('0');
        }
        Serial.print(this->txBuf[i], HEX);
        Serial.print(' ');
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
    switch(sizeof(Node_UID_t)) {
        case 1: {
            switch (which){
                case NEXT_NODE: {
                    return (Node_UID_t)(buf[HEADER_NEXT_UID_OFFSET]);
                } break;

                case PREVIOUS_NODE: {
                    return (Node_UID_t)(buf[HEADER_PREVIOUS_UID_OFFSET]);
                } break;

                default: {
                    return (Node_UID_t)(buf[HEADER_SIZE + PAYLOAD_NODE_UID_OFFSET]);
                } break;
            }
        } break;
        case 2: {
            switch (which){
                case NEXT_NODE: {
                    return (Node_UID_t)(buf[HEADER_NEXT_UID_OFFSET]<<8 | buf[HEADER_NEXT_UID_OFFSET+1]);
                } break;

                case PREVIOUS_NODE: {
                    return (Node_UID_t)(buf[HEADER_PREVIOUS_UID_OFFSET]<<8 | buf[HEADER_PREVIOUS_UID_OFFSET+1]);
                } break;

                default: {
                    return (Node_UID_t)(buf[HEADER_SIZE + PAYLOAD_NODE_UID_OFFSET]<<8 | buf[HEADER_SIZE + PAYLOAD_NODE_UID_OFFSET + 1]);
                } break;
            }
        } break;
        default:{
            return 0;
        } break;
    }
}

Msg_UID_t LoRaMultiHop::getMsgUidFromBuffer(uint8_t * buf){
    return (Msg_UID_t)(buf[HEADER_MESG_UID_OFFSET]<<8 | buf[HEADER_MESG_UID_OFFSET+1]);
}

/* Schedule a payload to be sent
 */
bool LoRaMultiHop::presetPayload(uint8_t * payload, uint8_t len){
    // Preset payload ready for sending; we'll wait for a message that needs to be
    // forwarded, so we can append this payload to that message

    // Check if forward payload does not exceed max length
    if((len & 0x07) > RH_RF95_MAX_MESSAGE_LEN-HEADER_SIZE-NODE_UID_SIZE-MESG_PAYLOAD_LEN_SIZE){
        return false;
    }
#ifdef DEBUG
    Serial.print(F("Preset payload, will send after "));
    Serial.print(this->latency);
    Serial.println(F(" ms."));
#endif

    // Copy message to buffer
    if(sizeof(Node_UID_t)>1){
        this->presetOwnData[PAYLOAD_NODE_UID_OFFSET] =  (uint8_t)(this->uid >> 8);
        this->presetOwnData[PAYLOAD_NODE_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
    }
    else{
        this->presetOwnData[PAYLOAD_NODE_UID_OFFSET] =  this->uid;
    }
    this->presetOwnData[PAYLOAD_LEN_OFFSET] = (len & 0x07);
    memcpy(this->presetOwnData+PAYLOAD_DATA_OFFSET, payload, len);
    this->presetLength = len + NODE_UID_SIZE + MESG_PAYLOAD_LEN_SIZE;

    // schedule transmission if needed
    if(this->presetSent){ // start new window
        this->presetSent = false;
        this->presetTime = DramcoUno.millisWithOffset() + random(this->latency - PRESET_MAX_LATENCY_RAND_WINDOW, this->latency + PRESET_MAX_LATENCY_RAND_WINDOW);
    }
    return true;
}

bool LoRaMultiHop::presetForwardPayload(uint8_t * payload, uint8_t len){
    // Preset payload ready for sending; we'll wait for a message that needs to be
    // forwarded, so we can append this payload to that message

    // Check if forward payload does not exceed max length
    if(len > RH_RF95_MAX_MESSAGE_LEN-HEADER_SIZE-NODE_UID_SIZE-MESG_PAYLOAD_LEN_SIZE){
#ifdef DEBUG
        Serial.println(F("Max. message length exceeded. Extra payload will be dropped."));
#endif
        return false;
    }
#ifdef DEBUG
    Serial.print(F("Preset payload, will send after "));
    Serial.print(this->latency);
    Serial.println(F(" ms."));
#endif

    memcpy((this->presetForwardedData + this->presetForwardedLength), payload, len);
    this->presetForwardedLength += len;

    // schedule transmission if needed
    if(this->presetSent){ // start new window
        this->presetSent = false;
        this->presetTime = DramcoUno.millisWithOffset() + random(this->latency - PRESET_MAX_LATENCY_RAND_WINDOW, this->latency + PRESET_MAX_LATENCY_RAND_WINDOW);
    }

    return true;
}

bool LoRaMultiHop::sendPresetPayload( void ){
    if(this->presetSent){
        return true;
    }
    this->presetSent = true;

  
    if(this->latency-PRESET_MIN_LATENCY < PRESET_LATENCY_DOWN_STEP) this->latency = PRESET_MIN_LATENCY;
    else this->latency -= PRESET_LATENCY_DOWN_STEP; 
    Serial.print(F("Latency updated: "));
    Serial.println(this->latency);

    // create dummy payload if no preset data available
    if(presetLength == 0){
        if(sizeof(Node_UID_t)>1){
            this->presetOwnData[PAYLOAD_NODE_UID_OFFSET] =  (uint8_t)(this->uid >> 8);
            this->presetOwnData[PAYLOAD_NODE_UID_OFFSET+1] = (uint8_t)(this->uid & 0x00FF);
        }
        else{
            this->presetOwnData[PAYLOAD_NODE_UID_OFFSET] =  this->uid;
        }
        this->presetOwnData[PAYLOAD_LEN_OFFSET] = (uint8_t)(this->presetForwardedLength<<3);
        this->presetLength = 2;
    }
    else{
        this->presetOwnData[PAYLOAD_LEN_OFFSET] |= (uint8_t)(this->presetForwardedLength<<3);
    }

    // copy preset forward data to preset own data
    memcpy((this->presetOwnData + this->presetLength), this->presetForwardedData, this->presetForwardedLength);
    this->presetLength += this->presetForwardedLength;

    // Send message, but let go of extra header
    bool rv = this->sendMessage(this->presetOwnData, this->presetLength, DATA_ROUTED);

    // reset counters
    this->presetLength = 0;
    this->presetForwardedLength = 0;

    return rv;
}
