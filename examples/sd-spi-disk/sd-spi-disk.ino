#include "SPI.h"
#include "fatfs.h"

#define MISO 2
#define MOSI 15
#define SCLK 14
#define CS 13

ArduinoSpiIO sd{CS, SPI};  // driver managing CS and assign SPI
SDClass SD1{sd};             // SD and assign driver
File file;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // start SPI and setup pins
  SPI.begin(SCLK, MISO, MOSI);

  // start SD
  if (!SD1.begin()) {
    Serial.println("SD.begin() failed!");
    while (true);
  }

  // open and write file
  file = SD1.open("test", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file!");
    while (true);
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