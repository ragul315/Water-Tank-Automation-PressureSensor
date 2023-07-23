/*
This code contains the default values to be writen in the RRPROM,
This code should be flashed before the main code is flashed
*/
#include <EEPROM.h>

int ORIGINAL_MIN = 210;  // Minimum value from the analog sensor
int ORIGINAL_MAX = 2000; // Maximum value from the analog sensor
int TurnOnLevel = 10;    // values to turn on the water pump
int TurnOffLevel = 90;   // values to turn off the water pump

void setup()
{
    Serial.begin(9600);

    EEPROM.put(1, ORIGINAL_MIN);
    EEPROM.put(5, ORIGINAL_MAX);
    EEPROM.put(9, TurnOnLevel);
    EEPROM.put(13, TurnOffLevel);

    EEPROM.get(1, ORIGINAL_MIN);
    EEPROM.get(5, ORIGINAL_MAX);
    EEPROM.get(9, TurnOnLevel);
    EEPROM.get(13, TurnOffLevel);
}
void loop()
{
    Serial.println(ORIGINAL_MAX);
    Serial.println(ORIGINAL_MIN);
    Serial.println(TurnOnLevel);
    Serial.println(TurnOffLevel);
    delay(3000);
}
