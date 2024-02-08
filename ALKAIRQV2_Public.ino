#include <Arduino.h>
#include <M5GFX.h>
#include <Wire.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <lgfx/v1/panel/Panel_GDEW0154D67.hpp>

#include <sntp.h>
#include <freertos/queue.h>

#include "I2C_BM8563.h"
#include <SensirionI2CScd4x.h>
#include <SensirionI2CSen5x.h>
#include <OneButton.h>
#include <cJSON.h>
#include <Preferences.h>

#include "config.h"
#include "misc.h"
#include "Sensor.hpp"
#include "sensorData.h"
sensorData co2Saved(true, 1000);
sensorData vocSaved(true, 1000);
sensorData ppmSaved(true, 1000);
sensorData tempSaved(true, 1000);


class AirQ_GFX : public lgfx::LGFX_Device {
    lgfx::Panel_GDEW0154D67 _panel_instance;
    lgfx::Bus_SPI           _spi_bus_instance;

   public:
    AirQ_GFX(void) {
        {
            auto cfg = _spi_bus_instance.config();

            cfg.pin_mosi   = EPD_MOSI;
            cfg.pin_miso   = EPD_MISO;
            cfg.pin_sclk   = EPD_SCLK;
            cfg.pin_dc     = EPD_DC;
            cfg.freq_write = EPD_FREQ;

            _spi_bus_instance.config(cfg);
            _panel_instance.setBus(&_spi_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();

            cfg.invert       = true;
            cfg.pin_cs       = EPD_CS;
            cfg.pin_rst      = EPD_RST;
            cfg.pin_busy     = EPD_BUSY;
            cfg.panel_width  = 200;
            cfg.panel_height = 200;
            cfg.offset_x     = 0;
            cfg.offset_y     = 0;

            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
    bool begin(void) { return init_impl(true , false); };
};
#define NUM_SSIDS 4  // must also add to two other arrays if increasing this #
const char *ssid[NUM_SSIDS] = { "", "", "", "" };
const char *password = "";
char *connectedSSID;
WiFiClient clientWifi;
PubSubClient client(clientWifi);
boolean connectedInternet = false;

SensirionI2CScd4x scd4x;
SensirionI2CSen5x sen5x;
I2C_BM8563 bm8563(I2C_BM8563_DEFAULT_ADDRESS, Wire);
Sensor sensor(scd4x, sen5x, bm8563);
Preferences preferences;
uint32_t successCounter = 0;
uint32_t failCounter = 0;
int voc=0;
int nox=0;
boolean needToReadSEN55Data = false;
#define MAX_BOOTUP_COUNT 5
int bootupCount = 0;

AirQ_GFX display;
//M5GFX display;

M5Canvas canvas(&display);
M5Canvas canvasGraph(&display);

String mac;
String apSSID;

 #define NTP_TIMEZONE  "EST5EDT,M3.2.0,M11.1.0"
 #define NTP_SERVER1   "0.pool.ntp.org"
 #define NTP_SERVER2   "1.pool.ntp.org"
 #define NTP_SERVER3   "2.pool.ntp.org"

typedef enum ButtonID_t {
  E_BUTTON_NONE,
  E_BUTTON_A,
  E_BUTTON_B,
  E_BUTTON_POWER,
} ButtonID_t;

typedef enum ButtonClickType_t {
  E_BUTTON_CLICK_TYPE_NONE,
  E_BUTTON_CLICK_TYPE_SINGLE,
  E_BUTTON_CLICK_TYPE_PRESS,
} ButtonClickType_t;


typedef struct ButtonEvent_t {
  ButtonID_t id;
  ButtonClickType_t type;
} ButtonEvent_t;


OneButton btnA = OneButton(
                   USER_BTN_A,  // Input pin for the button
                   true,        // Button is active LOW
                   true         // Enable internal pull-up resistor
                 );

OneButton btnB = OneButton(
                   USER_BTN_B,  // Input pin for the button
                   true,        // Button is active LOW
                   true         // Enable internal pull-up resistor
                 );

OneButton btnPower = OneButton(
                       USER_BUTTON_POWER,  // Input pin for the button
                       true,        // Button is active LOW
                       false         // Enable internal pull-up resistor
                     );

QueueHandle_t buttonEventQueue;
boolean donotsleep = false;
void btnAClickEvent() {
  log_d("btnAClickEvent");

  BUTTON_TONE();

  ButtonEvent_t buttonEvent = { .id = E_BUTTON_A, .type = E_BUTTON_CLICK_TYPE_SINGLE };
  if (xQueueSendToBack(buttonEventQueue, &buttonEvent, ( TickType_t ) 10 ) != pdPASS) {
    log_w("buttonEventQueue send Failed");
  }
}
void btnBClickEvent() {
  log_d("btnBClickEvent");

  BUTTON_TONE();

  ButtonEvent_t buttonEvent = { .id = E_BUTTON_B, .type = E_BUTTON_CLICK_TYPE_SINGLE };
  if (xQueueSendToBack(buttonEventQueue, &buttonEvent, ( TickType_t ) 10 ) != pdPASS) {
    log_w("buttonEventQueue send Failed");
  }
}
void btnALongPressStartEvent() {
  log_d("btnALongPressStartEvent");

  BUTTON_TONE();

  ButtonEvent_t buttonEvent = { .id = E_BUTTON_A, .type = E_BUTTON_CLICK_TYPE_PRESS };
  if (xQueueSendToBack(buttonEventQueue, &buttonEvent, ( TickType_t ) 10 ) != pdPASS) {
    log_w("buttonEventQueue send Failed");
  }
}
void btnBLongPressStartEvent() {
  log_d("btnBLongPressStartEvent");

  BUTTON_TONE();

  ButtonEvent_t buttonEvent = { .id = E_BUTTON_B, .type = E_BUTTON_CLICK_TYPE_PRESS };
  if (xQueueSendToBack(buttonEventQueue, &buttonEvent, ( TickType_t ) 10 ) != pdPASS) {
    log_w("buttonEventQueue send Failed");
  }
}
  void keyInterrupt() {
     esp_deep_sleep(0);
  }
void initInterrupts() {
    pinMode(USER_BUTTON_POWER, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(USER_BUTTON_POWER), keyInterrupt, FALLING);
}

void updateRunMode(String msg, boolean invertMode = false);

void setup() {
  Serial.begin(115200);

  log_i("Turn on main power");
  pinMode(POWER_HOLD, OUTPUT);
  digitalWrite(POWER_HOLD, HIGH);
  bootUpPreferences(); 

  log_i("Turn on SEN55 power");
  pinMode(SEN55_POWER_EN, OUTPUT);

  /** Start Beep */
  ledcAttachPin(BUZZER_PIN, 0);
  //BUTTON_TONE();

  log_i("NVS init");
  preferences.begin("airq", false);
  successCounter = preferences.getUInt("OK", 0);
  failCounter = preferences.getUInt("NG", 0);

  log_i("I2C init");
  pinMode(GROVE_SDA, OUTPUT);
  pinMode(GROVE_SCL, OUTPUT);
  Wire.begin(I2C1_SDA_PIN, I2C1_SCL_PIN);

  log_i("RTC(BM8563) init");
  bm8563.begin();
  bm8563.clearIRQ();
  initSensors(true);
  //initInterrupts();
  displayInit();
  btnA.attachClick(btnAClickEvent);
  btnA.attachLongPressStart(btnALongPressStartEvent);
  btnA.setPressTicks(5000);
  btnB.attachClick(btnBClickEvent);
  btnB.attachLongPressStart(btnBLongPressStartEvent);
  btnB.setPressTicks(5000);
  buttonEventQueue = xQueueCreate(16, sizeof(ButtonEvent_t));
  xTaskCreatePinnedToCore(buttonTask, "Button Task", 4096, NULL, 5, NULL, APP_CPU_NUM);
 // pinMode(39, OUTPUT);
 // digitalWrite(39, HIGH);  // turn the LED on (HIGH is the voltage level)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)

}
#define TIME_BETWEEN_SENSOR_QUERIES 90
void gotoSleep() {
  updateRunMode("Sleeping", true);
  saveSensorReadings(voc, nox); // save for future bootups
  debugMessage("Sleeping");
  powerDownSensors();
  display.sleep();
  display.waitDisplay();
  bm8563.clearIRQ();
  bm8563.SetAlarmIRQ(TIME_BETWEEN_SENSOR_QUERIES);
  delay(10);
  //digitalWrite(POWER_HOLD, LOW);
  delay(1000);
  esp_deep_sleep(TIME_BETWEEN_SENSOR_QUERIES * 1000 * 1000);
  esp_deep_sleep_start();
}

void buttonTask(void *) {

  for (;;) {
    btnA.tick();
    btnB.tick();
    btnPower.tick();
    delay(10);
  }

  vTaskDelete(NULL);
}
ButtonEvent_t buttonEvent = {
      .id = E_BUTTON_NONE,
      .type = E_BUTTON_CLICK_TYPE_NONE
    };

long sensorTimeStamp = -(100*1000);
void loop() {
    static int loopCount=0;
    // need to warm up VIOC/NOX sensor for 2 minutes
    if (loopCount==0 || donotsleep) {
       if (needToReadSEN55Data) {
              for (int i=0; i<65; ++i) {
                  sensor.getSEN55MeasurementResult();
                  showMessage(" Warming:" + String(i) + "/" + String(sensor.sen55.vocIndex));
                  delay(2000);
              };
              voc = sensor.sen55.vocIndex;
              nox =(isnan(sensor.sen55.noxIndex)?0:(int) sensor.sen55.noxIndex);
        } else {
              readSensorReadings(); // use saved ones instead for VOC and NOX
              sensor.getSEN55MeasurementResult(); // get PPM etc
              if (donotsleep) {
                  voc = sensor.sen55.vocIndex;
                  nox =(isnan(sensor.sen55.noxIndex)?0:(int) sensor.sen55.noxIndex);
              }
        }
        sensor.getSCD40MeasurementResult();
        sensor.getBatteryVoltageRaw();
        co2Saved.addData((int) sensor.scd40.co2); 
        vocSaved.addData(voc); 
        ppmSaved.addData(sensor.sen55.massConcentrationPm2p5); 
        tempSaved.addData(sensor.sen55.ambientTemperature); 

        if (loopCount == 0 || millis() > (sensorTimeStamp + 90 * 1000)) { // no more than once per 90 seconds}
            sensorTimeStamp = millis();
            String msg = ("Augmented Sensor#" + 
                    (String) (int) sensor.scd40.co2  + "#" +
                    (String) (int) sensor.sen55.ambientTemperature  + "#" + 
                    (String) (int) sensor.sen55.ambientHumidity  + "#" + 
                    (String) (int) sensor.sen55.massConcentrationPm2p5  + "#" + 
                    (String) (int) voc + "#" + 
                    (String) (int) nox + "#" + 
                    "REMREMOTE");
            updateRunMode("Connecting ...");
            sendMessageWifi(msg);
            showWifiStatus(connectedInternet);
        }
        updateInfo();
        updateRunMode(donotsleep?"Continuous":"Auto Sleep");
        needToReadSEN55Data=false;
    }

    xQueueReceive(buttonEventQueue, &buttonEvent, (TickType_t)10);
    checkButton(&buttonEvent);
    for (int i=0; !needToReadSEN55Data && i<(5*5); ++i) {
         delay(200);
         xQueueReceive(buttonEventQueue, &buttonEvent, (TickType_t)10);
         checkButton(&buttonEvent);
    }
    if (!donotsleep) gotoSleep();
    loopCount++;
    //digitalWrite(POWER_HOLD, HIGH);
    //delay(TIME_BETWEEN_SENSOR_QUERIES * 1000); // only is on USB power
    //debugMessage("Connected to USB power detected");
   // display.wakeup();
}
