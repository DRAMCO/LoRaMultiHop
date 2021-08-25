#ifndef __LORA_MULTI_HOP__
#define	__LORA_MULTI_HOP__

#include <Arduino.h>
#include <RH_RF95.h>
#include "CircBuffer.h"

#define PREAMBLE_DURATION 200.0

#define MAX_BUF_SIZE 16

#define PIN_ENABLE_3V3    8
#define PIN_MODEM_SS      6
#define PIN_MODEM_INT     2

typedef uint16_t Msg_UID_t;
typedef uint16_t Node_UID_t;
typedef uint8_t Msg_Type_t;

typedef enum msgTypes{
    GATEWAY_BEACON = 0x01,
    DATA_BROADCAST = 0x02,
    DATA_ROUTED = 0x03
} MsgType_t;

typedef enum nodeTypes{
    GATEWAY,
    SENSOR
} NodeType_t;

typedef enum nodeIds{
    SOURCE_NODE,
    NEXT_NODE,
    PREVIOUS_NODE
} NodeID_t;

typedef struct routeInfo{
    Node_UID_t viaNode;
    uint8_t hopsToGateway;
    Msg_UID_t lastGatewayBeacon;
} RouteToGatewayInfo_t;

typedef void (*MsgReceivedCb)(uint8_t *, uint8_t);

typedef void (*MsgReceivedCb)(uint8_t *, uint8_t);

class LoRaMultiHop{
    public:
        LoRaMultiHop(NodeType_t nodeType=SENSOR);
        bool begin();
        void loop();

        bool sendMessage(String str, MsgType_t type);
        bool sendMessage(uint8_t * payload, uint8_t len, MsgType_t type);
        void setMsgReceivedCb(MsgReceivedCb cb);
        void reconfigModem(void);

    private:
        void updateHeader(uint8_t * buf, uint8_t len);
        void initHeader(Msg_UID_t uid);
        void txMessage(uint8_t len);

        bool waitCADDone( void );
        bool waitRXAvailable(uint16_t timeout);

        bool handleMessage(uint8_t * buf, uint8_t len);
        bool forwardMessage(uint8_t * buf, uint8_t len);

        Node_UID_t getNodeUidFromBuffer(uint8_t * buf, NodeID_t which=SOURCE_NODE);
        Msg_UID_t getMsgUidFromBuffer(uint8_t * buf);
 
        bool txPending;
        unsigned long txTime;
        uint8_t txLen;
        uint8_t txBuf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];

        NodeType_t type;
        Node_UID_t uid;

        RouteToGatewayInfo_t shortestRoute;

        CircBuffer floodBuffer;
};

#endif	/* __LORA_MULTI_HOP__ */
