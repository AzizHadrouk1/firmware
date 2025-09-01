#include <dummy.h>

#define switche 23  // GPIO14 (D5 sur NodeMCU). Tu peux changer selon ton câblage.

void setup() {
  Serial.begin(115200);
  pinMode(switche, INPUT_PULLUP); // ESP8266 ne supporte pas INPUT_PULLDOWN
}

void loop() {
  int state = digitalRead(switche);

  if (state == LOW) {
    Serial.println("Bouton PRESSE");   // appuyé = LOW car relié à GND
  } else {
    Serial.println("Bouton RELACHE"); // relâché = HIGH grâce au pull-up interne
  }

  delay(300);
}