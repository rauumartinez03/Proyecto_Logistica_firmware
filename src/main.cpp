#include "RestClient.h"
#include "ArduinoJson.h"
#include <ESP8266WiFi.h>

// Replace 0 by ID of this current device
const int DEVICE_ID = 0;

int test_delay = 1000; //so we don't spam the API
boolean describe_tests = true;

// Replace 0.0.0.0 by your server local IP
RestClient client = RestClient("0.0.0.0", 80);

// Replace WifiName and WifiPassword by your WiFi credentials
#define STASSID "WifiName"
#define STAPSK  "WifiPassword"

//Setup
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

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup!");
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

void deserializeActuatorStatusBody(String responseJson){
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
    //const char *sensor = doc["sensor"];
    
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

void GET_tests()
{
  describe("Test GET full device info");
  test_status(client.get(("/api/devices/" + String(DEVICE_ID)).c_str(), &response));
  test_response();

  describe("Test GET sensors from deviceID");
  test_status(client.get(("/api/devices/" + String(DEVICE_ID) + "/sensors").c_str(), &response));
  test_response();

  describe("Test GET actuators from deviceID");
  test_status(client.get(("/api/devices/" + String(DEVICE_ID) + "/actuators").c_str(), &response));
  test_response();

  describe("Test GET sensors from deviceID and Type");
  test_status(client.get(("/api/devices/" + String(DEVICE_ID) + "/sensors/TEMPERATURE").c_str(), &response));
  test_response();

  describe("Test GET actuators from deviceID");
  test_status(client.get(("/api/devices/" + String(DEVICE_ID) + "/actuators/RELAY").c_str(), &response));
  test_response();
}

void POST_tests()
{
  String actuator_states_body = serializeSensorValueBody(18, millis(), random(2000, 4000)/100);
  describe("Test POST with path and body and response");
  test_status(client.post("/api/actuator_states/", actuator_states_body.c_str(), &response));
  test_response();

  String device_body = serializeDeviceBody(String(DEVICE_ID), ("Name_" + String(DEVICE_ID)).c_str(), ("mqtt_" + String(DEVICE_ID)).c_str(), 12);
  describe("Test POST with path and body and response");
  test_status(client.post("/api/device", device_body.c_str(), &response));
  test_response();
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
  test_status(client.put("/data/445654", post_body.c_str()));

  describe("Test PUT with path and body and response");
  test_status(client.put("/data/1241231", post_body.c_str(), &response));
  test_response();
}

void DELETE_tests()
{
  int temp = 37;
  long timestamp = 151241254122;
  // POST TESTS
  String post_body = "{ 'idsensor' : 18, 'value': " + temp;
  post_body = post_body + " , 'timestamp' :";
  post_body = post_body + timestamp;
  post_body = post_body + ", 'user' : 'Luismi'}";

  describe("Test DELETE with path");
  //note: requires a special endpoint
  test_status(client.del("/del/1241231"));

  describe("Test DELETE with path and body");
  test_status(client.del("/data/1241231", post_body.c_str()));
}

// Run the tests!
void loop()
{
  GET_tests();
  POST_tests();
}

void test_status(int statusCode)
{
  delay(test_delay);
  if (statusCode == 200 || statusCode == 201)
  {
    Serial.print("TEST RESULT: ok (");
    Serial.print(statusCode);
    Serial.println(")");
  }
  else
  {
    Serial.print("TEST RESULT: fail (");
    Serial.print(statusCode);
    Serial.println(")");
  }
}


void test_response()
{
  Serial.println("TEST RESULT: (response body = " + response + ")");
  response = "";
}

void describe(char *description)
{
  if (describe_tests)
    Serial.println(description);
}