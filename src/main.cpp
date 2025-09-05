#include <Arduino.h>
#include <Wire.h>

#define MCP23017_ADDR 0x20  // Adresse I2C du MCP23017
#define IODIRA 0x00         // Registre direction port A
#define GPPUA 0x0C          // Registre Pull-up port A
#define GPIOA 0x12          // Registre entrée/sortie port A

void setup() {
  Serial.begin(9600);
  Wire.begin();  // Initialisation I2C (Nano utilise A4 SDA, A5 SCL)

  // Configurer GPA0 en entrée (mettre le bit0 de IODIRA à 1)
  Wire.beginTransmission(MCP23017_ADDR);
  Wire.write(IODIRA);   // Sélection du registre IODIRA
  Wire.write(0x01);     // b00000001 → GPA0 en entrée, GPA1..7 en sortie
  Wire.endTransmission();

  // Activer la résistance pull-up sur GPA0
  Wire.beginTransmission(MCP23017_ADDR);
  Wire.write(GPPUA);    // Sélection du registre GPPUA
  Wire.write(0x01);     // b00000001 → pull-up activée sur GPA0
  Wire.endTransmission();
}

void loop() {
  uint8_t value = readRegister(GPIOA);   // Lire l'état des 8 bits du port A
  bool button = !(value & 0x01);         // Isoler le bit0 et inverser (LOW = appuyé)
  Serial.println(value);
  if (button) {
    Serial.println("Interrupteur APPUYE");
  } else {
    Serial.println("Interrupteur RELACHE");
  }

  delay(500);
}

// Fonction lecture registre MCP23017
uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(MCP23017_ADDR);
  Wire.write(reg);              // On indique le registre à lire
  Wire.endTransmission();
  Wire.requestFrom(MCP23017_ADDR, 1); // Lire 1 octet
  return Wire.read();
}