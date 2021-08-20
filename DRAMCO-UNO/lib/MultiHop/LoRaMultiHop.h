#ifndef __LORA_MULTI_HOP__
#define	__LORA_MULTI_HOP__

#include <Arduino.h>
#include <RH_RF95.h>
#include "CircBuffer.h"

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
    SENSOR_NODE
} NodeType_t;

typedef void (*MsgReceivedCb)(uint8_t *, uint8_t);

#define MAX_BUF_SIZE 16

#define PIN_ENABLE_3V3    8
#define PIN_MODEM_SS      6
#define PIN_MODEM_INT     2

class LoRaMultiHop{
    public:
        LoRaMultiHop(NodeType_t nodeType=SENSOR_NODE);
        bool begin();
        void loop();
        bool sendMessage(String str);
        bool sendMessage(uint8_t * buf, uint8_t len);
        void setMsgReceivedCb(MsgReceivedCb cb);
        void reconfigModem(void);

    private:
        void initMsgInfo(MsgInfo_t info, uint8_t pLen); // use existing info
        void initMsgInfo(uint8_t pLen); // auto generate msg uid
        void txMessage(uint8_t len);

        bool handleMessage(uint8_t * buf, uint8_t len);
        bool forwardMessage(uint8_t * buf, uint8_t len);

        bool txPending;
        unsigned long txTime;
        uint8_t txLen;
        uint8_t txBuf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];


        NodeType_t type;
        Node_UID_t uid;
        Node_UID_t shortestPathToGateway;
        uint8_t hopsToGateway;

        CircBuffer floodBuffer;
};

#endif	/* __LORA_MULTI_HOP__ */

