#include <Ticker.h>
#include "PulseSensor.h"

//Set USE_EXTERNAL_ADC to 1 if you have ESPectro Base (which has external ADC), otherwise set 0 to use INTERNAL ESP8266 ADC
#define USE_EXTERNAL_ADC    1

//Set this to 1 to wait for callback when BPM available, or set to 0 to poll the last read BPM and display using serial monitor or desktop app
#define USE_CALLBACK        1

//Set to 1 if you're using ESPectro board
#define USE_ESPECTRO_BOARD  1

//Set to 1 to start measurement automatically upon program start
#define AUTO_START_MEASUREMENT  0

#define OLED_SSD1306_DISPLAY    1

#define USE_CLOUD               1


#if USE_ESPECTRO_BOARD
#include "ESPectro.h"
ESPectro board;
ESPectro_Button onBoardButton;
#endif

#if USE_EXTERNAL_ADC
#include <ESPectroBase.h>
ESPectroBase base;
#endif

#if OLED_SSD1306_DISPLAY
#include "UIService.h"
UIService uiSvc;
#endif

#if USE_CLOUD
#include "MakestroCloudClient.h"
#include "DCX_AppSetting.h"
#include "DCX_WifiManager.h"

MakestroCloudClient makestroCloudClient("andri", "vByOlaA1UZyv8Iw7O9T64F55d9hH7iXZ1o1xkZVogPwVDjK42alDsFInJVb4gGL4", "heartbeat");
DCX_WifiManager wifiManager(AppSetting);
#endif

#if !USE_CALLBACK
// Set to 'false' by default to send the data via serial and display using desktop app.  Set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse
static boolean serialVisual = false;
#endif

//Ticker ticker;
PulseSensor sensor;
int lastBPM;

void readADC() {
#if USE_EXTERNAL_ADC
    int raw = base.analogRead(3);
#else
    int raw = analogRead(0);
#endif
    Serial.printf("Raw: %d\n", raw);
}

#if USE_CLOUD

void onMqttConnect() {
    Serial.println("** Connected to the broker **");

    MakestroCloudSubscribedPropertyCallback propsCallback = [=](const String prop, const String value) {
        Serial.print("incoming: ");
        Serial.print(prop);
        Serial.print(" = ");
        Serial.print(value);
        Serial.println();
//
//        if (value.equals("1")) {
//            board.turnOnLED();
//            neopixel.turnOn(HtmlColor(0x00f0af));
//            //gpioEx.digitalWrite(ESPECTRO_BASE_GPIOEX_LED_PIN, HIGH);
//        }
//        else {
//            board.turnOffLED();
//            neopixel.turnOff();
//            //gpioEx.digitalWrite(ESPECTRO_BASE_GPIOEX_LED_PIN, LOW);
//        }
    };

    makestroCloudClient.subscribeProperty("switch", propsCallback);
//    makestroCloudClient.subscribeProperty("hue", propsCallback);
//    makestroCloudClient.subscribe("andri/light/control", 2);
//    makestroCloudClient.publishData("{\"temperature\":23}");

//    makestroCloudClient.publishKeyValue("temperature", 25);

    //JsonKeyValueMap keyVal = {{"temperature", 35}, {"hue", 100}};
    //makestroCloudClient.publishMap(keyVal, "tweet_too_hot", "chphNHI8LjAGwEt87NALFK");

    //makestroCloudClient.triggerIFTTTEvent("notif_too_cold");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("** Disconnected from the broker **");
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
    Serial.println("** Subscribe acknowledged **");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
}

#endif

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    while(!Serial);

#if OLED_SSD1306_DISPLAY
    uiSvc.begin();
#endif

#if USE_CLOUD

    AppSetting.load();
    AppSetting.debugPrintTo(Serial);

    wifiManager.onWifiConnectStarted([]() {
        DEBUG_SERIAL("WIFI CONNECTING STARTED\r\n");
    });

    wifiManager.onWifiConnected([](boolean newConn) {
        DEBUG_SERIAL("WIFI CONNECTED\r\n");

        board.turnOffLED();

        makestroCloudClient.onConnect(onMqttConnect);
        makestroCloudClient.onDisconnect(onMqttDisconnect);
        makestroCloudClient.onSubscribe(onMqttSubscribe);

        makestroCloudClient.connect();

    });

    wifiManager.onWifiConnecting([](unsigned long elapsed) {
        //DEBUG_SERIAL("%d\r\n", elapsed);
        board.toggleLED();
    });

    wifiManager.onWifiDisconnected([](WiFiDisconnectReason reason) {
        DEBUG_SERIAL("WIFI GIVE UP\r\n");
        //board.turnOffLED();
    });

//    wifiManager.begin();
    wifiManager.begin("DyWare-AP3", "p@ssw0rd");
#endif

#if USE_EXTERNAL_ADC
    base.beginADC();
#endif

//    ticker.attach_ms(200, readADC); //-> just for testig

#if USE_CALLBACK
    sensor.onBPMAvailable([](int BPM) {
        //Serial.printf("BPM: %d\n", BPM);

        //qualifies BPM for a grown-up
        if (BPM > lastBPM+20 || BPM < lastBPM-20|| BPM > 160 || BPM < 54) {
            Serial.printf("[Not Valid] BPM: %d\n", BPM);
        }
        else {
            Serial.printf("[Valid] BPM: %d\n", BPM);
            uiSvc.setBPMValue(BPM);

#if USE_CLOUD
            if (makestroCloudClient.connected()) {
                makestroCloudClient.publishKeyValue("BPM", BPM);
            }
#endif

        }

        lastBPM = BPM;
    });

    sensor.onPulseDetectionCallback([](bool detected) {
#if USE_ESPECTRO_BOARD
        if (detected) {
            board.turnOnLED();
        }
        else {
            board.turnOffLED();
        }
#endif
    });

#endif

    //set callback to actually read the raw value from ADC
    sensor.onReadingRawSignal([](int &raw) {

#if USE_EXTERNAL_ADC
        raw = base.analogRead(3);
#else
        raw = analogRead(0);
#endif

    });

#if AUTO_START_MEASUREMENT
    sensor.start();
#else
    #if USE_ESPECTRO_BOARD

    onBoardButton.begin();
    onBoardButton.onButtonUp([](){
        if (sensor.isStarted()) {
            Serial.println("Pulse sensor stopped");
            sensor.stop();
        }
        else {
            Serial.println("Pulse sensor started");
            sensor.start();
        }
    });

    #endif
#endif
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
        lastBPM = sensor.getLastBPM();
        Serial.print(lastBPM);
        Serial.print("  ");
    } else{
        sendDataToSerial('B', sensor.getLastBPM());   // send heart rate with a 'B' prefix
        sendDataToSerial('Q', sensor.getLastIBI());   // send time between beats with a 'Q' prefix
    }
}
#endif

// the loop function runs over and over again forever
void loop() {

#if USE_CLOUD
    wifiManager.loop();
#endif

#if OLED_SSD1306_DISPLAY
    uiSvc.loop();
#endif

#if !AUTO_START_MEASUREMENT
#if USE_ESPECTRO_BOARD
    onBoardButton.loop();
#endif
#endif

#if !USE_CALLBACK
    serialOutput();

    if (sensor.isBeatDetected()) {
        serialOutputWhenBeatHappens();
    }

    delay(20);                             //  take a break
#endif

}
