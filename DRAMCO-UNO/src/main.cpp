// Example sketch showing how to create a simple messageing client
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_server
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with the RFM95W 

#include <Arduino.h>
#include "LoRaMultiHop.h"

#define PIN_BUTTON        10
#define PIN_LED           4

bool newMsg = false;
uint8_t payloadBuf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t payloadLen = 0;

bool prevButtonState = true;
bool btnPressed = false;

unsigned long prevTT = 0;

LoRaMultiHop multihop;

ISR (PCINT0_vect){ //  pin change interrupt for D8 to D13
  PCIFR  |= bit (digitalPinToPCICRbit(PIN_BUTTON)); // clear any outstanding interrupt

  bool currentState = (bool)digitalRead(PIN_BUTTON);
  if(!btnPressed && prevButtonState){
    if(!currentState){
      btnPressed = true;
    }
  }
  prevButtonState = currentState;
}


// callback function for when a new message is received
void msgReceived(uint8_t * payload, uint8_t plen){
  Serial.print("Payload: ");
  for(uint8_t i=0; i<plen; i++){
    if(payload[i] < 16){
        Serial.print("0");
    }
    Serial.print(payload[i], HEX);
    Serial.print(" ");
    payloadBuf[i] = payload[i];
  }
  Serial.println();

  payloadLen = plen;
  newMsg = true;
}


void setup(){
  // Start serial connection for printing debug information
  Serial.begin(115200);

  // don't wait for serial connection to be established, but give the developer some time to hook up the console
  delay(5000);
  Serial.println(F("LoRa PTP multihop demo."));

  Serial.print(F("Initializing I/O..."));
  // button pin
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  *digitalPinToPCMSK(PIN_BUTTON) |= bit (digitalPinToPCMSKbit(PIN_BUTTON));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(PIN_BUTTON)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(PIN_BUTTON)); // enable interrupt for the group

  // led
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  Serial.println(F(" done."));

  Serial.println(F("Starting lora multihop..."));
  if(!multihop.begin()){
    while(true){
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
      delay(100);
    }
  }
  multihop.setMsgReceivedCb(&msgReceived);
  Serial.println(F("done."));

  Serial.println(F("Setup complete."));
  Serial.println(F("Press the button to send a message."));
}

void loop(){
  bool autoToggle = false;
#ifdef TOGGLE_TIME
  if((millis() - prevTT) > TOGGLE_TIME){
    prevTT = millis();
    autoToggle = true;
  }
#endif

  // button press initiates a "send message"
  if(btnPressed || autoToggle){
    uint8_t lv = !digitalRead(PIN_LED);
    digitalWrite(PIN_LED, lv);
    Serial.print(F("Broadcasting new LED value: "));
    Serial.println(lv);

    //multihop.sendMessage(&lv, 1);
    String test = "testtest";
    multihop.sendMessage(test);

    btnPressed = false;
  }

  multihop.loop();

  if(newMsg){
    digitalWrite(PIN_LED, payloadBuf[0]);
    newMsg = false;
  }
}


// go low-power:
// https://lowpowerlab.com/forum/low-power-techniques/any-success-with-lora-low-power-listening/