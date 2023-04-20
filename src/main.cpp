#include <HTTPClient.h>
#include "ArduinoJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace 0 by ID of this current device
const int DEVICE_ID = 0;

int test_delay = 1000; // so we don't spam the API
boolean describe_tests = true;

// Replace 0.0.0.0 by your server local IP
String serverName = "http://192.168.1.88/";
HTTPClient http;

// Replace WifiName and WifiPassword by your WiFi credentials
#define STASSID "GalaxyHotspot"
#define STAPSK "d7?a35D9EnaPepXY?c!4"

// NTP (Net time protocol) settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Pinout settings
const int analogSensorPin = 34;
const int digitalSensorPin = 13;
const int actuatorPin = 15;
const int analogActuatorPin = 2;

// Setup
void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(STASSID);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup!");

  // Configure pin modes for actuators (output mode) and sensors (input mode). Pin numbers should be described by GPIO number (https://www.upesy.com/blogs/tutorials/esp32-pinout-reference-gpio-pins-ultimate-guide)
  // For ESP32 WROOM 32D https://uelectronics.com/producto/esp32-38-pines-esp-wroom-32/
  // You must find de pinout for your specific board version
  pinMode(actuatorPin, OUTPUT);
  pinMode(analogActuatorPin, OUTPUT);
  pinMode(analogSensorPin, INPUT);
  pinMode(digitalSensorPin, INPUT);

  // Init and get the time
  timeClient.begin();
}

String response;

String serializeSensorValueBody(int idSensor, long timestamp, float value)
{
  DynamicJsonDocument doc(2048);

  // StaticJsonObject allocates memory on the stack, it can be
  // replaced by DynamicJsonDocument which allocates in the heap.
  //
  // DynamicJsonDocument  doc(200);

  // Add values in the document
  //
  doc["idSensor"] = idSensor;
  doc["timestamp"] = timestamp;
  doc["value"] = value;

  // Generate the minified JSON and send it to the Serial port.
  //
  String output;
  serializeJson(doc, output);
  // Line below prints (or something similar with different values):
  // {"idSensor":18,"timestamp":1351824120,"value":48.75}
  // Start a new line
  Serial.println(output);

  return output;
}

String serializeActuatorStatusBody(float status, bool statusBinary, int idActuator, long timestamp, bool removed)
{
  DynamicJsonDocument doc(2048);

  doc["status"] = status;
  doc["statusBinary"] = statusBinary;
  doc["idActuator"] = idActuator;
  doc["timestamp"] = timestamp;
  doc["removed"] = removed;

  String output;
  serializeJson(doc, output);
  return output;
}

String serializeDeviceBody(String deviceSerialId, String name, String mqttChannel, int idGroup)
{
  DynamicJsonDocument doc(2048);

  doc["deviceSerialId"] = deviceSerialId;
  doc["name"] = name;
  doc["mqttChannel"] = mqttChannel;
  doc["idGroup"] = idGroup;

  String output;
  serializeJson(doc, output);
  return output;
}

void deserializeActuatorStatusBody(String responseJson)
{
  if (responseJson != "")
  {
    DynamicJsonDocument doc(2048);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, responseJson);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // Fetch values.
    //
    // Most of the time, you can rely on the implicit casts.
    // In other case, you can do doc["time"].as<long>();
    int idActuatorState = doc["idActuatorState"];
    float status = doc["status"];
    bool statusBinary = doc["statusBinary"];
    int idActuator = doc["idActuator"];
    long timestamp = doc["timestamp"];
    bool removed = doc["removed"];
    // const char *sensor = doc["sensor"];

    // Print values.
    Serial.println("Actuator status deserialized:");
    Serial.println(idActuatorState);
    Serial.println(status);
    Serial.println(statusBinary);
    Serial.println(idActuator);
    Serial.println(timestamp);
    Serial.println(removed);
  }
}

void deserializeDeviceBody(String responseJson)
{
  if (responseJson != "")
  {
    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    int idDevice = doc["idDevice"];
    String deviceSerialId = doc["deviceSerialId"];
    String name = doc["name"];
    String mqttChannel = doc["mqttChannel"];
    int idGroup = doc["idGroup"];

    Serial.println("Device deserialized:");
    Serial.println(idDevice);
    Serial.println(deviceSerialId);
    Serial.println(name);
    Serial.println(mqttChannel);
    Serial.println(idGroup);
  }
}

void test_response(int httpResponseCode)
{
  delay(test_delay);
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void describe(char *description)
{
  if (describe_tests)
    Serial.println(description);
}

void GET_tests()
{
  describe("Test GET full device info");
  String serverPath = serverName + "api/devices/" + String(DEVICE_ID);
  http.begin(serverPath.c_str());
  test_response(http.GET());

  describe("Test GET sensors from deviceID");
  serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/sensors";
  http.begin(serverPath.c_str());
  test_response(http.GET());

  describe("Test GET actuators from deviceID");
  serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/actuators";
  http.begin(serverPath.c_str());
  test_response(http.GET());

  describe("Test GET sensors from deviceID and Type");
  serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/sensors/TEMPERATURE";
  http.begin(serverPath.c_str());
  test_response(http.GET());

  describe("Test GET actuators from deviceID");
  serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/actuators/RELAY";
  http.begin(serverPath.c_str());
  test_response(http.GET());
}

void POST_tests()
{
  String actuator_states_body = serializeSensorValueBody(18, millis(), random(2000, 4000) / 100);
  describe("Test POST with path and body and response");
  String serverPath = serverName + "api/actuator_states";
  http.begin(serverPath.c_str());
  test_response(http.POST(actuator_states_body));

  String device_body = serializeDeviceBody(String(DEVICE_ID), ("Name_" + String(DEVICE_ID)).c_str(), ("mqtt_" + String(DEVICE_ID)).c_str(), 12);
  describe("Test POST with path and body and response");
  serverPath = serverName + "api/device";
  http.begin(serverPath.c_str());
  test_response(http.POST(actuator_states_body));
}

void PUT_tests()
{
  int temp = 38;
  long timestamp = 151241254122;
  // POST TESTS
  String post_body = "{ 'idsensor' : 18, 'value': " + temp;
  post_body = post_body + " , 'timestamp' :";
  post_body = post_body + timestamp;
  post_body = post_body + ", 'user' : 'Luismi'}";

  describe("Test PUT with path and body");
  String serverPath = serverName + "api/actuator_states/18";
  http.begin(serverPath.c_str());
  test_response(http.PUT(post_body));
}

// Run the tests!
void loop()
{
  GET_tests();
  POST_tests();
  delay(1000);
  timeClient.update();

  Serial.println(timeClient.getFormattedTime());

  if (timeClient.getSeconds() % 2 == 1)
  {
    digitalWrite(actuatorPin, HIGH);
    Serial.println("ON");
  }
  else
  {
    digitalWrite(actuatorPin, LOW);
    Serial.println("OFF");
  }


  ledcWrite(analogActuatorPin, timeClient.getSeconds());

  int analogValue = analogRead(analogSensorPin);
  int digitalValue = digitalRead(digitalSensorPin);
  Serial.println("Analog sensor value :" + String(analogValue));
  if (digitalValue == HIGH)
  {
    Serial.println("Digital sensor value : ON");
  }
  else
  {
    Serial.println("Digital sensor value : OFF");
  }
}