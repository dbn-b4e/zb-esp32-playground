// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief This example demonstrates Zigbee temperature sensor.
 *
 * The example demonstrates how to use Zigbee library to create a end device temperature sensor.
 * The temperature sensor is a Zigbee end device, which is controlled by a Zigbee coordinator.
 *
 * Proper Zigbee mode must be selected in Tools->Zigbee mode
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 *
 * Please check the README.md for instructions and more detailed description.
 *
 * Created by Jan Procházka (https://github.com/P-R-O-C-H-Y/)
 */

#ifndef ZIGBEE_MODE_ED
// #error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"
#include "ZigbeeButton.h"

/* Zigbee temperature sensor configuration */
#define TEMP_SENSOR_ENDPOINT_NUMBER     10
#define ZIGBEE_LIGHT_ENDPOINT           20
#define BUTTON_ENDPOINT_NUMBER          30
#define CONTACT_SWITCH_ENDPOINT_NUMBER  40

#define ZIGBEE_ROLE ZIGBEE_ROUTER

/* Zigbee OTA configuration */
#define OTA_UPGRADE_RUNNING_FILE_VERSION    0x01010100  // Increment this value when the running image is updated
#define OTA_UPGRADE_DOWNLOADED_FILE_VERSION 0x01010101  // Increment this value when the downloaded image is updated
#define OTA_UPGRADE_HW_VERSION              0x0101      // The hardware version, this can be used to differentiate between different hardware versions


uint8_t button = 9;
uint8_t sensor_pin = 9;
#define LED_PIN 8

typedef enum {
  SWITCH_ON_CONTROL,
  SWITCH_OFF_CONTROL,
  SWITCH_ONOFF_TOGGLE_CONTROL,
  SWITCH_LEVEL_UP_CONTROL,
  SWITCH_LEVEL_DOWN_CONTROL,
  SWITCH_LEVEL_CYCLE_CONTROL,
  SWITCH_COLOR_CONTROL,
} SwitchFunction;

typedef struct {
  uint8_t pin;
  SwitchFunction func;
} SwitchData;

typedef enum {
  SWITCH_IDLE,
  SWITCH_PRESS_ARMED,
  SWITCH_PRESS_DETECTED,
  SWITCH_PRESSED,
  SWITCH_RELEASE_DETECTED,
} SwitchState;

// Optional Time cluster variables
struct tm timeinfo;
struct tm *localTime;
int32_t timezone;



ZigbeeTempSensor zbTempSensor = ZigbeeTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
ZigbeeLight zbLight = ZigbeeLight(ZIGBEE_LIGHT_ENDPOINT);
ZigbeeButton zbButton = ZigbeeButton(BUTTON_ENDPOINT_NUMBER);
ZigbeeContactSwitch zbContactSwitch = ZigbeeContactSwitch(CONTACT_SWITCH_ENDPOINT_NUMBER);


/********************* RGB LED functions **************************/
void setLED(bool value) {
  if (value)
    rgbLedWrite(LED_PIN,0,255,0);
  else
    rgbLedWrite(LED_PIN,0,0,0);
}

/********************* Button functions **************************/
void setButton(bool state) {
  if (state) {
    Serial.printf("Button on\n");
  } else {
    Serial.printf("Button off\n");
  }
}

/************************ Temp sensor *****************************/
static void temp_sensor_value_update(void *arg) {
  for (;;) {
    // Read temperature sensor value
    float tsens_value = temperatureRead();
    Serial.printf("Updated temperature sensor value to %.2f°C\r\n", tsens_value);
    // Update temperature value in Temperature sensor EP
    zbTempSensor.setTemperature(tsens_value);
    delay(60000);
  }
}

/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);
  Serial.println("Boot...");

 // Init button + switch
  pinMode(button, INPUT_PULLUP);
  pinMode(sensor_pin, INPUT_PULLUP);

  // Init RGB and leave light OFF
  rgbLedWrite(LED_PIN,0,0,0);

  // Set callback function for light change
  zbLight.onLightChange(setLED);

  zbLight.setVersion( 12);
  zbTempSensor.setVersion( 12);
  zbContactSwitch.setVersion( 12);
  zbButton.setVersion(12);

  // Add OTA client to the light bulb
  zbLight.addOTAClient(OTA_UPGRADE_RUNNING_FILE_VERSION, OTA_UPGRADE_DOWNLOADED_FILE_VERSION, OTA_UPGRADE_HW_VERSION);

  // Set binding settings depending on the role
  if (ZIGBEE_ROLE == ZIGBEE_COORDINATOR) {
    zbLight.allowMultipleBinding(true);  // To allow binding multiple lights to the switch
  } else {
    zbLight.setManualBinding(true);  //Set manual binding to true, so binding is done on Home Assistant side
  }

  // Add zbButton to Zigbee Core
  zbButton.onButtonChange(setButton);
  Zigbee.addEndpoint(&zbButton);

  //Add endpoint to Zigbee Core
  Serial.println("Adding ZigbeeSwitch endpoint to Zigbee Core");
  Zigbee.addEndpoint(&zbContactSwitch);

  //Add endpoint to Zigbee Core
  Serial.println("Adding ZigbeeLight endpoint to Zigbee Core");
  Zigbee.addEndpoint(&zbLight);

  // Optional: set Zigbee device name and model
  zbTempSensor.setManufacturerAndModel("B4E", "ZB_Test_1");

  // Set minimum and maximum temperature measurement value (10-50°C is default range for chip temperature measurement)
  zbTempSensor.setMinMaxValue(10, 60);

  // Optional: Set tolerance for temperature measurement in °C (lowest possible value is 0.01°C)
  zbTempSensor.setTolerance(1);

  // Optional: Time cluster configuration (default params, as this device will revieve time from coordinator)
  zbTempSensor.addTimeCluster();

  // Add endpoint to Zigbee Core
    Serial.println("Adding ZigbeeTemperature endpoint to Zigbee Core");
  Zigbee.addEndpoint(&zbTempSensor);

  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin(ZIGBEE_ROLE)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  Serial.println("Connected to network!");

  Serial.println();

  // Optional: If time cluster is added, time can be read from the coordinator
  timeinfo = zbTempSensor.getTime();
  timezone = zbTempSensor.getTimezone();

  Serial.println("UTC time:");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  time_t local = mktime(&timeinfo) + timezone;
  localTime = localtime(&local);

  Serial.println("Local time with timezone:");
  Serial.println(localTime, "%A, %B %d %Y %H:%M:%S");

  // Start Temperature sensor reading task
  xTaskCreate(temp_sensor_value_update, "temp_sensor_update", 2048, NULL, 10, NULL);

  // Set reporting interval for temperature measurement in seconds, must be called after Zigbee.begin()
  // min_interval and max_interval in seconds, delta (temp change in 0,1 °C)
  // if min = 1 and max = 0, reporting is sent only when temperature changes by delta
  // if min = 0 and max = 10, reporting is sent every 10 seconds or temperature changes by delta
  // if min = 0, max = 10 and delta = 0, reporting is sent every 10 seconds regardless of temperature change
  zbTempSensor.setReporting(0, 10, 0);
  zbButton.setReporting(0, 10, 1);

  // Start Zigbee OTA client query, first request is within a minute and the next requests are sent every hour automatically
  zbLight.requestOTAUpdate();

}

void loop() {

  // Checking pin for contact change
#if 0
  static bool contact = false;
  if (digitalRead(sensor_pin) == HIGH && !contact)
  {
    // Update contact sensor value

    zbContactSwitch.setClosed();
    contact = true;
    Serial.println("Switch true");
  } else if (digitalRead(sensor_pin) == LOW && contact)
  {
    zbContactSwitch.setOpen();
    contact = false;
    Serial.println("Switch false");
  }
#endif

  // Checking button for factory reset
  if (digitalRead(button) == LOW)
  {  // Push button pressed
    // Key debounce handling
    delay(10);
    int startTime = millis();
    int pressed = 0;
    #if 0
    while (digitalRead(button) == LOW)
    {
      delay(10);
    }
    #endif
    Serial.println("Toggle button state...");
    zbButton.toggleButton();
    
#if 0
    Serial.println("Reporting button state...");
    zbButton.setButtonState(false);
    delay(500);
    zbButton.reportButton();
    delay(2000);
    zbButton.setButtonState(true);
    delay(500);
    zbButton.reportButton();
#endif
  }


#if 1
  // Checking button for factory reset
  if (digitalRead(button) == LOW)
  {  // Push button pressed
    // Key debounce handling
    delay(10);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(10);
      if ((millis() - startTime) > 1000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 100ms.");
        delay(5000);
        Zigbee.factoryReset();
      }
    }
  }

  #endif

  
  // Send temperature each 10 seconds
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 300000) {
    lastPrint = millis();
    zbTempSensor.reportTemperature();
  }
}
