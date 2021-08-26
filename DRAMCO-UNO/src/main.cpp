// Example for flooding mesh network using CAD
// Extra low power: disable 3v3 regulator on Dramco Uno => EXTRA CAPS NEEDED ON VCC RF95 (100nF // 10nF)

#include <Arduino.h>
#include "LoRaMultiHop.h"

#include "Dramco-UNO-Sensors.h"

#define PIN_BUTTON        10
#define PIN_LED           4

#define DRAMCO_UNO_LPP_DIGITAL_INPUT_MULT          1
#define DRAMCO_UNO_LPP_ANALOG_INPUT_MULT           100
#define DRAMCO_UNO_LPP_GENERIC_SENSOR_MULT         1
#define DRAMCO_UNO_LPP_LUMINOSITY_MULT             1
#define DRAMCO_UNO_LPP_TEMPERATURE_MULT            10
#define DRAMCO_UNO_LPP_ACCELEROMETER_MULT          1000
#define DRAMCO_UNO_LPP_PERCENTAGE_MULT         	   1

#define TOGGLE_TIME 10000

bool newMsg = false;
uint8_t payloadBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t payloadLen = 0;

unsigned long prevTT = 0;

#ifdef COMPILE_FOR_GATEWAY
LoRaMultiHop multihop(GATEWAY);
#else
#ifdef COMPILE_FOR_SENSOR
    LoRaMultiHop multihop(SENSOR);
#else
  #error "No COMPILE_FOR specified"
#endif
#endif

// callback function for when a new message is received
void msgReceived(uint8_t * payload, uint8_t plen){
#ifdef DEBUG
  //Serial.begin(115200);
  Serial.print("Payload: ");
#endif
  for(uint8_t i=0; i<plen; i++){
#ifdef DEBUG
    if(payload[i] < 16){
        Serial.print("0");
    }
    Serial.print(payload[i], HEX);
    Serial.print(" ");
#endif
    payloadBuf[i] = payload[i];
  }
#ifdef DEBUG
  Serial.println();
#endif

  payloadLen = plen;
  newMsg = true;
}


void setup(){
  // Start serial connection for printing debug information
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(F("LoRa PTP multihop demo."));
#endif

#ifdef DEBUG
  Serial.println(F("Starting Dramco Uno firmware..."));
#endif
  DramcoUno.begin();

Serial.println("Random number:");
Serial.println(DramcoUno.random());
#ifdef DEBUG
  Serial.println(F("Starting lora multihop..."));
#endif
  if(!multihop.begin()){
    while(true){
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
      delay(100);
    }
  }
  multihop.setMsgReceivedCb(&msgReceived);
#ifdef DEBUG
  Serial.println(F("done."));
#endif

#ifdef DEBUG
  Serial.println(F("Setting button press interrupt."));
#endif
DramcoUno.interruptOnButtonPress();

#ifdef DEBUG
  Serial.println(F("Setup complete."));
  Serial.println(F("Press the button to send a message."));
#endif
  
}

void loop(){
#ifdef COMPILE_FOR_GATEWAY
  bool autoToggle = false;
#ifdef TOGGLE_TIME
  if((DramcoUno.millisWithOffset() - prevTT) > TOGGLE_TIME){
    prevTT = DramcoUno.millisWithOffset();
    autoToggle = true;
  }
#endif
#endif

  multihop.loop();

#ifdef COMPILE_FOR_GATEWAY
  // timer initiates a "send message"
  if(autoToggle){
#endif
#ifdef COMPILE_FOR_SENSOR
  // button press initiates a "send message"
  if(DramcoUno.processInterrupt()){
#endif
#ifdef DEBUG
    Serial.println(F("Composing message"));
#endif

    /*uint8_t data[15];
    uint8_t i = 0;

    uint16_t vx = DramcoUno.readAccelerationXInt();

    uint16_t vy = DramcoUno.readAccelerationYInt();
    uint16_t vz = DramcoUno.readAccelerationZInt();
    uint16_t vt = DramcoUno.readTemperatureAccelerometerInt();
    uint8_t vl = DramcoUno.readLuminosity();

    data[i++] = vx >> 8;
    data[i++] = vx;
    data[i++] = vy >> 8;
    data[i++] = vy;
    data[i++] = vz >> 8;
    data[i++] = vz;
    data[i++] = vt >> 8;
    data[i++] = vt;
    data[i++] = vl;*/

    DramcoUno.blink();

#ifdef COMPILE_FOR_GATEWAY
    multihop.sendMessage("UUUU", GATEWAY_BEACON);

    //DramcoUno.interruptOnButtonPress();
  }
#endif

#ifdef COMPILE_FOR_SENSOR
    multihop.sendMessage("YYYY", DATA_ROUTED);

    DramcoUno.interruptOnButtonPress();
  }
#endif

  if(newMsg){
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    newMsg = false;
  }
  digitalWrite(DRAMCO_UNO_LED_NAME, LOW);
}


// go low-power:
// https://lowpowerlab.com/forum/low-power-techniques/any-success-with-lora-low-power-listening/
