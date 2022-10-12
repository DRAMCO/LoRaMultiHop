# Multihop symbols and definitions

## Defines in LoraMultiHop.h

| New							    | Old								| Description																| 
| ----------------------------------| --------------------------------- | ------------------------------------------------------------------------- |
| `PREAMBLE_DURATION`			    | `PREAMBLE_DURATION`				| Time in ms for how long the preamble is									| 
|									|									| 																			| 
| `CAD_STABILIZE`					| `CAD_STABILIZE`              		| Timedelay before executing CAD, after deep sleep when power down: to stabilize power supply | 
| `CAD_DELAY`		     			| `CAD_DELAY_MIN`             		| Nominal delay between CADs, defined by PREAMBLE_DURATION					| 
| `CAD_DELAY_RANDOM`				| `CAD_DELAY_MAX`					| Random window around CAD delay, defined by PREAMBLE_DURATION				| 
|									| 									| 																			| 
| `AGGREGATION_TIMER_MIN`			| `PRESET_MIN_LATENCY`         		| Aggregation timer = timer to wait for incoming data (own data or rx) to append to message in queu. To imrove latency, a stepup/stepdown mechanism is put in place (depending on how much extra data is coming in): this is the min value of that. | 
| `AGGREGATION_TIMER_MAX` 			| `PRESET_MAX_LATENCY`             	| Max value of the aggregation timer.										| 
| `AGGREGATION_TIMER_RANDOM`		| `PRESET_MAX_LATENCY_RAND_WINDOW` 	| Size of the random window around the current value of the aggregation timer. | 
| `AGGREGATION_TIMER_UPSTEP`		| `PRESET_LATENCY_UP_STEP`         	| Added to the current value of the aggregation timer, when more latency is tolerated, or more packets are coming in (efficient appending/aggregating). | 
| `AGGREGATION_TIMER_DOWNSTEP`		| `PRESET_LATENCY_DOWN_STEP`       	| Subtracted to the current value of the aggregation timer, when less latency is prefered, or almost no packets are coming in (added latency amounts to nothing: no use in waiting for packets if none are coming in). | 
|									| 									| 																			| 
| /									| `FORWARD_BACKOFF_MIN`				| Not in use 																| 
| /									| `FORWARD_BACKOFF_MAX `       		| Not in use 																| 
|									|									| 																			| 
| `COLLISION_DELAY`	            	| `TX_BACKOFF_MIN`					| When pending tx (any packet), and a CAD is detected just before, postpone the message by this amount. This is the nominal value. Postpone at least 1 preamble. | 
| `COLLISION_DELAY_RANDOM`	 		| `TX_BACKOFF_MAX`					| Random window around time delay after CAD									| 
|									|									| 																			| 
| `AGGREGATION_BUFFER_SIZE`			| `MAX_PRESET_BUFFER_SIZE`			| Buffer size for one aggregation: own payload or forward					| 
| `TX_BUFFER_SIZE` 					| `MAX_FORWARD_BUFFER_SIZE`			| Buffer size for total package/messag: multiple AGGREGATION_BUFFER_SIZE + header	| 

## Methods

| New							    											| Old																			| Description																| 
| ----------------------------------------------------------------------------- | ----------------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| `void begin( void );`															| `void begin( void );`															| Setup of the lib. 														| 
| `void loop( void );`															| `void loop( void );`															| Loop of the lib, call this in Arduino `loop`, protocol handlers start here. | 
| `void sendMessage(String str, MsgType_t type);`								| `void sendMessage(String str, MsgType_t type);`								| Higher level function to send message. Overloaded function for String objects. |
| `bool sendMessage(uint8_t * payload, uint8_t len, MsgType_t type);`			| `bool sendMessage(uint8_t * payload, uint8_t len, MsgType_t type);`			| Higher level function to send message. Overloaded function for char arrays. | 
| `void setMsgReceivedCb(MsgReceivedCb cb);`									| `void setMsgReceivedCb(MsgReceivedCb cb);`									| Set callback to return to application when a message (for you, predominantly for gateway) was received. | 
| `void reconfigModem(void);`													| `void reconfigModem(void);`													| Set hardware (rfm95) to right config for LoRa Multihop.					| 
| 																				| 																				| 																			| 
| `bool presetPayload(uint8_t * payload, uint8_t len);`							| `void bool presetPayload(uint8_t * payload, uint8_t len);`					| Used to prepare a buffer to get the sensor's own data ready for sending. The actual sending will happen when the aggregation timer decides it is time. | 
| `bool presetForwardPayload(uint8_t * payload, uint8_t len);`					| `bool presetForwardPayload(uint8_t * payload, uint8_t len);`					| Used to prepare a buffer to get the rx message ready for forwarding. The actual sending will happen when the aggregation timer decides it is time. | 
| `bool sendPresetPayload( void );`												| `bool sendPresetPayload( void );`												| Send prepared buffer (via txMessage) | 
| `bool isPresetPayloadSent( void );`											| `bool isPresetPayloadSent( void );`											| Returns a message is waiting to be sent in buffer (waiting for aggregation). | 
|																				|																				| 																			|
| `void updateHeader(uint8_t * buf, uint8_t len);`								| `void updateHeader(uint8_t * buf, uint8_t len);`								| Updates the header when from an incoming package to get it ready for forwarding | 
| `void initHeader(Msg_UID_t uid);`												| `void initHeader(Msg_UID_t uid);`												| Not in use, was only in h file. 											| 
| `void txMessage(uint8_t len);`												| `void txMessage(uint8_t len);`												| Effectively send message via physical layer| 
| `bool waitCADDone( void );`													| `bool waitCADDone( void );`													| Blocking wait to wait for when CAD mechanism is done| 
| `bool waitRXAvailable(uint16_t timeout);`										| `bool waitRXAvailable(uint16_t timeout);`										| Blocking wait to wait on rx message. Used when CAD is succesful to wait on start of rx. | 
| `bool handleMessage(uint8_t * buf, uint8_t len);`								| `bool handleMessage(uint8_t * buf, uint8_t len);`								| Handle incoming message, all rx messages start here. | 
| `bool forwardMessage(uint8_t * buf, uint8_t len);`							| `bool forwardMessage(uint8_t * buf, uint8_t len);`							| Send incoming message to forward mechanism, will forward to `presetpayloadforward` for aggregation mechanism. | 
| `Node_UID_t getNodeUidFromBuffer(uint8_t * buf, NodeID_t which=SOURCE_NODE);`	| `Node_UID_t getNodeUidFromBuffer(uint8_t * buf, NodeID_t which=SOURCE_NODE);`	| Extracts nodeuid from buffer. | 
| `Msg_UID_t getMsgUidFromBuffer(uint8_t * buf);`								| `Msg_UID_t getMsgUidFromBuffer(uint8_t * buf);`								| Extracts message id from buffer. | 

## Private vars

| New							   							| Old														| Description																| 
| --------------------------------------------------------- | --------------------------------------------------------- | ------------------------------------------------------------------------- |
| `bool txPending;`				   		 					| `bool txPending;`											| | 
| `unsigned long txTime;`			    					| `unsigned long txTime;`									| | 
| `uint8_t txLen;`				    						| `uint8_t txLen;`											| | 
| `uint8_t txBuf[RH_RF95_MAX_MESSAGE_LEN];`					| `uint8_t txBuf[RH_RF95_MAX_MESSAGE_LEN];`					| | 
| `uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];`					| `uint8_t rxBuf[RH_RF95_MAX_MESSAGE_LEN];`					| | 
| `NodeType_t type;`				    					| `NodeType_t type;`										| | 
| `Node_UID_t uid;`				    						| `Node_UID_t uid;`											| | 
| `RouteToGatewayInfo_t shortestRoute;`  					| `RouteToGatewayInfo_t shortestRoute;`						| | 
| `uint8_t presetOwnData[MAX_PRESET_BUFFER_SIZE];`			| `uint8_t presetOwnData[MAX_PRESET_BUFFER_SIZE];`			| | 
| `uint8_t presetForwardedData[MAX_FORWARD_BUFFER_SIZE];`	| `uint8_t presetForwardedData[MAX_FORWARD_BUFFER_SIZE];`	| | 
| `uint8_t presetLength = 0;`		    					| `uint8_t presetLength = 0;`								| | 
| `uint8_t presetForwardedLength = 0;`   					| `uint8_t presetForwardedLength = 0;`						| | 
| `unsigned long presetTime;`			   					| `unsigned long presetTime;`								| | 
| `bool presetSent = true;`			   						| `bool presetSent = true;`									| | 
| `uint16_t latency;`										| `uint16_t latency;`										| | 
| `CircBuffer floodBuffer;`			   						| `CircBuffer floodBuffer;`									| | 
