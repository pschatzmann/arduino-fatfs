// Example to use the Arduino SPI API to access  the SD drive
#include "SPI.h"
#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS 15

File file;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  // start SPI and setup pins
  SPI.begin(SCLK, MISO, MOSI);

  // start SD
  SD.begin(CS); // or use SD.begin(SD, SPI);

  // open and write file
  file = SD.open("test", FILE_WRITE);
  file.println("test");
  file.flush();

  // read back value
  file.seek(0);
  auto str = file.readStringUntil('\n');
  Serial.println(str);

  Serial.println(file.size());

  file.close();

}

void loop() {}