#include "assert.h"
#include "fatfs.h"

RamIO drv{200, 512};  // 100 sector with 512 bytes
File file;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("SD begin");

  // start SD
  if (!SD.begin(drv)){
    Serial.println("SD.begin() error");
    while(true);
  }

  Serial.println("Opening file...");
  file = SD.open("test", FILE_WRITE);
  if (!file){
    Serial.println("Could not create file");
    while(true);
  }

  // printing data
  file.println("test");
  file.flush();

  // reading back the result
  Serial.println("Reading file...");
  file.seek(0);
  uint8_t data[100] = {0};
  int len = file.readBytes(data, 100);
  Serial.print("bytes read: ");
  Serial.println(len);
  Serial.print("data: ");
  Serial.println((char*)data);
  Serial.print("file size: ");
  Serial.println(file.size());

  file.close();


}

void loop() {}