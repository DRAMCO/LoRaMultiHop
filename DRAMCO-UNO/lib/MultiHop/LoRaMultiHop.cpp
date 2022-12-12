#include "LoRaMultiHop.h"
#include "Dramco-UNO-Sensors.h"
#include "CircBuffer.h"

// Singleton instance of the radio driver
RH_RF95 rf95(PIN_MODEM_SS, PIN_MODEM_INT);

MsgReceivedCb mrc = NULL;

/*--------------- UTILS ----------------- */
static void wdtYield(){
    DramcoUno.loop();
}

void printBuffer(uint8_t * buf, uint8_t len){
#pragma region DEBUG
#ifdef DEBUG
    for(uint8_t i; i<len; i++){
        if(buf[i]<16) Serial.print(" 0");
        else Serial.print(' ');
        Serial.print(buf[i], HEX);
    }
    Serial.println();
#endif
#pragma endregion
}

/*--------------- CONSTRUCTOR ----------------- */
LoRaMultiHop::LoRaMultiHop(NodeType_t nodeType){
    this->latency = AGGREGATION_TIMER_NOMINAL;
    this->type = nodeType;
}

/*--------------- BEGIN ----------------- */
bool LoRaMultiHop::begin(){
#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("Initializing neighbours table... "));
#endif
#pragma endregion

    this->neighbours = (RouteToGatewayInfo_t *)malloc(sizeof(RouteToGatewayInfo_t)*MAX_NUMBER_OF_NEIGHBOURS);

#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F("done."));
    Serial.print(F("Initializing flood buffer... "));
#endif
#pragma endregion

    this->floodBuffer.init(MESSAGE_FLOODBUFFER_SIZE);
    // prefill buffer (otherwise "CircBuffer::find()"" fails miserably and I've yet to fix it)
    MsgInfo_t dummy;
    for(uint8_t i=0; i<MESSAGE_FLOODBUFFER_SIZE; i++){
        this->floodBuffer.put(dummy);
    }
#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F("done."));
    Serial.print(F("Enabling 3V3 regulator... "));
#endif
#pragma endregion

    // Dramco uno - enable 3v3 voltage regulator
    pinMode(PIN_ENABLE_3V3, OUTPUT);
    digitalWrite(PIN_ENABLE_3V3, HIGH);

#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F("done."));
    Serial.print(F("Initializing modem... "));
#endif
#pragma endregion

    if(!rf95.init()){
#pragma region DEBUG
#ifdef DEBUG
        Serial.println(F("failed."));
#endif
#pragma endregion
        return false;
    }

    this->reconfigModem();

#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F("done"));
    Serial.print(F("Setting node UID: "));
#endif
#pragma endregion

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
#pragma region DEBUG
#ifdef DEBUG
    Serial.println(uid, HEX);
#endif
#pragma endregion
    
    return true;
}

/*--------------- LOOP ----------------- */
void LoRaMultiHop::loop(void){
    rf95.sleep();

#ifdef VERY_LOW_POWER
#pragma region DEBUG
#ifdef DEBUG
        Serial.flush(); // This takes time! Adjust in sleep
        Serial.end();
#endif
#pragma endregion
    
        DramcoUno.sleep(random(CAD_DELAY_MIN,CAD_DELAY_MAX), DRAMCO_UNO_3V3_DISABLE); // First sleep with 3v3 off
        DramcoUno.sleep(CAD_STABILIZE, DRAMCO_UNO_3V3_ACTIVE); // Let 3v3 get stabilised
        rf95.init(false);
        this->reconfigModem();

#ifdef DEBUG
#pragma region DEBUG
        Serial.begin(115200);
#endif
#pragma endregion 
#endif // VERY_LOW_POWER

#ifndef VERY_LOW_POWER
        DramcoUno.sleep(random(10,PREAMBLE_DURATION-5), true);
#endif
    
#ifdef DEBUG_LED
    digitalWrite(DRAMCO_UNO_LED_NAME, HIGH);
#endif
    rf95.setModeCad(); // listen for channel activity
    
    if(!this->waitCADDone()){
#pragma region DEBUG
#ifdef DEBUG
        Serial.println(F("CAD failed"));
#endif
#pragma endregion
    };

    // if channel activity has been detected during the previous CAD - enable RX and receive message
    if(rf95.cadDetected()){
        rf95.setModeRx();
        rf95.waitAvailableTimeout(random(3*PREAMBLE_DURATION,6*PREAMBLE_DURATION));
        uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
        if(rf95.recv(this->rxBuf, &len)){
#pragma region DEBUG
#ifdef DEBUG
            Serial.println(F("Message received."));
#endif
#pragma endregion
            if(this->handleAnyRxMessage(this->rxBuf, len)){
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
        // TODO: make this clear, can this be just to the next CAD? 
        unsigned long now = DramcoUno.millisWithOffset();
        if(this->txPending && (now > this->txTime-COLLISION_DELAY_MAX)){
            this->txTime = now + random(COLLISION_DELAY_MIN,COLLISION_DELAY_MAX); // If CAD detected, add 150-300ms to pending schedule time of next message
        }
        // This is presetSend: only for routed/appended algorithm
        if(!this->presetSent && (now > this->presetTime-COLLISION_DELAY_MAX)){
           this->presetTime = now + random(COLLISION_DELAY_MIN,COLLISION_DELAY_MAX); // If CAD detected, add 150-300ms to pending schedule time of next message
        }
    }
    else{
        // handle any pending tx (beacon or broadcast)
        unsigned long now = DramcoUno.millisWithOffset();
        if(this->txPending && (now > this->txTime)){
            this->txMessage(txLen);
            this->txTime = now + random(COLLISION_DELAY_MIN,COLLISION_DELAY_MAX);
        }
        else{
           // handle any preset payload (routed/appended algorithm)
            if(!this->presetSent && (now > this->presetTime)){
                this->sendAggregatedMessage();
            }
        }
        rf95.sleep();
    }
    
    DramcoUno.loop();

}

/*--------------- PHYSICAL RELATED METHODS ----------------- */

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

void LoRaMultiHop::setMsgReceivedCb(MsgReceivedCb cb){
    mrc = cb;
}

void LoRaMultiHop::txMessage(uint8_t len){
    rf95.send(this->txBuf, len);
    void (*  fptr)() = &wdtYield;
    rf95.waitPacketSent(1000, fptr);
    this->txPending = false;

#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("TX MSG: "));
    printBuffer(this->txBuf, len);
#endif
#pragma endregion

}

bool LoRaMultiHop::getFieldFromBuffer(BaseType_t * field, uint8_t * buf, uint8_t fieldOffset, size_t size){
    BaseType_t rv = 0;
    if(size == 0 || size > sizeof(BaseType_t)){
        return false;
    }

    for(uint8_t index=fieldOffset; index<fieldOffset+size; index++){
        rv<<=8;
        rv |= buf[index];
    }
    *field = rv;
    return true;
}

bool LoRaMultiHop::setFieldInBuffer(BaseType_t field, uint8_t * buf, uint8_t fieldOffset, size_t size){
    if(size == 0 || size > sizeof(BaseType_t)){
        return false;
    }

    // TODO: check buf length?

    BaseType_t temp = field;
    for(int8_t index=fieldOffset+size-1; index>=fieldOffset; index--){
        buf[index] = temp & 0xFF;
        temp >>= 8;
    }
    return true;
}


/*--------------- PROTOCOL RELATED METHODS ----------------- */
/*--- 1. Message type independant methods ---*/
bool LoRaMultiHop::handleAnyRxMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }

#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("RX MSG: "));
    printBuffer(buf, len);

    Serial.print(F("Type: "));
    Serial.print(buf[HEADER_TYPE_OFFSET]);
#endif
#pragma endregion
    
    // Check if incoming message has already been transmitted, otherwise return false
    switch(buf[HEADER_TYPE_OFFSET]){
        // ROUTE DISCOVERY MESSAGE: Determine best path to the gateway
        case MESG_ROUTE_DISCOVERY:{
            return handleRouteDiscoveryMessage(buf, len); // no user callback
        } break;
        
        // ROUTED MESSAGES: To end node (gateway), only forward according to routing 
        case MESG_ROUTED:{
            return handleRoutedMessage(buf, len);
        } break;

        // BROADCAST: To all nodes, always forward 
        case MESG_BROADCAST:{
            return handleBroadcastMessage(buf, len);
        } break;

        default:{
            return false;
        } break;
    }

    return false;
}

bool LoRaMultiHop::sendMessage(String str, MsgType_t type){
    uint8_t pLen = str.length();
    if(pLen > OWN_DATA_BUFFER_SIZE){
#pragma region DEBUG
#ifdef DEBUG
        Serial.println(F("Payload is too long."));
#endif
#pragma endregion
        return false;
    }

    // copy payload to tx Buffer
    uint8_t * pPtr = (this->ownDataBuffer);
    for(uint8_t i=0; i<pLen; i++){
        *pPtr++ = (uint8_t)str.charAt(i);
    }
    this->ownDataBufferLength = pLen;

    return this->sendMessage(type);
}

bool LoRaMultiHop::sendMessage(uint8_t * payload, uint8_t len, MsgType_t type){
    if(len > OWN_DATA_BUFFER_SIZE){
        return false;
    }
    
    memcpy(payload, this->ownDataBuffer, len);
    this->ownDataBufferLength = len;
    
    return sendMessage(type);
}

bool LoRaMultiHop::sendMessage(MsgType_t type){
    uint8_t len = HEADER_SIZE + this->ownDataBufferLength + this->forwardedDataBufferLength;

#pragma region DEBUG
#ifdef DEBUG
    if(len > TX_BUFFER_SIZE){
        Serial.println(F("Payload is too long."));
    }
#endif
#pragma endregion

    // Generate message uid
    MsgInfo_t mInfo;
    Msg_UID_t mUID = (uint16_t) random();

    // Add message info to flood buffer
    addMessageToFloodBuffer(&mInfo);

    // Build header
    this->setFieldInBuffer(mUID, this->txBuf, HEADER_MESG_UID_OFFSET, sizeof(Msg_UID_t));
    this->setFieldInBuffer(type, this->txBuf, HEADER_TYPE_OFFSET, sizeof(Msg_Type_t));
    this->setFieldInBuffer(0, this->txBuf, HEADER_HOPS_OFFSET, sizeof(uint8_t));

    // Adjust header types, based on message type
    switch(type) {
        case MESG_ROUTE_DISCOVERY:{
            this->setFieldInBuffer(BROADCAST_UID, this->txBuf, HEADER_NEXT_UID_OFFSET, sizeof(Node_UID_t));
            this->setFieldInBuffer(this->uid, this->txBuf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
            this->setFieldInBuffer(0, this->txBuf, HEADER_LQI_OFFSET, sizeof(Msg_LQI_t));
        } break;

        case MESG_ROUTED:{
            this->setFieldInBuffer(this->uid, this->txBuf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
            if(this->bestRoute != NULL){
                this->setFieldInBuffer(this->bestRoute->viaNode, this->txBuf, HEADER_NEXT_UID_OFFSET, sizeof(Node_UID_t));
            }
            else{
                this->setFieldInBuffer(0, this->txBuf, HEADER_NEXT_UID_OFFSET, sizeof(Node_UID_t));
#pragma region DEBUG
#ifdef DEBUG
                Serial.println(F("No route to gateway established yet."));
#endif
#pragma endregion
            }
        } break;

        case MESG_BROADCAST:{
            // TODO: implement broadcast
        } break;

        default: return false;
    }

#pragma region DEBUG
#ifdef DEBUG
    if((this->ownDataBufferLength == 0) && (this->forwardedDataBufferLength == 0)){
        Serial.println(F("Payload is empty"));
    }
#endif
#pragma endregion

    if(this->ownDataBufferLength > 0){
        // copy payload to tx Buffer
        uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET);
        memcpy(pPtr, this->ownDataBuffer, this->ownDataBufferLength);
    }
    
    // copy aggregated data to tx buffer
    if(this->forwardedDataBufferLength > 0){
        // copy payload to tx Buffer
        uint8_t * pPtr = (this->txBuf + HEADER_PAYLOAD_OFFSET+this->ownDataBufferLength);
        memcpy(pPtr, this->forwardedDataBuffer, this->forwardedDataBufferLength);
    }

    // Final transmit message (NOW)
    this->txMessage(len);

    return true;
}

bool LoRaMultiHop::checkFloodBufferForMessage(uint8_t * buf, uint8_t len){
    // Node_UID_t nodeUid;
    BaseType_t msgUid;
    // getFieldFromBuffer((BaseType_t *)(&nodeUid), buf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
    getFieldFromBuffer(&msgUid, buf, HEADER_MESG_UID_OFFSET, sizeof(Msg_UID_t));

    MsgInfo_t mInfo;
    mInfo.msgUid = (Msg_UID_t)msgUid;

    if(this->floodBuffer.find(mInfo) == CB_SUCCESS){
        return true; // Found message in buffer
    }else{
        return false;
    }
}

bool LoRaMultiHop::addMessageToFloodBuffer(uint8_t * buf, uint8_t len){
    //Node_UID_t nodeUid;
    BaseType_t msgUid;
    //getFieldFromBuffer((BaseType_t *)(&nodeUid), buf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
    getFieldFromBuffer(&msgUid, buf, HEADER_MESG_UID_OFFSET, sizeof(Msg_UID_t));

    MsgInfo_t mInfo;
    //mInfo.nodeUid = nodeUid;
    mInfo.msgUid = (Msg_UID_t)msgUid;

    //if(this->floodBuffer.find(mInfo) == CB_NOT_IN_BUFFER){
    this->floodBuffer.put(mInfo);
    //}

    return true;
}


bool LoRaMultiHop::addMessageToFloodBuffer(MsgInfo_t * mInfo){
    if(this->floodBuffer.find(*mInfo) == CB_NOT_IN_BUFFER){
        this->floodBuffer.put(*mInfo);
    }

    return true;
}

bool LoRaMultiHop::updateNeighbours(RouteToGatewayInfo_t &neighbour){
    bool neighbourInList = false;
    for(uint8_t i=0; i<this->numberOfNeighbours; i++){ // look up neighbour in the list
        if(neighbour.viaNode == this->neighbours[i].viaNode){ // neighbour is in the list -> update info
            this->neighbours[i] = neighbour;
            neighbourInList = true;
#pragma region DEBUG
#ifdef DEBUG
            Serial.println(F("Updated neighbour table."));
#endif
#pragma endregion
            return true;
        }
    }
    if(!neighbourInList){ // -> add neighbour to the list if there's still room
        if(this->numberOfNeighbours < MAX_NUMBER_OF_NEIGHBOURS){
            this->neighbours[this->numberOfNeighbours] = neighbour;
            this->numberOfNeighbours++;
#pragma region DEBUG
#ifdef DEBUG
            Serial.println(F("Added neighbour to table."));
#endif
#pragma endregion
            return true;
        }
        // TODO: else -> replace worst neighbour if this one is better
    }
    return false;
}

Msg_LQI_t LoRaMultiHop::computeCummulativeLQI(Msg_LQI_t previousCummulativeLQI, int8_t snr){
    // super-duper-high-tech algorithm to compute a Link Quality Indicator
    return previousCummulativeLQI + (RH_RF95_MAX_SNR-snr); // the lower the cummulative SNR is, the better
}

void LoRaMultiHop::updateRouteToGateway(){
    // default best route is first neighbour
    uint8_t idxBestRoute = 0;
    
    // loop through all neigbours in search for a better one
    for(uint8_t i=0; i<this->numberOfNeighbours; i++){
        // make sure isBest is set to false, it will be set later (if it is)
        this->neighbours[i].isBest = false;
        // search for lowest LQI
        if(this->neighbours[i].cumLqi < this->neighbours[idxBestRoute].cumLqi){ 
            // cummulative LQI of this neighbour is better than the previous one -> save index of this neighbour
            idxBestRoute = i;
        }
        else{ // LQI was not lower then the previous one
            // see if the LQI is equal -> best route is least number of hops
            if(this->neighbours[i].cumLqi == this->neighbours[idxBestRoute].cumLqi){
                if(this->neighbours[i].hopsToGateway < this->neighbours[idxBestRoute].hopsToGateway){
                    idxBestRoute = i;
                }
                else{ // nr of hops was not lower
                    if(this->neighbours[i].lastSnr > this->neighbours[idxBestRoute].lastSnr){
                        idxBestRoute = i;
                    }
                }
            }
        }
    }

    // idxBestRoute should now point to the best route
    this->neighbours[idxBestRoute].isBest = true;
    this->bestRoute = this->neighbours+idxBestRoute;
}

void LoRaMultiHop::printNeighbours(){
#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F("| VIANODE | SNR | CUMLQI | HOPS | BEST |"));
    Serial.println(F("+---------+-----+--------+------+------+"));
    char c[32];
    for(uint8_t i=0; i<this->numberOfNeighbours; i++){
        Serial.print(F("| 0x"));
        sprintf(c, "%02x    | %3d | %6d | %4d", this->neighbours[i].viaNode, this->neighbours[i].lastSnr, this->neighbours[i].cumLqi, this->neighbours[i].hopsToGateway);
        Serial.print(c);
        if(this->neighbours[i].isBest){
            Serial.println(F(" |  **  |"));
        }
        else{
            Serial.println(F(" |      |"));
        }
    }
    Serial.println();
#endif
#pragma endregion
}

/*--- 2. Route discovery methods ---*/
bool LoRaMultiHop::handleRouteDiscoveryMessage(uint8_t * buf, uint8_t len){
#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F(" = MESG_ROUTE_DISCOVERY"));
#endif
#pragma endregion

    if(this->uid == GATEWAY_UID){
        return false; // we got our own beacon back
    }

    // Get info from message
    BaseType_t receivedFrom;
    BaseType_t hops;
    BaseType_t msgId;
    BaseType_t lqiCum;

    this->getFieldFromBuffer(&receivedFrom, buf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
    this->getFieldFromBuffer(&hops, buf, HEADER_HOPS_OFFSET, sizeof(uint8_t));
    this->getFieldFromBuffer(&lqiCum, buf, HEADER_LQI_OFFSET, sizeof(Msg_LQI_t));
    this->getFieldFromBuffer(&msgId, buf,HEADER_MESG_UID_OFFSET, sizeof(Msg_UID_t));

    // Set info in neighbour struct to update neighbour list
    RouteToGatewayInfo_t neighbour;
    neighbour.lastGatewayBeacon = (Msg_UID_t)msgId;
    neighbour.hopsToGateway = (uint8_t)hops;
    neighbour.viaNode = (Node_UID_t)receivedFrom;
    neighbour.lastSnr = rf95.lastSnr();
    neighbour.lastRssi = rf95.lastRssi();
    neighbour.cumLqi = computeCummulativeLQI((Msg_LQI_t)lqiCum, neighbour.lastSnr);
    neighbour.isBest = false;

    // Update neighbour list
    this->updateNeighbours(neighbour);

    // Check if new and better route is available
    this->updateRouteToGateway();
#pragma region DEBUG
#ifdef DEBUG
    this->printNeighbours();
#endif
#pragma endregion

    if(this->checkFloodBufferForMessage(buf, len)){
#pragma region DEBUG
#ifdef DEBUG
        Serial.println(F("Duplicate. Message not forwarded."));
#endif
#pragma endregion
        return true;
    }else{
#pragma region DEBUG
#ifdef DEBUG
        Serial.println(F("Message will be forwarded."));
#endif
#pragma endregion
        this->addMessageToFloodBuffer(buf, len);
        return this->forwardRouteDiscoveryMessage(buf, len);
    }
}

bool LoRaMultiHop::forwardRouteDiscoveryMessage(uint8_t * buf, uint8_t len){
    if(len < HEADER_SIZE){
        return false;
    }

    // In case of beacon or data broadcast
    // copy complete message to tx Buffer
    uint8_t * pPtr = (this->txBuf);
    memcpy(pPtr, buf, len);
    
    // Update header 
    this->updateRouteDiscoveryHeader(buf, len);

    // Schedule tx
    uint8_t backoff =  random(PREAMBLE_DURATION,3*PREAMBLE_DURATION);
    this->txTime = DramcoUno.millisWithOffset() + backoff;
    this->txPending = true; 
    this->txLen = len;

    return true;
}

void LoRaMultiHop::updateRouteDiscoveryHeader(uint8_t * buf, uint8_t pLen){
    // Increase hop count
    this->txBuf[HEADER_HOPS_OFFSET]++;

    // Adjust LQI
    this->setFieldInBuffer(this->bestRoute->cumLqi, this->txBuf, HEADER_LQI_OFFSET, sizeof(Msg_LQI_t));

    // The following if statement is only needed when HEADER_NEXT_UID_OFFSET == HEADER_PREVIOUS_UID_OFFSET
    // otherwise, the order of operations does not matter
    if(this->txBuf[HEADER_TYPE_OFFSET]==MESG_ROUTE_DISCOVERY){
        this->setFieldInBuffer(this->bestRoute->viaNode, this->txBuf, HEADER_NEXT_UID_OFFSET, sizeof(Node_UID_t));
        this->setFieldInBuffer(this->uid, this->txBuf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
    }
    else{
        this->setFieldInBuffer(this->bestRoute->viaNode, this->txBuf, HEADER_PREVIOUS_UID_OFFSET, sizeof(Node_UID_t));
        this->setFieldInBuffer(this->uid, this->txBuf, HEADER_NEXT_UID_OFFSET, sizeof(Node_UID_t));
    }
} 


/*--- 2. Routed message methods ---*/
bool LoRaMultiHop::handleRoutedMessage(uint8_t * buf, uint8_t len){
    if(this->checkFloodBufferForMessage(buf, len)){
        return false;
    } else {
        addMessageToFloodBuffer(buf, len);
    }
    
    BaseType_t temp;
    BaseType_t sentTo, sentFrom;
    this->getFieldFromBuffer(&temp, buf, HEADER_NEXT_UID_OFFSET, sizeof(Node_UID_t));
    sentTo = (Node_UID_t) temp;
    this->getFieldFromBuffer(&temp, buf, HEADER_SIZE+PAYLOAD_NODE_UID_OFFSET, sizeof(Node_UID_t));
    sentFrom = (Node_UID_t) temp;

#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F(" = ROUTED MESSAGE"));
    Serial.print("To: 0x");
    Serial.println(sentTo, HEX);
    Serial.print("From: 0x");
    Serial.println(sentFrom, HEX);
//  Serial.print(F("RSSI: "));
//  Serial.println(rf95.lastRssi());
//  Serial.print(F("SNR: "));
//  Serial.println(rf95.lastSnr());
    Serial.print(F("Action: "));
    if(sentTo == this->uid){
        if(sentTo == GATEWAY_UID){
            Serial.println(F("none/arrived"));
        }else{ 
            Serial.println(F("forward"));
        }

    }else{
        Serial.println("none/not-for-me");
    }
#endif
#pragma endregion

    if(sentTo == this->uid){
        if(sentTo == GATEWAY_UID){
            return true;
        }else{ 
            // Message will be forwarded after aggregation,
            // 1. Adjust aggregation timer for dynamic aggregation
            this->latency += AGGREGATION_TIMER_UPSTEP;
            if(this->latency > AGGREGATION_TIMER_MAX) this->latency = AGGREGATION_TIMER_MAX;
#pragma region DEBUG
#ifdef DEBUG
            Serial.print(F("Latency: "));
            Serial.println(this->latency);
#endif
#pragma endregion
            // 2. Setup payload for aggregation to other message
            uint8_t payloadLen = (len - HEADER_SIZE);
            this->prepareRxDataForAggregation(buf+HEADER_PAYLOAD_OFFSET, payloadLen);
        }
    }

#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("Packet: "));
    for(uint8_t i=0; i<len; i++){
        if(buf[i] < 16){
            Serial.print('0');
        }
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
#endif
#pragma endregion

    return false;
}

bool LoRaMultiHop::prepareOwnDataForAggregation(uint8_t * payload, uint8_t len){
    // Preset payload ready for sending; we'll wait for a message that needs to be
    // forwarded, so we can append this payload to that message

    // Check if forward payload does not exceed max length
    if((len & 0x07) > AGGREGATION_BUFFER_SIZE-HEADER_SIZE-NODE_UID_SIZE-MESG_PAYLOAD_LEN_SIZE){
        return false;
    }

#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("Preset payload, will send after "));
    Serial.print(this->latency);
    Serial.println(F(" ms."));
#endif
#pragma endregion

    // Copy message to buffer
    setFieldInBuffer(this->uid, this->ownDataBuffer, PAYLOAD_NODE_UID_OFFSET, sizeof(Node_UID_t));
    setFieldInBuffer((len & 0x07), this->ownDataBuffer, PAYLOAD_LEN_OFFSET, MESG_PAYLOAD_LEN_SIZE);

    memcpy(this->ownDataBuffer+PAYLOAD_DATA_OFFSET, payload, len);
    this->ownDataBufferLength = len + NODE_UID_SIZE + MESG_PAYLOAD_LEN_SIZE;

    // schedule transmission if needed
    if(this->presetSent){ // start new window
        this->presetSent = false;
        this->presetTime = DramcoUno.millisWithOffset() + random(this->latency - AGGREGATION_TIMER_RANDOM, this->latency + AGGREGATION_TIMER_RANDOM);
    }
    return true;
}

bool LoRaMultiHop::prepareRxDataForAggregation(uint8_t * payload, uint8_t len){
    // Preset forwarded payload ready for sending; we'll wait for a message that needs to be
    // forwarded, so we can append this payload to that message

    // Check if forward payload does not exceed max length
    if(len > TX_BUFFER_SIZE-HEADER_SIZE-NODE_UID_SIZE-MESG_PAYLOAD_LEN_SIZE){
#pragma region DEBUG
#ifdef DEBUG
        Serial.println(F("Max. message length exceeded. Extra payload will be dropped."));
#endif
#pragma endregion
        return false;
    }

#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("Preset forwarded payload, will send after "));
    Serial.print(this->latency);
    Serial.println(F(" ms."));
#endif
#pragma endregion

    memcpy((this->forwardedDataBuffer + this->forwardedDataBufferLength), payload, len);
    this->forwardedDataBufferLength += len;

    // schedule transmission if needed
    if(this->ownDataBufferLength + this->forwardedDataBufferLength > PAYLOAD_TX_THRESHOLD){
        this->presetSent = false;
        this->presetTime = DramcoUno.millisWithOffset();
    }
    else{
        if(this->presetSent){ // start new window
            this->presetSent = false;
            this->presetTime = DramcoUno.millisWithOffset() + random(this->latency - AGGREGATION_TIMER_RANDOM, this->latency + AGGREGATION_TIMER_RANDOM);
        }
    }

    return true;
}

// build message in tx buffer and transmit
bool LoRaMultiHop::sendAggregatedMessage( void ){
    if(this->presetSent){
        return true;
    }
    this->presetSent = true;

  
    if(this->latency-AGGREGATION_TIMER_MIN < AGGREGATION_TIMER_DOWNSTEP) this->latency = AGGREGATION_TIMER_MIN;
    else this->latency -= AGGREGATION_TIMER_DOWNSTEP; 

#pragma region DEBUG
#ifdef DEBUG
    Serial.print(F("Latency updated: "));
    Serial.println(this->latency);
#endif
#pragma endregion

    // create dummy payload if no preset data available
    if(this->ownDataBufferLength == 0){
        setFieldInBuffer(this->uid, this->ownDataBuffer, PAYLOAD_NODE_UID_OFFSET, sizeof(Node_UID_t));
        setFieldInBuffer((uint8_t)(this->forwardedDataBufferLength<<3), this->ownDataBuffer, PAYLOAD_LEN_OFFSET, sizeof(uint8_t));
        this->ownDataBufferLength = 2;
    }
    else{
        this->ownDataBuffer[PAYLOAD_LEN_OFFSET] |= (uint8_t)(this->forwardedDataBufferLength<<3);
    }

    // Send message
    bool rv = this->sendMessage(MESG_ROUTED);

    // reset counters
    this->ownDataBufferLength = 0;
    this->forwardedDataBufferLength = 0;

    return rv;
}


/*--- 3. Broadcast message methods ---*/
bool LoRaMultiHop::handleBroadcastMessage(uint8_t * buf, uint8_t len){
#pragma region DEBUG
#ifdef DEBUG
    Serial.println(F(" = BROADCAST MESSAGE"));
    if(this->uid == GATEWAY_UID){
        Serial.println(F("Arrived at gateway -> user cb."));
    }
#endif
#pragma endregion

    if(this->uid == GATEWAY_UID){
        return true;
    }
    // TODO
    //this->forwardBroadcastMessage(buf, len);
    return false;
}
