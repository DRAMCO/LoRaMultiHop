#ifndef __LORA_MULTI_HOP__
#define	__LORA_MULTI_HOP__

#include <Arduino.h>
#include <RH_RF95.h>
#include "CircBuffer.h"

#define PREAMBLE_DURATION               200.0 // In ms
#define PRESET_MIN_LATENCY              50000 // In ms
#define PRESET_MAX_LATENCY              60000 // In ms
#define PRESET_MAX_LATENCY_RAND_WINDOW  1000 // Random window around PRESET_MAX_LATENCY
#define PRESET_LATENCY_UP_STEP          10000
#define PRESET_LATENCY_DOWN_STEP        0

#define MAX_BUF_SIZE 16     // Max preset buffer size 

#define PIN_ENABLE_3V3    8
#define PIN_MODEM_SS      6
#define PIN_MODEM_INT     2

typedef uint16_t Msg_UID_t;
typedef uint16_t Node_UID_t;
typedef uint8_t Msg_Type_t;

#define GATEWAY_UID                 0x0000
#define BROADCAST_UID               0xFFFF

#define NODE_UID_SIZE               sizeof(Node_UID_t)
#define MESG_UID_SIZE               sizeof(Msg_UID_t)
#define MESG_TYPE_SIZE              sizeof(Msg_Type_t)
#define MESG_HOPS_SIZE              1
#define MESG_PAYLOAD_LEN_SIZE       1


// +----------+------+------+-------------------------+----------+--------------+---------+-------------+----------------------+-----------------+
// |    0     |  2   |  3   |            4            |    6     |      8       |    9    |             |                      |                 |
// +----------+------+------+-------------------------+----------+--------------+---------+-------------+----------------------+-----------------+
// | MESG_UID | TYPE | HOPS | NEXT_UID = PREVIOUS_UID | NODE_UID | PAYLOAD_SIZE | PAYLOAD | (EXTRA_UID) | (EXTRA_PAYLOAD_SIZE) | (EXTRA_PAYLOAD) |
// +----------+------+------+-------------------------+----------+--------------+---------+-------------+----------------------+-----------------+


#define HEADER_MESG_UID_OFFSET          0
#define HEADER_TYPE_OFFSET              (HEADER_MESG_UID_OFFSET + MESG_UID_SIZE)
#define HEADER_HOPS_OFFSET              (HEADER_TYPE_OFFSET + MESG_TYPE_SIZE)
#define HEADER_NEXT_UID_OFFSET          (HEADER_HOPS_OFFSET + MESG_HOPS_SIZE)
#define HEADER_PREVIOUS_UID_OFFSET      (HEADER_HOPS_OFFSET + MESG_HOPS_SIZE)
//#define HEADER_PREVIOUS_UID_OFFSET    (HEADER_NEXT_UID_OFFSET + MESG_TYPE_SIZE) // saves 2 bytes
#define HEADER_NODE_UID_OFFSET          (HEADER_PREVIOUS_UID_OFFSET + NODE_UID_SIZE)
#define HEADER_PAYLOAD_LEN_OFFSET       (HEADER_NODE_UID_OFFSET + NODE_UID_SIZE)
#define HEADER_PAYLOAD_OFFSET           (HEADER_PAYLOAD_LEN_OFFSET + MESG_PAYLOAD_LEN_SIZE)
#define HEADER_SIZE                     HEADER_PAYLOAD_OFFSET

#define HEADER_EXTRA_NODE_UID_OFFSET    0
#define HEADER_EXTRA_PAYLOAD_LEN_OFFSET (HEADER_EXTRA_NODE_UID_OFFSET + NODE_UID_SIZE)
#define HEADER_EXTRA_PAYLOAD_OFFSET     (HEADER_EXTRA_PAYLOAD_LEN_OFFSET + MESG_PAYLOAD_LEN_SIZE)

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
        bool begin( void );
        void loop( void );

        bool sendMessage(String str, MsgType_t type);
        bool sendMessage(uint8_t * payload, uint8_t len, MsgType_t type);
        void setMsgReceivedCb(MsgReceivedCb cb);
        void reconfigModem(void);

        bool presetPayload(uint8_t * payload, uint8_t len);
        bool sendPresetPayload( void );
        bool isPresetPayloadSent( void ){
            return presetSent;
        }

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

        uint8_t presetBuffer[MAX_BUF_SIZE];
        uint8_t presetLength = 0;
        unsigned long presetTime;
        bool presetSent = true;
        uint16_t latency;

        CircBuffer floodBuffer;
};

#endif	/* __LORA_MULTI_HOP__ */
