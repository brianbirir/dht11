/*
 * FILE:        DHT_simple.cpp
 * VERSION:     0.4
 * PURPOSE:     Example that uses DHT library with two sensors
 * LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * Samples one sensor and monitors the results for long term
 * analysis.  It calls DHT.acquireAndWait
 *
 * Scott Piette (Piette Technologies) scott.piette@gmail.com
 *      January 2014        Original Spark Port
 *      October 2014        Added support for DHT21/22 sensors
 *                          Improved timing, moved FP math out of ISR
 *      September 2016      Updated for Particle and removed dependency
 *                          on callback_wrapper.  Use of callback_wrapper
 *                          is still for backward compatibility but not used
 *                          is still for backward compatibility but not used
 * ScruffR
 *      February 2017       Migrated for Libraries 2.0
 *                          Fixed blocking acquireAndWait()
 *                          and previously ignored timeout setting
 *                          Added timeout when waiting for Serial input
 *
 * With this library connect the DHT sensor to the following pins
 * Spark Core: D0, D1, D2, D3, D4, A0, A1, A3, A5, A6, A7
 * Particle  : any Pin but D0 & A5
 * See docs for more background
 *   https://docs.particle.io/reference/firmware/photon/#attachinterrupt-
 */

#include "PietteTech_DHT.h"
#include "MQTT.h"

#define DHTTYPE  DHT11       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D2          // Digital pin for communications

int sendDelay = 30000;

TCPClient t_client;
byte server[] = { 216, 58, 223, 78 }; // Google

void callback(char* topic, byte* payload, unsigned int length);
MQTT client("ruleblox.com", 1883, callback);

// receive message from MQTT broker
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Command received:");
}

void connect() {
  Serial.print("Connecting to Ruleblox");
  while (!client.isConnected()) {
    client.connect("rulebloxclient_" + String(Time.now()));
    if(client.isConnected()) {
      Serial.println("connected!");
    }
  }
}

 /*
  * NOTE: Use of callback_wrapper has been deprecated
  */
  //declaration
  //void dht_wrapper(); // must be declared before the lib initialization

  // Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE);
int n;      // counter

void setup()
{
  Serial.begin(9600);

  Serial.println("connecting...");

  // establish TCP connection
  if (t_client.connect(server, 80))
  {
    Serial.println("connected");
    t_client.println("GET /search?q=unicorn HTTP/1.0");
    t_client.println("Host: www.google.com");
    t_client.println("Content-Length: 0");
    t_client.println();
  }
  else
  {
    Serial.println("connection failed");
  }

  while (!Serial.available() && millis() < 30000) {
    Serial.println("Press any key to start.");
    Particle.process();
    delay(1000);
  }

  Serial.println("DHT Simple program using DHT.acquireAndWait");
  Serial.print("LIB version: ");
  Serial.println(DHTLIB_VERSION);
  Serial.println("---------------");
  DHT.begin();
}

/*
 * NOTE:  Use of callback_wrapper has been deprecated but left in this example
 * to confirm backwards compabibility.
 */
 // This wrapper is in charge of calling
 // must be defined like this for the lib work
 //void dht_wrapper() {
 //    DHT.isrCallback();
 //}

void loop()
{
  Serial.print("\n");
  Serial.print(n);
  Serial.print(": Retrieving information from sensor: ");
  Serial.print("Read sensor: ");

  int result = DHT.acquireAndWait(1000); // wait up to 1 sec (default indefinitely)

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
  Serial.print("Humidity (%): ");
  Serial.println(DHT.getHumidity());

  Serial.print("Temperature (oC): ");
  Serial.println(DHT.getCelsius());

  // publish to broker
  if (!client.isConnected()) {
    connect();
    client.publish("sensor/temp", DHT.getCelsius());
    client.publish("sensor/hum", DHT.getHumidity());
  }

  n++;
  delay(sendDelay);
}
