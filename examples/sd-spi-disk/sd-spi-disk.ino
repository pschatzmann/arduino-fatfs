#include "SPI.h"
#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS 15

SDArduinoSPIIO sd{CS, SPI};  // driver managing CS and assign SPI
SDClass SD1{sd};             // SD and assign driver
File file;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // start SPI and setup pins
  SPI.begin(SCLK, MISO, MOSI);

  // start SD
  SD1.begin(drv);

  // open and write file
  file = SD1.open("test", FILE_WRITE);
  file.println("test");
  file.flush();

  // read back value
  file.seek(0);
  auto str = file.readStringUntil('\n');
  Serial.println(str);
  assert(str == "test");

  Serial.println(file.size());
  file.close();
}

void loop() {}