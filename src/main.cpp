#include <HTTPClient.h>
#include "ArduinoJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <pwmWrite.h>
#include <PubSubClient.h>

//Funciones
int ping(int TriggerPin, int EchoPin);
void postValorSensor(int tipo, int idSensor, float value, int boxNumber);
void postValorActuador(float status, int idActuator);

//Variables 
int retraso = 0;
int cont = 150;
int boxNumber = 0;
bool ready = false;
String content2 = "";

// Replace 0 by ID of this current device
const int DEVICE_ID = 1;

int test_delay = 500; // so we don't spam the API
boolean describe_tests = true;

// Replace 0.0.0.0 by your server local IP (ipconfig [windows] or ifconfig [Linux o MacOS] gets IP assigned to your PC)
String serverName = "http://192.168.199.19/";
HTTPClient http;

// Replace WifiName and WifiPassword by your WiFi credentials
#define STASSID "Xiaomi 11T Pro"
#define STAPSK "jajajaxd"

// NTP (Net time protocol) settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// MQTT configuration
WiFiClient espClient;
PubSubClient client(espClient);

// Server IP, where de MQTT broker is deployed
const char *MQTT_BROKER_ADRESS = "192.168.199.19";
const uint16_t MQTT_PORT = 1883;

// Name for this MQTT client
const char *MQTT_CLIENT_NAME = "mqttChannel1";

// Pinout settings
const int servoPin = 25;
const int ultrasound1PinTRIG = 23;
const int ultrasound1PinECHO = 22;
const int ultrasound2PinTRIG = 17;
const int ultrasound2PinECHO = 16;

// Property initialization for servo movement using PWM signal
Pwm pwm = Pwm();

// callback a ejecutar cuando se recibe un mensaje
// en este ejemplo, muestra por serial el mensaje recibido
void OnMqttReceived(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received on ");
  Serial.print(topic);
  Serial.print(": ");

  String content = "";
  for (size_t i = 0; i < length; i++)
  {
    content.concat((char)payload[i]);
  }
  Serial.print(content);
  Serial.println();

  content2 = content;
   
}

// inicia la comunicacion MQTT
// inicia establece el servidor y el callback al recibir un mensaje
void InitMqtt()
{
  client.setServer(MQTT_BROKER_ADRESS, MQTT_PORT);
  client.setCallback(OnMqttReceived);
}

// Setup
void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(STASSID);

  /* Explicitly set the ESP32 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  InitMqtt();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup!");

  // Init and get the time
  timeClient.begin();

  //Init pins
  pinMode(ultrasound1PinTRIG, OUTPUT);
  pinMode(ultrasound1PinECHO, INPUT);
  pinMode(ultrasound2PinTRIG, OUTPUT);
  pinMode(ultrasound2PinECHO, INPUT);
}

String response;

String serializeSensorValueBody(int idSensor, long timestamp, float value, int boxNumber)
{
  // StaticJsonObject allocates memory on the stack, it can be
  // replaced by DynamicJsonDocument which allocates in the heap.
  //
  DynamicJsonDocument doc(2048);

  // Add values in the document
  //
  doc["idSensor"] = idSensor;
  doc["timestamp"] = timestamp;
  doc["value"] = value;
  doc["removed"] = false;
  doc["boxNumber"] = boxNumber;

  // Generate the minified JSON and send it to the Serial port.
  //
  String output;
  serializeJson(doc, output);
  Serial.println(output);

  return output;
}

String serializeActuatorStatusBody(float status, bool statusBinary, int idActuator, long timestamp)
{
  DynamicJsonDocument doc(2048);

  doc["status"] = status;
  doc["statusBinary"] = statusBinary;
  doc["idActuator"] = idActuator;
  doc["timestamp"] = timestamp;
  doc["removed"] = false;

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
    int idActuatorState = doc["idActuatorState"];
    float status = doc["status"];
    bool statusBinary = doc["statusBinary"];
    int idActuator = doc["idActuator"];
    long timestamp = doc["timestamp"];

    Serial.println(("Actuator status deserialized: [idActuatorState: " + String(idActuatorState) + ", status: " + String(status) + ", statusBinary: " + String(statusBinary) + ", idActuator" + String(idActuator) + ", timestamp: " + String(timestamp) + "]").c_str());
  }
}

void deserializeDeviceBody(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
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

    Serial.println(("Device deserialized: [idDevice: " + String(idDevice) + ", name: " + name + ", deviceSerialId: " + deviceSerialId + ", mqttChannel" + mqttChannel + ", idGroup: " + idGroup + "]").c_str());
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void deserializeSensorsFromDevice(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    // allocate the memory for the document
    DynamicJsonDocument doc(ESP.getMaxAllocHeap());

    // parse a JSON array
    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // extract the values
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject sensor : array)
    {
      int idSensor = sensor["idSensor"];
      String name = sensor["name"];
      String sensorType = sensor["sensorType"];
      int idDevice = sensor["idDevice"];

      Serial.println(("Sensor deserialized: [idSensor: " + String(idSensor) + ", name: " + name + ", sensorType: " + sensorType + ", idDevice: " + String(idDevice) + "]").c_str());
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void deserializeActuatorsFromDevice(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    // allocate the memory for the document
    DynamicJsonDocument doc(ESP.getMaxAllocHeap());

    // parse a JSON array
    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // extract the values
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject sensor : array)
    {
      int idActuator = sensor["idActuator"];
      String name = sensor["name"];
      String actuatorType = sensor["actuatorType"];
      int idDevice = sensor["idDevice"];

      Serial.println(("Actuator deserialized: [idActuator: " + String(idActuator) + ", name: " + name + ", actuatorType: " + actuatorType + ", idDevice: " + String(idDevice) + "]").c_str());
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void test_response(int httpResponseCode)
{
  //delay(test_delay);
  if (httpResponseCode > 0)
  {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload + "\n");
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
    Serial.print(description);
}

void GET_tests()
{
  describe("Test GET full device info");
  String serverPath = serverName + "api/devices/" + String(DEVICE_ID);
  http.begin(serverPath.c_str());
  // test_response(http.GET());
  deserializeDeviceBody(http.GET());

  delay(2000);

  describe("Test GET sensors from deviceID");
  serverPath = serverName + "api/devices/sensors/" + String(DEVICE_ID);
  http.begin(serverPath.c_str());
  deserializeSensorsFromDevice(http.GET());

  delay(2000);

  describe("Test GET actuators from deviceID");
  serverPath = serverName + "api/devices/actuators/" + String(DEVICE_ID);
  http.begin(serverPath.c_str());
  deserializeActuatorsFromDevice(http.GET());

  delay(2000);

  describe("Test GET sensors from deviceID and Type");
  serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/sensors/Temperature";
  http.begin(serverPath.c_str());
  deserializeSensorsFromDevice(http.GET());

  delay(2000);

  describe("Test GET actuators from deviceID");
  serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/actuators/Relay";
  http.begin(serverPath.c_str());
  deserializeActuatorsFromDevice(http.GET());
}

void POST_tests()
{
  String actuator_status_body = serializeActuatorStatusBody(random(2000, 4000) / 100, true, 1, millis());
  describe("Test POST with actuator status");
  String serverPath = serverName + "api/actuatorStatus";
  http.begin(serverPath.c_str());
  test_response(http.POST(actuator_status_body));

  delay(2000);

  String sensor_value_body = serializeSensorValueBody(18, millis(), random(2000, 4000) / 100, 0);
  describe("Test POST with sensor value");
  serverPath = serverName + "api/sensorValues";
  http.begin(serverPath.c_str());
  test_response(http.POST(sensor_value_body));

  // String device_body = serializeDeviceBody(String(DEVICE_ID), ("Name_" + String(DEVICE_ID)).c_str(), ("mqtt_" + String(DEVICE_ID)).c_str(), 12);
  // describe("Test POST with path and body and response");
  // serverPath = serverName + "api/device";
  // http.begin(serverPath.c_str());
  // test_response(http.POST(actuator_states_body));
}

// conecta o reconecta al MQTT
// consigue conectar -> suscribe a topic y publica un mensaje
// no -> espera 5 segundos
void ConnectMqtt()
{
  Serial.print("Starting MQTT connection...");
  if (client.connect(MQTT_CLIENT_NAME))
  {
    client.subscribe(MQTT_CLIENT_NAME);
    client.publish(MQTT_CLIENT_NAME, "connected");
  }
  else
  {
    Serial.print("Failed MQTT connection, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");

    delay(5000);
  }
}

// gestiona la comunicación MQTT
// comprueba que el cliente está conectado
// no -> intenta reconectar
// si -> llama al MQTT loop
void HandleMqtt()
{
  if (!client.connected())
  {
    ConnectMqtt();
  }
  
  client.loop();
}

// Run the tests!
void loop()
{
  HandleMqtt();

  int anchura = ping(ultrasound1PinTRIG, ultrasound1PinECHO);
  int altura = ping(ultrasound2PinTRIG, ultrasound2PinECHO);
  
  cont--;
  if ((anchura < 60 || altura < 60) && cont == 0){
    Serial.println("Caja leida\n");
    boxNumber++; //Esperando que haya pasado de verdad una nueva caja y no sea la misma
    postValorSensor(2, 1, 70 - altura, boxNumber);
    postValorSensor(1, 2, 70 - anchura, boxNumber);
    cont = 150; //~ 9s
    retraso = 50; //~ 3s
    ready = true;
  } else if (cont == 0){
    Serial.println("Esperando caja...");
    cont = 150; //Reset en caso de que no haya objeto
  }

  retraso--;

  if (retraso == 0 && ready) { //Pasados 3 segundos, el actuador ya puede funcionar
    //Recibir MQTT
    Serial.println("Clasificando caja:\n");
    int angulo = content2.toInt();
    pwm.writeServo(servoPin, angulo);        // set the servo position (degrees)
    postValorActuador(angulo, 1);
    ready = false;
  }
  
}

void postValorSensor(int tipo, int idSensor, float value, int boxNumber){
  String t = tipo == 1 ? "anchura" : "altura";
  String m = "Enviado POST sensorValue de " + t + " - ";
  char* mensaje = strcpy(new char[m.length() + 1], m.c_str());
  String sensor_value_body = serializeSensorValueBody(idSensor, timeClient.getEpochTime(), value, boxNumber);
  describe(mensaje);
  String serverPath = serverName + "api/sensorValues";
  http.begin(serverPath.c_str());
  test_response(http.POST(sensor_value_body));
}

void postValorActuador(float status, int idActuator){
  String actuator_status_body = serializeActuatorStatusBody(status, true, idActuator, timeClient.getEpochTime());
  describe("Enviado POST actuatorStatus - ");
  String serverPath = serverName + "api/actuatorStatus";
  http.begin(serverPath.c_str());
  test_response(http.POST(actuator_status_body));
}

int ping(int TriggerPin, int EchoPin) {
  long duration, distanceCm;
  
  digitalWrite(TriggerPin, LOW);  //para generar un pulso limpio ponemos a LOW 4us
  delayMicroseconds(4);
  digitalWrite(TriggerPin, HIGH);  //generamos Trigger (disparo) de 10us
  delayMicroseconds(10);
  digitalWrite(TriggerPin, LOW);
  
  duration = pulseIn(EchoPin, HIGH);  //medimos el tiempo entre pulsos, en microsegundos
  
  distanceCm = duration * 10 / 292/ 2;   //convertimos a distancia, en cm
  return distanceCm;
}