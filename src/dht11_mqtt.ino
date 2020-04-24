// FILE:        PietteTech_DHT.h
// VERSION:     0.0.12
// PURPOSE:     Particle Interrupt driven lib for DHT sensors
// LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
//
// S Piette (Piette Technologies) scott.piette@gmail.com
//      January 2014        Original Spark Port
//      October 2014        Added support for DHT21/22 sensors
//                          Improved timing, moved FP math out of ISR
//      September 2016      Updated for Particle and removed dependency
//                          on callback_wrapper.  Use of callback_wrapper
//                          is still for backward compatibility but not used
// ScruffR
//      February 2017       Migrated for Libraries 2.0
//                          Fixed blocking acquireAndWait()
//                          and previously ignored timeout setting
//      January  2019       Updated timing for Particle Mesh devices
//                          issue: https://github.com/particle-iot/device-os/issues/1654
//      November 2019       Incorporate workaround for SOS+14 bug
//                          https://github.com/eliteio/PietteTech_DHT/issues/1
//
// Based on adaptation by niesteszeck (github/niesteszeck)
// Based on original DHT11 library (http://playgroudn.adruino.cc/Main/DHT11Lib)
//
// With this library connect the DHT sensor to the interrupt enabled pins
// See docs for more background
//   https://docs.particle.io/reference/firmware/photon/#attachinterrupt-

#include "PietteTech_DHT.h"
#include "MQTT.h"

 // system defines
#define DHTTYPE  DHT11              // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D3                 // Digital pin for communications
#define DHT_SAMPLE_INTERVAL   10000  // Sample every ten seconds

SerialLogHandler logger(LOG_LEVEL_ERROR, {{ "app", LOG_LEVEL_TRACE }});

//
// NOTE: Use of callback_wrapper has been deprecated but left in this example
//       to confirm backwards compabibility.  Look at DHT_2sensor for how
//       to write code without the callback_wrapper
//

void dht_wrapper();                 // must be declared before the lib initialization
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
void dht_wrapper() {
  DHT.isrCallback();
}

bool bDHTstarted;		                // flag to indicate we started acquisition
int n;                              // counter

// device mac address
byte mac[6];
byte server[] = { 138,197,6,61 };

char _mac[13];


// MQTT initialization
void callback(char* topic, byte* payload, unsigned int length);
MQTT client(server, 3881, callback);

void connect() {
  Serial.println("Connecting to MQTT broker server");
  while (!client.isConnected()) {
    client.connect("rulebloxclient_" + String(Time.now()));
    if(client.isConnected()) {
      Serial.println("connected!");
    }
  }
}

// receive feedback or message from MQTT broker
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Command received:");
}

// disable OTA updates
int disableOTA(String arg) {
  System.disableUpdates();
  return 0;
}

void setup()
{
  Particle.function("disableOTA", disableOTA);
  Serial.begin(9600);
  delay(2000);
  Serial.println("DHT Simple program using DHT.acquire");
  Serial.printlnf("LIB version: %s", (const char*)DHTLIB_VERSION);
  Serial.println("---------------");
  DHT.begin();

  // set mac address
  WiFi.macAddress(mac);
  sprintf(_mac, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
}

void loop()
{
  static uint32_t msLastSample = 0;
  if (millis() - msLastSample <  DHT_SAMPLE_INTERVAL) return;

  if (!bDHTstarted) {               // start the sample
    Serial.printlnf("\r\n%d: Retrieving information from sensor: ", n);
    DHT.acquire();
    bDHTstarted = true;
  }

  if (!DHT.acquiring()) {           // has sample completed?
    int result = DHT.getStatus();

    Serial.print("Read sensor: ");
    switch (result) {
      case DHTLIB_OK:
        Serial.println("OK");
        break;
      case DHTLIB_ERROR_CHECKSUM:
        Serial.println("Error\n\r\tChecksum error");
        break;
      case DHTLIB_ERROR_ISR_TIMEOUT:
        Serial.println("Error\n\r\tISR time out error");
        break;
      case DHTLIB_ERROR_RESPONSE_TIMEOUT:
        Serial.println("Error\n\r\tResponse time out error");
        break;
      case DHTLIB_ERROR_DATA_TIMEOUT:
        Serial.println("Error\n\r\tData time out error");
        break;
      case DHTLIB_ERROR_ACQUIRING:
        Serial.println("Error\n\r\tAcquiring");
        break;
      case DHTLIB_ERROR_DELTA:
        Serial.println("Error\n\r\tDelta time to small");
        break;
      case DHTLIB_ERROR_NOTSTARTED:
        Serial.println("Error\n\r\tNot started");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }

    Serial.printlnf("MAC address is %s", _mac);

    // Serial.printlnf("Humidity (%): %.2f", DHT.getHumidity());
    // Serial.printlnf("Temperature (oC): %.2f", DHT.getCelsius());
    // Serial.printlnf("Temperature (oF): %.2f", DHT.getFahrenheit());
    // Serial.printlnf("Temperature (K): %.2f", DHT.getKelvin());
    // Serial.printlnf("Dew Point (oC): %.2f", DHT.getDewPoint());
    // Serial.printlnf("Dew Point Slow (oC): %.2f", DHT.getDewPointSlow());

    char humidity_value[8];
    sprintf(humidity_value, "%0.2f", DHT.getHumidity());
    const char* humidity_value_str = humidity_value;
    Serial.printlnf("Humidity (%): %s", humidity_value_str);

    // publish to broker
    if (!client.isConnected()) {
      connect();
    }
    client.publish("project/room/device/dht11", humidity_value_str);

    n++;
    bDHTstarted = false;  // reset the sample flag so we can take another
    msLastSample = millis();
  }
}
