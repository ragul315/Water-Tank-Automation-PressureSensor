#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_ADS1X15.h>
#include <EEPROM.h>

Adafruit_ADS1115 ads; /* Use this for the 16-bit version */

const char *ssid = "YOUR WIFI SSID";
const char *password = "YOUR WIFI PASSWORD";
const char *mqtt_server = "MQTT SERVER ADDRESS";

WiFiClient espClient;
PubSubClient client(espClient);
char aa;
int device1, device2, device3;

String PId = "wta01";
char buffer[200];

String topicStr3 = "wtapkd";
const char *topic3 = topicStr3.c_str(); // pub
const char *mqttTopicmin = "wtapkd/TurnOnLevel";
const char *mqttTopicmax = "wtapkd/TurnOffLevel";

// const int sensorPin = A0;               // Analog pin connected to the pressure transducer
const int button = D5;
const int floatSensor = D0; // Digital pin connected to the floatsensor
const int MotorRelay = D4;  // Digital pin connected to the Relay

int waterLevel = 0;
const int NUM_READINGS = 500; // Number of readings to average

int readings[NUM_READINGS]; // Array to store the sensor readings
int readingIndex = 0;       // Index of the current reading
int total = 0;              // Running total of the readings
int sensorValue, mappedValue, average;

int ORIGINAL_MIN; // Minimum value from the analog sensor
int ORIGINAL_MAX; // Maximum value from the analog sensor
int tempTotal;

const int TARGET_MIN = 1;   // Minimum value for the target range
const int TARGET_MAX = 100; // Maximum value for the target range
int TurnOnLevel;            // values to turn on the water pump
int TurnOffLevel;           // values to turn off the water pump
int ctr = 0;
int fl = 1;

void setup()
{
    pinMode(A0, INPUT);
    pinMode(D0, INPUT);
    pinMode(D4, OUTPUT);
    pinMode(D5, INPUT);
    EEPROM.get(1, ORIGINAL_MIN);
    EEPROM.get(5, ORIGINAL_MAX);
    EEPROM.get(9, TurnOnLevel);
    EEPROM.get(13, TurnOffLevel);
    Serial.begin(9600);
    if (!ads.begin())
    {
        Serial.println("Failed to initialize ADS.");
        while (1)
            ;
    }
    // WIFI Setup
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    Serial.print("Local IP Address: ");
    Serial.println(WiFi.localIP());
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    // Water overflow intrupt
    attachInterrupt(digitalPinToInterrupt(floatSensor), overflow, CHANGE);
    attachInterrupt(digitalPinToInterrupt(button), initilise, CHANGE);
}
void initilise()
{
    tempTotal = 0;
    for (int i = 0; i < 100; i++)
    {
        sensorValue = ads.readADC_SingleEnded(0);
        tempTotal += sensorValue;
    }
    if (fl)
    {
        ORIGINAL_MIN = tempTotal / 100;
        EEPROM.put(1, ORIGINAL_MIN);
        fl = 0;
    }
    else
    {
        ORIGINAL_MAX = tempTotal / 100;
        EEPROM.put(5, ORIGINAL_MAX);
    }
}
void loop()
{
    ctr++;
    Serial.println(ctr);
    tklevel();

    if (!digitalRead(floatSensor))
    {
        if (average <= TurnOnLevel)
        {
            digitalWrite(MotorRelay, LOW);
        }
        if (average >= TurnOffLevel)
        {
            digitalWrite(MotorRelay, HIGH);
        }
    }

    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
    int wtalevel = 22;
    DynamicJsonDocument doc(6000);
    doc["level"] = average;
    doc["sensorvalue"] = sensorValue;
    doc["min"] = ORIGINAL_MIN;
    doc["max"] = ORIGINAL_MAX;
    doc["turnonlevel"] = TurnOnLevel;
    doc["turnofflevel"] = TurnOffLevel;
    Serial.println(average);
    serializeJson(doc, buffer);
    client.publish(topic3, buffer);
    Serial.println("got it");
}
void overflow()
{
    if (digitalRead(floatSensor) == HIGH)
    {
        digitalWrite(MotorRelay, HIGH);
    }
}
void tklevel()
{ /// Function to calculate Level returns the average value
    sensorValue = ads.readADC_SingleEnded(0);
    Serial.println("sensorValue");
    Serial.println(sensorValue);

    //   int sensorValue = Serial.parseInt();

    // Map the value to the target range
    mappedValue = mapValue(sensorValue);

    // Subtract the oldest reading from the total
    total -= readings[readingIndex];

    // Store the new reading in the array
    readings[readingIndex] = mappedValue;

    // Add the new reading to the total
    total += mappedValue;

    // Move to the next reading index
    readingIndex = (readingIndex + 1) % NUM_READINGS;

    // Calculate the average of the readings
    average = total / NUM_READINGS;

    // Limit the average within the target range
    average = constrain(average, TARGET_MIN, TARGET_MAX);

    // Print the stable reading to the Serial Monitor
    Serial.println(average);
    delay(200);
}
int mapValue(int value)
{
    // Map the value from the original range to the target range
    return map(value, ORIGINAL_MIN, ORIGINAL_MAX, TARGET_MIN, TARGET_MAX);
}
void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP322Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            // Once connected, publish an announcement...

            client.publish(topic3, "Hello from ESP8266");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Received message on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String topicStr = String(topic);

    if (topicStr.equals(mqttTopicmin))
    {
        // Convert payload to a string
        String message = "";
        for (int i = 0; i < length; i++)
        {
            message += (char)payload[i];
        }

        // Convert message to an integer
        TurnOnLevel = message.toInt();
        EEPROM.put(9, TurnOnLevel);
    }
    else if (topicStr.equals(mqttTopicmax))
    {
        // Convert payload to a string
        String message = "";
        for (int i = 0; i < length; i++)
        {
            message += (char)payload[i];
        }

        // Convert message to an integer
        TurnOffLevel = message.toInt();
        EEPROM.put(13, TurnOffLevel);
    }

    String aa = "";
    for (int i = 0; i < length; i++)
    {
        aa += (char)payload[i];
    }
    Serial.println(aa);
    DynamicJsonDocument doc(6000);
    DeserializationError error = deserializeJson(doc, aa); // Deserialize the message into the JSON document
    if (error)
    {
        Serial.print("Error deserializing JSON: ");
        Serial.println(error.c_str());
        return;
    }
}
