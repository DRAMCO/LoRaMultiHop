#ifndef __LORA_MULTI_HOP__
#define	__LORA_MULTI_HOP__

#include <Arduino.h>
#include <RH_RF95.h>
#include "CircBuffer.h"


#define PREAMBLE_DURATION                   250 // In ms (for now: don't increase)

#define CAD_STABILIZE                       30 // In ms
#define CAD_DELAY                           PREAMBLE_DURATION*0.625
#define CAD_DELAY_RANDOM                    PREAMBLE_DURATION*0.125
#define CAD_DELAY_MIN                       CAD_DELAY - CAD_DELAY_RANDOM
#define CAD_DELAY_MAX                       CAD_DELAY + CAD_DELAY_RANDOM

#define HEADER_MESG_UID_OFFSET          0
#define HEADER_TYPE_OFFSET              (HEADER_MESG_UID_OFFSET + MESG_UID_SIZE)
#define HEADER_HOPS_OFFSET              (HEADER_TYPE_OFFSET + MESG_TYPE_SIZE)
#define HEADER_LQI_OFFSET               (HEADER_HOPS_OFFSET + MESG_HOPS_SIZE)
#define HEADER_NEXT_UID_OFFSET          (HEADER_LQI_OFFSET + MESG_LQI_SIZE)     // 3 + 1 = 4
#define HEADER_PREVIOUS_UID_OFFSET      (HEADER_LQI_OFFSET + MESG_LQI_SIZE)
//#define HEADER_PREVIOUS_UID_OFFSET    (HEADER_NEXT_UID_OFFSET + MESG_TYPE_SIZE) // saves 2 bytes
#define HEADER_PAYLOAD_OFFSET           (HEADER_PREVIOUS_UID_OFFSET + NODE_UID_SIZE)
#define HEADER_SIZE                     (HEADER_PAYLOAD_OFFSET)

// #define AGGREGATION_TIMER_MIN               (MEASURE_INTERVAL/2)-10000L // In ms
// #define AGGREGATION_TIMER_MAX               (MEASURE_INTERVAL/2)+10000L // In ms
// #define AGGREGATION_TIMER_RANDOM            1000 // Random window around PRESET_MAX_LATENCY
// #define AGGREGATION_TIMER_UPSTEP            10000
// #define AGGREGATION_TIMER_DOWNSTEP          0

#ifndef AGGREGATION_TIMER_NOMINAL
#define AGGREGATION_TIMER_NOMINAL           MEASURE_INTERVAL/2+60000UL// in ms
#endif
#define AGGREGATION_TIMER_MIN               0 // In ms
#define AGGREGATION_TIMER_MAX               AGGREGATION_TIMER_NOMINAL+60000 // In ms
#define AGGREGATION_TIMER_RANDOM            90000 // Random window around PRESET_MAX_LATENCY
#define AGGREGATION_TIMER_UPSTEP            60000
#define AGGREGATION_TIMER_DOWNSTEP          30000

#define COLLISION_DELAY                     2*PREAMBLE_DURATION       // Backoff if CAD detected when wanting to send
#define COLLISION_DELAY_RANDOM              PREAMBLE_DURATION
#define COLLISION_DELAY_MIN                 COLLISION_DELAY-COLLISION_DELAY_RANDOM/2       // Backoff if CAD detected when wanting to send
#define COLLISION_DELAY_MAX                 COLLISION_DELAY-COLLISION_DELAY_RANDOM/2

#define OWN_DATA_BUFFER_SIZE                24
#define FORWARDED_BUFFER_SIZE               96
#define ROUTING_EXTRA_SIZE                  30
#define AGGREGATION_BUFFER_SIZE             (OWN_DATA_BUFFER_SIZE + FORWARDED_BUFFER_SIZE)    // Max preset buffer size 
#define TX_BUFFER_SIZE                      (HEADER_SIZE + AGGREGATION_BUFFER_SIZE + ROUTING_EXTRA_SIZE + 2)     // Max preset buffer size 

#define PAYLOAD_TX_THRESHOLD_START          79 // Good for 8 packages and 7 byte header
#define PAYLOAD_TX_THRESHOLD_MINUS_PER_HOP  3

#define PIN_ENABLE_3V3                      8
#define PIN_MODEM_SS                        6
#define PIN_MODEM_INT                       2

typedef uint32_t BaseType_t;
typedef uint16_t Msg_UID_t;
typedef uint8_t Node_UID_t;
typedef uint8_t Msg_Type_t;
typedef int16_t Msg_LQI_t;

#define GATEWAY_UID                             0x0000
#define BROADCAST_UID                           0xFFFF

#define NODE_UID_SIZE                           sizeof(Node_UID_t)
#define MESG_UID_SIZE                           sizeof(Msg_UID_t)
#define MESG_TYPE_SIZE                          sizeof(Msg_Type_t)
#define MESG_HOPS_SIZE                          1
#define MESG_PAYLOAD_OWN_DATA_LEN_SIZE          1
#define MESG_PAYLOAD_FORWARDED_DATA_LEN_SIZE    1
#define MESG_LQI_SIZE                           sizeof(Msg_LQI_t)

// +----------+------+------+-------------+-------------------------+----------+------------------+------------------------+---------+-------------+----------------------+-----------------+
// |    0     |  2   |  3   |      4      |            6            |    8     |        9         |           10           |   11    |             |                      |                 |
// +----------+------+------+-------------+-------------------------+----------+------------------+------------------------+---------+-------------+----------------------+-----------------+
// | MESG_UID | TYPE | HOPS | CUMMUL. LQI | NEXT_UID = PREVIOUS_UID | NODE_UID | PAYLOAD_SIZE_OWN | PAYLOAD_SIZE_FORWARDED | PAYLOAD | (EXTRA_UID) | (EXTRA_PAYLOAD_SIZE) | (EXTRA_PAYLOAD) |
// +----------+------+------+-------------+-------------------------+----------+------------------+------------------------+---------+----------------------+-----------------+
//                                                                  | Payload starts here



#define PAYLOAD_NODE_UID_OFFSET         0
#define PAYLOAD_OWN_DATA_LEN_OFFSET             (PAYLOAD_NODE_UID_OFFSET + NODE_UID_SIZE)
#define PAYLOAD_FORWARDED_DATA_LEN_OFFSET       (PAYLOAD_OWN_DATA_LEN_OFFSET + MESG_PAYLOAD_OWN_DATA_LEN_SIZE)
#define PAYLOAD_DATA_OFFSET                     (PAYLOAD_FORWARDED_DATA_LEN_OFFSET + MESG_PAYLOAD_FORWARDED_DATA_LEN_SIZE)

#define MESSAGE_FLOODBUFFER_SIZE         8
#define MAX_NUMBER_OF_NEIGHBOURS         8

typedef enum msgTypes{
    MESG_ROUTE_DISCOVERY = 0x01,
    MESG_BROADCAST = 0x02,
    MESG_ROUTED = 0x03
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
    int8_t lastRssi; 
    int8_t lastSnr;
    int16_t cumLqi;
    bool isBest;
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
        bool sendMessage(MsgType_t type); // sends any pending data immediately
        void setMsgReceivedCb(MsgReceivedCb cb);
        void reconfigModem(void);

        bool prepareOwnDataForAggregation(uint8_t * payload, uint8_t len);

    private:
        bool handleAnyRxMessage(uint8_t * buf, uint8_t len);
        bool handleRouteDiscoveryMessage(uint8_t * buf, uint8_t len);
        bool handleRoutedMessage(uint8_t * buf, uint8_t len);
        bool handleBroadcastMessage(uint8_t * buf, uint8_t len);
        bool sendAggregatedMessage( void );

        void updateRouteDiscoveryHeader(uint8_t * buf, uint8_t len);
        void initHeader(Msg_UID_t uid);
        void txMessage(uint8_t len);

        bool waitCADDone( void );
        bool waitRXAvailable(uint16_t timeout);
        bool isPresetPayloadSent( void ){
            return presetSent;
        }

        bool checkFloodBufferForMessage(uint8_t * buf, uint8_t len);
        bool addMessageToFloodBuffer(uint8_t * buf, uint8_t len);
        bool addMessageToFloodBuffer(MsgInfo_t * mInfo);

        bool forwardRouteDiscoveryMessage(uint8_t * buf, uint8_t len);
        bool prepareRxDataForAggregation(uint8_t * payload, uint8_t len);

        //Node_UID_t getNodeUidFromBuffer(uint8_t * buf, NodeID_t which=SOURCE_NODE);
        //Msg_UID_t getMsgUidFromBuffer(uint8_t * buf);

        bool getFieldFromBuffer(BaseType_t * field, uint8_t * buf, uint8_t fieldOffset, size_t size);
        bool setFieldInBuffer(BaseType_t field, uint8_t * buf, uint8_t fieldOffset, size_t size);
 
        bool updateNeighbours(RouteToGatewayInfo_t &neighbour);
        void updateRouteToGateway();
        void printNeighbours();
        Msg_LQI_t computeCummulativeLQI(Msg_LQI_t previousCummulativeLQI, int8_t snr);

        bool txPending;
        unsigned long txTime;
        uint8_t txLen;
        uint8_t txBuf[TX_BUFFER_SIZE];
        uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];

        NodeType_t type;
        Node_UID_t uid;

        RouteToGatewayInfo_t * bestRoute = NULL;
        RouteToGatewayInfo_t * neighbours = NULL;
        uint8_t numberOfNeighbours = 0;

        uint8_t ownDataBuffer[OWN_DATA_BUFFER_SIZE];
        uint8_t forwardedDataBuffer[FORWARDED_BUFFER_SIZE];
        uint8_t forwardedDataBufferOverflow[FORWARDED_BUFFER_SIZE];
        uint8_t ownDataBufferLength = 0;
        uint8_t forwardedDataBufferLength = 0;
        uint8_t forwardedDataBufferOverflowLength = 0;
        bool forwardedDataOverflowInUse = false;
        unsigned long presetTime;
        bool presetSent = true;
        bool sendOwnData = true;
        uint32_t latency;

        CircBuffer floodBuffer;
};

#endif	/* __LORA_MULTI_HOP__ */
