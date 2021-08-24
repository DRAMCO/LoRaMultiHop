#include <Dramco-UNO-Sensors.h>

static volatile unsigned int _wdtSleepTimeMillis;
static volatile unsigned long _millisInDeepSleep;

static bool _keep3V3Active = false;
static uint8_t _accelerometerIntEnabled = false;
static uint8_t _buttonIntEnabled = false;

float _calibration = 1;

static volatile bool _interruptHappened = false;

LIS2DW12 _accelerometer;

unsigned long _millisOffset = 0;

#ifdef DEBUG
void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}
#endif 

static void pciInit(byte pin){
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

static void pciDeinit(){
    PCICR  = 0x00; // disable interrupt for all
}

// ------------------------ DRAMCO UNO LIB ------------------------
void DramcoUnoClass::begin(){

    pinMode(DRAMCO_UNO_3V3_ENABLE_PIN, OUTPUT);
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, LOW);

    pinMode(DRAMCO_UNO_TEMPERATURE_SENSOR_ENABLE_PIN, OUTPUT);
    digitalWrite(DRAMCO_UNO_TEMPERATURE_SENSOR_ENABLE_PIN, LOW);

    pinMode(DRAMCO_UNO_LED_NAME, OUTPUT);

    // Initialize accelerometer int pin
    pinMode(DRAMCO_UNO_ACCELEROMTER_INT_PIN, INPUT);
    digitalWrite(DRAMCO_UNO_ACCELEROMTER_INT_PIN, HIGH);

    pinMode(DRAMCO_UNO_BUTTON_INT_PIN, INPUT);
    digitalWrite(DRAMCO_UNO_BUTTON_INT_PIN, HIGH);

    pinMode(DRAMCO_UNO_SOIL_PIN_EN, OUTPUT);
    digitalWrite(DRAMCO_UNO_SOIL_PIN_EN, LOW);
}

// --- General UTILs ---
void DramcoUnoClass::blink(){
    digitalWrite(DRAMCO_UNO_LED_NAME, HIGH);
    delayMicroseconds(50000);
    digitalWrite(DRAMCO_UNO_LED_NAME, LOW);
}


void DramcoUnoClass::delay(uint32_t d){
    DramcoUnoClass::_sleep(d);
}

unsigned long DramcoUnoClass::millisWithOffset(){
   return millis() + _millisOffset;
}


// --- Sensor readings ---

// - Temperature 
void DramcoUnoClass::calibrateTemperature(){
    _calibration = readTemperatureAccelerometer()/readTemperature();
}

float DramcoUnoClass::readTemperature(){
    digitalWrite(DRAMCO_UNO_TEMPERATURE_SENSOR_ENABLE_PIN, HIGH);
    float average=0;
    analogReference(EXTERNAL);
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    _keep3V3Active = true;
    sleep(500); // Wait for voltage to stabilize
    _keep3V3Active = false;
    for(int i = 0; i < DRAMCO_UNO_TEMPERATURE_AVERAGE; i++){
        float value = (float)(analogRead(DRAMCO_UNO_TEMPERATURE_SENSOR_PIN))*DRAMCO_UNO_TEMPERATURE_CALIBRATE; //Calibrated value of 1024/3.3V (AREF tied to 3.3V reg)
        value = (8.194 - sqrt(67.1416+0.01048*(1324-value)))/(-0.00524)+30;
        average += value;
    }
    average=average/DRAMCO_UNO_TEMPERATURE_AVERAGE;
    return average*_calibration;
}

// - Luminosity
float DramcoUnoClass::readLuminosity(){
    analogReference(EXTERNAL);
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    float value = analogRead(DRAMCO_UNO_LIGHT_SENSOR_PIN)*1.6; // max light value = 160, *255
    if(value <= 255)
        return value;
    else
        return 255.0;
}

// - Acceleration
float DramcoUnoClass::readAccelerationX(){
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    _accelerometer.begin();
    return _accelerometer.readFloatAccelX();
}

float DramcoUnoClass::readAccelerationY(){
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    _accelerometer.begin();
    return _accelerometer.readFloatAccelY();
}

float DramcoUnoClass::readAccelerationZ(){
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    _accelerometer.begin();
    return _accelerometer.readFloatAccelZ();
}

float DramcoUnoClass::readTemperatureAccelerometer(){
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    _accelerometer.begin();
    #ifdef DEBUG
    Serial.println("acc begin end");
    #endif
    return _accelerometer.readTempC();
}

int16_t DramcoUnoClass::readAccelerationXInt(){
    return (int16_t) (this->readAccelerationX()*DRAMCO_UNO_LPP_ACCELEROMETER_MULT);
}

int16_t DramcoUnoClass::readAccelerationYInt(){
    return (int16_t) (this->readAccelerationY()*DRAMCO_UNO_LPP_ACCELEROMETER_MULT);
}

int16_t DramcoUnoClass::readAccelerationZInt(){
    return (int16_t) (this->readAccelerationZ()*DRAMCO_UNO_LPP_ACCELEROMETER_MULT);
}

int16_t DramcoUnoClass::readTemperatureAccelerometerInt(){
    return (int16_t) (this->readTemperatureAccelerometer()*DRAMCO_UNO_LPP_TEMPERATURE_MULT);
}

void DramcoUnoClass::interruptOnMotion(){
    _accelerometer.begin();
    _keep3V3Active = true;
    _accelerometerIntEnabled = true;
    pciInit(9);
    _accelerometer.initWakeUp();
    sleep(-1);
}

void DramcoUnoClass::interruptOnMovement(){
    interruptOnMotion();
}

void DramcoUnoClass::interruptOnShake(){
    _accelerometer.begin();
    _keep3V3Active = true;
    _accelerometerIntEnabled = true;
    pciInit(9);
    _accelerometer.initDoubleTap(4);
}

void DramcoUnoClass::interruptOnFall(){
    _accelerometer.begin();
    _keep3V3Active = true;
    _accelerometerIntEnabled = true;
    pciInit(9);
    _accelerometer.initFreefall();
}

void DramcoUnoClass::interruptOnFreeFall(){
    interruptOnFall();
}

bool DramcoUnoClass::processInterrupt(){
    if(_interruptHappened){
        _interruptHappened = false;
        return true;
    }else{
        return false; 
    }
}

void DramcoUnoClass::delayUntilShake(){
    interruptOnShake();
    sleep(-1);
}

void DramcoUnoClass::delayUntilFall(){
    interruptOnFall();
    sleep(-1);
}

void DramcoUnoClass::delayUntilMotion(){
    interruptOnMotion();
    sleep(-1);
}

void DramcoUnoClass::delayUntilMovement(){
    delayUntilMovement();
}


void DramcoUnoClass::delayUntilFreeFall(){
    delayUntilFall();
}


// - Button
void DramcoUnoClass::interruptOnButtonPress(){
    _buttonIntEnabled = true;
    pciInit(10);
}

void DramcoUnoClass::delayUntilButtonPress(){
    interruptOnButtonPress();
    sleep(-1);
}

// - Soil Moisture
float DramcoUnoClass::readSoilMoisture(){
    digitalWrite(DRAMCO_UNO_SOIL_PIN_EN, HIGH);
    byte ADCSRAoriginal = ADCSRA; 
    ADCSRA = (ADCSRA & B11111000) | 4; 
    float i = 0;
    while(analogRead(DRAMCO_UNO_SOIL_PIN_ANALOG) < 1000 && i < 20000){ // Read until analog value at 1000, count cycles
        i++;
    }
    ADCSRA = ADCSRAoriginal;
    digitalWrite(DRAMCO_UNO_SOIL_PIN_EN, LOW);
    sleep(500); // cap discharge delay
    

    float value = (float(i)) / DRAMCO_UNO_SOIL_DIVIDER;


    if (value > 100)
        return 100;
    else
        return value;
}

float DramcoUnoClass::readSoil(){
    return readSoilMoisture();
}

// --- Sleep ---
unsigned long DramcoUnoClass::sleep(uint32_t d, bool keep3v3active){
    _keep3V3Active = keep3v3active;
    return DramcoUnoClass::sleep(d);
}

unsigned long DramcoUnoClass::sleep(uint32_t d){
    #ifdef DEBUG
    Serial.flush();
    #endif
    return DramcoUnoClass::_sleep(d);
}


unsigned long DramcoUnoClass::_sleep(unsigned long maxWaitTimeMillis) {
    
    if(!_keep3V3Active){
        digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, LOW);
        digitalWrite(DRAMCO_UNO_ACCELEROMTER_INT_PIN, LOW);
        digitalWrite(DRAMCO_UNO_TEMPERATURE_SENSOR_ENABLE_PIN, LOW);
    }
    
    // Backup ports before sleep
    uint8_t d_d = DDRD;
    uint8_t v_d = PORTD;
    
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);


    // Adapted from https://github.com/PRosenb/DeepSleepScheduler/blob/1595995576be62041a1c9db1d51435550ca49c53/DeepSleepScheduler_avr_implementation.h

    // Enable sleep bit with sleep_enable() before the sleep time evaluation because it can happen
    // that the WDT interrupt occurs during sleep time evaluation but before the CPU
    // sleeps. In that case, the WDT interrupt clears the sleep bit and the CPU will not sleep
    // but continue execution immediatelly.

    byte adcsraSave = ADCSRA;

    _millisInDeepSleep = 0;
    while( _millisInDeepSleep <= maxWaitTimeMillis-1){ // -1 for enabling to stop sleeping
        sleep_enable(); // enables the sleep bit, a safety pin
        
        _wdtSleepTimeMillis = DramcoUnoClass::_wdtEnableForSleep(maxWaitTimeMillis-_millisInDeepSleep);
        // _millisInDeepSleep is incremented in in _isrWdt
        DramcoUnoClass::_wdtEnableInterrupt();

        noInterrupts();
        set_sleep_mode(SLEEP_MODE);
        
        ADCSRA = 0;  // disable ADC
        if(maxWaitTimeMillis > 10000 && !_keep3V3Active){
            UCSR0A = 0x00; 
            UCSR0B = 0x00;
            UCSR0C = 0x00;
        }

        // turn off brown-out in software
        #if defined(BODS) && defined(BODSE)
        sleep_bod_disable();
        #endif
        interrupts (); // guarantees next instruction executed
        sleep_cpu(); // here the device is actually put to sleep
        
        // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
    }
    // re-enable ADC
    ADCSRA = adcsraSave;
    
    sleep_disable();
    wdt_reset();
    wdt_disable();

    DDRD = d_d;
    PORTD = v_d;

    Serial.begin(DRAMCO_UNO_SERIAL_BAUDRATE);
    
    digitalWrite(DRAMCO_UNO_ACCELEROMTER_INT_PIN, HIGH);
    digitalWrite(DRAMCO_UNO_3V3_ENABLE_PIN, HIGH);
    _millisOffset += _millisInDeepSleep;
    return _millisInDeepSleep;
}

unsigned long DramcoUnoClass::_wdtEnableForSleep(const unsigned long maxWaitTimeMillis) {
    // From https://github.com/PRosenb/DeepSleepScheduler/blob/1595995576be62041a1c9db1d51435550ca49c53/DeepSleepScheduler_avr_implementation.h#L173
    unsigned long wdtSleepTimeMillis;
    if (maxWaitTimeMillis >= SLEEP_TIME_8S ) {
        wdtSleepTimeMillis = SLEEP_TIME_8S;
        wdt_enable(WDTO_8S);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_4S ) {
        wdtSleepTimeMillis = SLEEP_TIME_4S;
        wdt_enable(WDTO_4S);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_2S ) {
        wdtSleepTimeMillis = SLEEP_TIME_2S;
        wdt_enable(WDTO_2S);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_1S ) {
        wdtSleepTimeMillis = SLEEP_TIME_1S;
        wdt_enable(WDTO_1S);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_500MS ) {
        wdtSleepTimeMillis = SLEEP_TIME_500MS;
        wdt_enable(WDTO_500MS);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_250MS ) {
        wdtSleepTimeMillis = SLEEP_TIME_250MS;
        wdt_enable(WDTO_250MS);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_120MS ) {
        wdtSleepTimeMillis = SLEEP_TIME_120MS;
        wdt_enable(WDTO_120MS);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_60MS ) {
        wdtSleepTimeMillis = SLEEP_TIME_60MS;
        wdt_enable(WDTO_60MS);
    } else if (maxWaitTimeMillis >= SLEEP_TIME_30MS ) {
        wdtSleepTimeMillis = SLEEP_TIME_30MS;
        wdt_enable(WDTO_30MS);
    } else { // maxWaitTimeMs >= 17
        wdtSleepTimeMillis = SLEEP_TIME_15MS;
        wdt_enable(WDTO_15MS);
    }
    return wdtSleepTimeMillis;
}

void DramcoUnoClass::_isrWdt() {
    sleep_disable();
    _millisInDeepSleep += _wdtSleepTimeMillis;
}

void DramcoUnoClass::_wdtEnableInterrupt() { 
    WDTCSR |= (1 << WDCE) | (1 << WDIE);
}

ISR (WDT_vect) {
    DramcoUnoClass::_isrWdt();
}

ISR (PCINT0_vect){ // handle pin change interrupt for D8 to D13 here  
    if(_accelerometerIntEnabled){
        if (!(DRAMCO_UNO_ACCELEROMTER_INT_PORT & _BV(DRAMCO_UNO_ACCELEROMTER_INT_NAME))){ // If pin 9 is low
            DramcoUnoClass::blink();
            pciDeinit();
            _interruptHappened = true;
            _millisInDeepSleep = -1; // Stop WDT sleep
            _keep3V3Active = false; // Accelerometer can shut up now
            _accelerometerIntEnabled = false;
        }
    }
    if(_buttonIntEnabled){
        if (!(DRAMCO_UNO_BUTTON_INT_PORT & _BV(DRAMCO_UNO_BUTTON_INT_NAME))){ // If pin 10 is low
            delay(50);
            if (!(DRAMCO_UNO_BUTTON_INT_PORT & _BV(DRAMCO_UNO_BUTTON_INT_NAME))){ // Debounce
                DramcoUnoClass::blink();
                _interruptHappened = true;
                pciDeinit();
                _millisInDeepSleep = -1; // Stop WDT sleep
                _buttonIntEnabled = false;
            }
        }
    }
    
}

void (*resetptr)( void ) = 0x0000;

void error(uint8_t errorcode){
    int i = 0;
    while(true){
        for(byte i = 0; i < errorcode; i++){
            digitalWrite(DRAMCO_UNO_LED_NAME, HIGH);
            delay(100);
            digitalWrite(DRAMCO_UNO_LED_NAME, LOW);
            delay(100);
        }
        delay(500);
        i++;
        if(i > 240){
            resetptr();
        }
    }
}
