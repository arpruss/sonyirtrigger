#define IR_SEND_PIN PA7
#include <IRremote.hpp>
#include <EEPROM.h>

#define LED PB12
#define LIGHT_SENSOR PA6
#define CMDLINE_MAX 256

char cmdLine[256];
char* cmdLinePos = cmdLine;

uint32_t shutterDelay = 20;
uint32_t resetDelay = 2000;
uint32_t sensorThreshold = 300;
uint32_t displaySensor = 0;

bool ready = true;
uint32_t lastShutter = 0;

#define IDX_SHUTTER_DELAY 0
#define IDX_RESET_DELAY   2
#define IDX_THRESHOLD     4
#define IDX_DISPLAY       6
#define IDX_SAVED         7

void store16(uint32_t index, uint16_t value) {
  EEPROM8_storeValue(index, value&0xFF);
  EEPROM8_storeValue(index+1, value>>8);
}

uint16_t get16(uint32_t index, uint16_t def) {
  if (!EEPROM8_getValue(IDX_SAVED))
    return def;
  uint16_t x = EEPROM8_getValue(index);
  x |= (uint16_t)EEPROM8_getValue(index+1) << 8;
  return x;
}

uint8_t get8(uint32_t index, uint8_t def) {
  if (!EEPROM8_getValue(IDX_SAVED))
    return def;
  return EEPROM8_getValue(index);
}

void setup() {
  EEPROM8_init();
  shutterDelay = get16(IDX_SHUTTER_DELAY, shutterDelay);
  resetDelay = get16(IDX_RESET_DELAY, resetDelay);
  sensorThreshold = get16(IDX_THRESHOLD, resetDelay);
  displaySensor = get8(IDX_DISPLAY, displaySensor);
  
  pinMode(LED, OUTPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IR_SEND_PIN, OUTPUT);
  digitalWrite(LED, 1);
  IrSender.begin();
  Serial.begin();
  help();
}

void help() {
  Serial.println("HELP");
  char buf[80];
  sprintf(buf,   "sXXX: shutter delay (%u)", shutterDelay);
  Serial.println(buf);
  sprintf(buf,   "rXXX: reset delay (%u)", resetDelay);
  Serial.println(buf);
  sprintf(buf,   "tXXX: sensor threshold (%u)", sensorThreshold);
  Serial.println(buf);
  sprintf(buf,   "dX:   display sensor (%u)", displaySensor);
  Serial.println(buf);
  Serial.println("h/?:  help\n"
                 "S:    save");
}

void shutter() {
  IrSender.sendSony(0xB4B8F, 20);
  delay(40);
  IrSender.sendSony(0xB4B8F, 20);
  // manual shutter triggers at around 110-125 ms after this
  lastShutter = millis();
  ready = false;
} 

void save() {
  store16(IDX_SHUTTER_DELAY, shutterDelay);
  store16(IDX_RESET_DELAY, resetDelay);
  store16(IDX_THRESHOLD, sensorThreshold);
  EEPROM8_storeValue(IDX_DISPLAY, displaySensor);
  EEPROM8_storeValue(IDX_SAVED, 1);
  Serial.println("Saved!");
}

void readSerial() {
  bool newLine = false;
  while (Serial.available()) {
    int c = Serial.read();
    Serial.write(c);
    if (c == '\n' || c == '\r') {
      newLine = true;
      break;
    }
    if (cmdLinePos + 1 < cmdLine + sizeof(cmdLine)) {
      *cmdLinePos++ = c;      
    }    
  }
  if (! newLine)
    return;
  *cmdLinePos = 0;
  cmdLinePos = cmdLine;
  switch (cmdLine[0]) {
    case 'h':
    case '?':
      help();
      return;
    case 'S':
      save();
      return;
    case 's':
      shutterDelay = atoi(cmdLine+1);
      ready = true;
      return;
    case 'r':
      resetDelay = atoi(cmdLine+1);
      return;
    case 't':
      sensorThreshold = atoi(cmdLine+1);
      return;
    case 'd':
      displaySensor = atoi(cmdLine+1);
      return;
  }
}

void loop() {
  readSerial();
  int x = analogRead(LIGHT_SENSOR);
  
  if (displaySensor) {
    char buf[64];
    sprintf(buf, "sensor = %4u", x);
    Serial.println(buf);
  }
  
  if (!ready && millis()-lastShutter >= resetDelay)
    ready = true;

  if (ready && x < sensorThreshold) {
    Serial.println("trigger");
    delay(shutterDelay);
    digitalWrite(LED,0);
    shutter();
    digitalWrite(LED,1);
  }
}

void xloop() {
  digitalWrite(LED, 1);
  shutter();
  digitalWrite(LED, 0);
  delay(125);
  digitalWrite(LED, 1);
  delay(5000);
}
