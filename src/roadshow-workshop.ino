/*
 * Project roadshow-workshop
 * Description: https://accelerate-workshop-2019.herokuapp.com/docs/ch2.html#working-with-particle-variables-plus-the-temperature-humidity-sensor
 * Author: Ben Peterson
 * Date:
 */

#include "Grove_Temperature_And_Humidity_Sensor.h"
#include "Grove_ChainableLED.h"
#include "DiagnosticsHelperRK.h"
#include "JsonParserGeneratorRK.h"

SYSTEM_THREAD(ENABLED);

ChainableLED leds(A4, A5, 1);
DHT dht(D2);

// Private battery and power service UUID
const BleUuid serviceUuid("9dae10af-bb32-46db-b033-f052feab6a1b");

BleCharacteristic uptimeCharacteristic("uptime", BleCharacteristicProperty::NOTIFY, BleUuid("fdcf4a3f-3fed-4ed2-84e6-04bbb9ae04d4"), serviceUuid);
BleCharacteristic signalStrengthCharacteristic("strength", BleCharacteristicProperty::NOTIFY, BleUuid("cc97c20c-5822-4800-ade5-1f661d2133ee"), serviceUuid);
BleCharacteristic freeMemoryCharacteristic("freeMemory", BleCharacteristicProperty::NOTIFY, BleUuid("d2b26bf3-9792-42fc-9e8a-41f6107df04c"), serviceUuid);

const unsigned long UPDATE_INTERVAL = 2000;
unsigned long lastUpdate = 0;
float temp, humidity;
double temp_dbl, humidity_dbl;
double currentLightLevel;

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.

  Serial.begin(9600);

  dht.begin();
  pinMode(A0, INPUT);

  Particle.variable("temp", temp_dbl);
  Particle.variable("humidity", humidity_dbl);
  Particle.function("toggleLed", toggleLed);

  leds.init();
  leds.setColorHSB(0, 0.0, 0.0, 0.0);

  configureBLE();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.

  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = millis();

    temp = dht.getTempFarenheit();
    humidity = dht.getHumidity();

    temp_dbl = temp;
    humidity_dbl = humidity;

    double lightAnalogVal = analogRead(A0);
    currentLightLevel = map(lightAnalogVal, 0.0, 4095.0, 0.0, 100.0);

    Serial.printlnf("Temp: %f", temp);
    Serial.printlnf("Humidity: %f", humidity);

    if (currentLightLevel > 50) {
      Particle.publish("light-meter/level", String(currentLightLevel), PRIVATE);
    }

    if (BLE.connected())
    {
      uint8_t uptime = (uint8_t)DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_UPTIME);
      uptimeCharacteristic.setValue(uptime);

      uint8_t signalStrength = (uint8_t)(DiagnosticsHelper::getValue(DIAG_ID_NETWORK_SIGNAL_STRENGTH) >> 8);
      signalStrengthCharacteristic.setValue(signalStrength);

      int32_t usedRAM = DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_USED_RAM);
      int32_t totalRAM = DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_TOTAL_RAM);
      int32_t freeMem = (totalRAM - usedRAM);
      freeMemoryCharacteristic.setValue(freeMem);
    }

    createEventPayload(temp, humidity, currentLightLevel);
  }
}

int toggleLed(String args) {
  leds.setColorHSB(0, 0.0, 1.0, 0.5);
  delay(1000);
  leds.setColorHSB(0, 0.0, 0.0, 0.0);
  return 1;
}

void configureBLE()
{
  BLE.addCharacteristic(uptimeCharacteristic);
  BLE.addCharacteristic(signalStrengthCharacteristic);
  BLE.addCharacteristic(freeMemoryCharacteristic);

  BleAdvertisingData advData;

  // Advertise our private service only
  advData.appendServiceUUID(serviceUuid);

  // Continuously advertise when not connected
  BLE.advertise(&advData);
}

void createEventPayload(int temp, int humidity, double light)
{
  JsonWriterStatic<256> jw;
  {
    JsonWriterAutoObject obj(&jw);

    jw.insertKeyValue("temp", temp);
    jw.insertKeyValue("humidity", humidity);
    jw.insertKeyValue("light", light);
  }
  Particle.publish("env-vals", jw.getBuffer(), PRIVATE);
}
