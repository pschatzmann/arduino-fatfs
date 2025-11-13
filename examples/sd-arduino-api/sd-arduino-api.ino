// Example to use the Arduino SPI API to access  the SD drive
#include "SPI.h"
#include "fatfs.h"

// pins for audiokit
#define MISO 2
#define MOSI 15
#define SCLK 14
#define CS 13

File file;

void setup() {
  Serial.begin(115200);
  delay(3000);

  // start SPI and setup pins
  SPI.begin(SCLK, MISO, MOSI);
  
  // start SD
  if (!SD.begin(CS, SPI)) {
    Serial.println("SD.begin() failed!");
  }

  Serial.println("SD card initialized successfully!");

  // open and write file
  file = SD.open("test", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file!");
    while(true);
  }
  file.println("test");
  file.flush();

  // read back value
  file.seek(0);
  auto str = file.readStringUntil('\n');
  Serial.println(str);

  Serial.print("File size: ");
  Serial.println(file.size());

  file.close();

}

void loop() {}