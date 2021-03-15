#ifndef __LORA_MULTI_HOP__
#define	__LORA_MULTI_HOP__

#include <Arduino.h>
#include <RH_RF95.h>
#include "CircBuffer.h"

#define MAX_BUF_SIZE 16

#define PIN_ENABLE_3V3    8
#define PIN_MODEM_SS      6
#define PIN_MODEM_INT     2

typedef struct msg{
  MsgInfo_t info;
  uint8_t hops;
  uint8_t payloadLength;
  uint8_t * payload;
} Msg_t;

typedef void (*MsgReceivedCb)(uint8_t *, uint8_t);

class LoRaMultiHop{
	public:
        LoRaMultiHop();
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

        bool forwardMessage(uint8_t * buf, uint8_t len); // make private ? ( and forward automatically)

        bool txPending;
        unsigned long txTime;
        uint8_t txLen;

        uint16_t uid;
        uint8_t txBuf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];
        
        CircBuffer floodBuffer;
        

};

#endif	/* __LORA_MULTI_HOP__ */

