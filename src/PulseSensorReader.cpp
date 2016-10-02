#include <Ticker.h>
#include "PulseSensor.h"

//Set USE_EXTERNAL_ADC to 1 if you have ESPectro Base (which has external ADC), otherwise set 0 to use INTERNAL ESP8266 ADC
#define USE_EXTERNAL_ADC  1

//Set this to 1 to wait for callback when BPM available, or set to 0 to poll the last read BPM and display using serial monitor or desktop app
#define USE_CALLBACK      1

#define USE_ESPECTRO_BOARD 1

#if USE_ESPECTRO_BOARD
#include "ESPectro.h"
ESPectro board;
#endif

#if USE_EXTERNAL_ADC
#include <ESPectroBase.h>
ESPectroBase base;
#endif

#if !USE_CALLBACK
// Set to 'false' by default to send the data via serial and display using desktop app.  Set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse
static boolean serialVisual = false;
#endif

//Ticker ticker;
PulseSensor sensor;

void readADC() {
#if USE_EXTERNAL_ADC
    int raw = base.analogRead(3);
#else
    int raw = analogRead(0);
#endif
    Serial.printf("Raw: %d\n", raw);
}

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    while(!Serial);

#if USE_EXTERNAL_ADC
    base.beginADC();
#endif

//    ticker.attach_ms(200, readADC); //-> just for testig

#if USE_CALLBACK
    sensor.onBMPAvailable([](int BPM) {
        //Serial.printf("BPM: %d\n", BPM);

        if (BPM > BPM+20 || BPM < BPM-20|| BPM > 160 || BPM < 54) {
            Serial.printf("[Not Valid] BPM: %d\n", BPM);
        }
        else {
            Serial.printf("[Valid] BPM: %d\n", BPM);
        }
    });

//    sensor.onPulseDetectionCallback([](bool detected) {
//#if USE_ESPECTRO_BOARD
//        if (detected) {
//            board.turnOnLED();
//        }
//        else {
//            board.turnOffLED();
//        }
//#endif
//    });

#endif

    //set callback to actually read the raw value from ADC
    sensor.onReadingRawSignal([](int &raw) {

#if USE_EXTERNAL_ADC
        raw = base.analogRead(3);
#else
        raw = analogRead(0);
#endif

        //Serial.printf("Raw 1: %d\n", raw);
    });

    sensor.start();
}

#if !USE_CALLBACK
//  Code to Make the Serial Monitor Visualizer Work
void arduinoSerialMonitorVisual(char symbol, int data ){
    const int sensorMin = 0;      // sensor minimum, discovered through experiment
    const int sensorMax = 1024;    // sensor maximum, discovered through experiment

    int sensorReading = data;
    // map the sensor range to a range of 12 options:
    int range = map(sensorReading, sensorMin, sensorMax, 0, 11);

    // do something different depending on the
    // range value:
    switch (range) {
        case 0:
            Serial.println("");     /////ASCII Art Madness
            break;
        case 1:
            Serial.println("---");
            break;
        case 2:
            Serial.println("------");
            break;
        case 3:
            Serial.println("---------");
            break;
        case 4:
            Serial.println("------------");
            break;
        case 5:
            Serial.println("--------------|-");
            break;
        case 6:
            Serial.println("--------------|---");
            break;
        case 7:
            Serial.println("--------------|-------");
            break;
        case 8:
            Serial.println("--------------|----------");
            break;
        case 9:
            Serial.println("--------------|----------------");
            break;
        case 10:
            Serial.println("--------------|-------------------");
            break;
        case 11:
            Serial.println("--------------|-----------------------");
            break;

    }
}

void sendDataToSerial(char symbol, int data ){
    Serial.print(symbol);
    Serial.println(data);
}


void serialOutput(){   // Decide How To Output Serial.
    if (serialVisual == true){
        arduinoSerialMonitorVisual('-', sensor.doReadRawSignal());   // goes to function that makes Serial Monitor Visualizer
    } else{
        sendDataToSerial('S', sensor.doReadRawSignal());     // goes to sendDataToSerial function
    }
}

void serialOutputWhenBeatHappens(){
    if (serialVisual == true){            //  Code to Make the Serial Monitor Visualizer Work
        Serial.print("*** Heart-Beat Happened *** ");  //ASCII Art Madness
        Serial.print("BPM: ");
        Serial.print(sensor.getLastBPM());
        Serial.print("  ");
    } else{
        sendDataToSerial('B', sensor.getLastBPM());   // send heart rate with a 'B' prefix
        sendDataToSerial('Q', sensor.getLastIBI());   // send time between beats with a 'Q' prefix
    }
}
#endif

// the loop function runs over and over again forever
void loop() {

#if !USE_CALLBACK
    serialOutput();

    if (sensor.isBeatDetected()) {
        serialOutputWhenBeatHappens();
    }

    delay(20);                             //  take a break
#endif

}
