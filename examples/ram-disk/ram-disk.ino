#include "assert.h"
#include "fatfs.h"

RamIO drv{200, 512};  // 100 sector with 512 bytes
File file;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("SD begin");
  SD.setDriver(drv);
  if (!SD.mkfs()){
    Serial.println("mkfs error");
    while(true);
  }

  // start SD
  if (!SD.begin()){
    Serial.println("SD.begin() error");
    while(true);
  }

  Serial.println("Opening file");
  file = SD.open("test", FILE_WRITE);
  if (!file){
    Serial.println("Could not create file");
    while(true);
  }


  file.println("test");
  file.flush();

  Serial.println("Reading file");
  file.seek(0);
  uint8_t data[100] = {0};
  file.readBytes(data, 100);
  Serial.println((char*)data);

  Serial.println(file.size());

  file.close();


}

void loop() {}