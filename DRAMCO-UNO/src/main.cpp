// Example for flooding mesh network using CAD
// Extra low power: disable 3v3 regulator on Dramco Uno => EXTRA CAPS NEEDED ON VCC RF95 (100nF // 10nF)

#include <Arduino.h>
#include "LoRaMultiHop.h"

#include "Dramco-UNO-Sensors.h"

#define PIN_BUTTON        10
#define PIN_LED           4

#define DRAMCO_UNO_LPP_DIGITAL_INPUT_MULT   1
#define DRAMCO_UNO_LPP_ANALOG_INPUT_MULT    100
#define DRAMCO_UNO_LPP_GENERIC_SENSOR_MULT  1
#define DRAMCO_UNO_LPP_LUMINOSITY_MULT      1
#define DRAMCO_UNO_LPP_TEMPERATURE_MULT     10
#define DRAMCO_UNO_LPP_ACCELEROMETER_MULT   1000
#define DRAMCO_UNO_LPP_PERCENTAGE_MULT      1



bool newMsg = false;
bool measureNow = false;

uint8_t payloadBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t payloadLen = 0;

uint16_t ctr = 0x00;

unsigned long prevTT = 0;
unsigned long prevMeasurement = 0;

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
void msgReceived(uint8_t * packet, uint8_t plen){
#ifdef DEBUG
  //Serial.begin(115200);
  Serial.print("Packet: ");
#endif
  for(uint8_t i=0; i<plen; i++){
#ifdef DEBUG
    if(packet[i] < 16){
        Serial.print('0');
    }
    Serial.print(packet[i], HEX);
    Serial.print(' ');
#endif
    payloadBuf[i] = packet[i];
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
  prevTT = DramcoUno.millisWithOffset() + BEACON_INTERVAL;
}

void loop(){
  multihop.loop();

  // --------------- COMPILE FOR GATEWAY ----------------
#ifdef COMPILE_FOR_GATEWAY
  bool autoToggle = false;
#ifdef BEACON_INTERVAL
  if((DramcoUno.millisWithOffset() - prevTT) > BEACON_INTERVAL){
    prevTT = DramcoUno.millisWithOffset();
    autoToggle = true;
    Serial.print(F("Own ID: "));
    Serial.println(NODE_UID, HEX);
  }
  if(DramcoUno.processInterrupt() || autoToggle){
    multihop.sendMessage("UUUU", MESG_ROUTE_DISCOVERY);
    DramcoUno.interruptOnButtonPress();
  }
#endif
#endif

  // --------------- COMPILE FOR SENSOR ----------------
#ifdef COMPILE_FOR_SENSOR
  if((DramcoUno.millisWithOffset() - prevMeasurement) > MEASURE_INTERVAL){
    prevMeasurement = DramcoUno.millisWithOffset();
    measureNow = true;
  }

  // button press initiates a "send message"
  if(DramcoUno.processInterrupt() || measureNow){
    measureNow = false;

#ifdef DEBUG
    Serial.print(F("Own ID: "));
    Serial.println(NODE_UID, HEX);
    Serial.println(F("Composing message"));
    
#endif

    uint8_t data[15];
    uint8_t i = 0;

    //uint16_t vx = DramcoUno.readAccelerationXInt();

    //uint16_t vy = DramcoUno.readAccelerationYInt();
    //uint16_t vz = DramcoUno.readAccelerationZInt();
    //uint16_t vt = DramcoUno.readTemperatureAccelerometerInt();
    uint16_t vt = ctr++; // use counter instead
    //uint8_t vl = DramcoUno.readLuminosity();

    /*data[i++] = vx >> 8;
    data[i++] = vx;
    data[i++] = vy >> 8;
    data[i++] = vy;
    data[i++] = vz >> 8;
    data[i++] = vz;
    */data[i++] = (uint8_t)(vt >> 8);
    data[i++] = (uint8_t)vt;
    //data[i++] = vl;

    // Add sensor data to preset payload, multihop will take care when it will be sent
    // (within PRESET_MAX_LATENCY)
    multihop.prepareOwnDataForAggregation(data, 12);
    
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
