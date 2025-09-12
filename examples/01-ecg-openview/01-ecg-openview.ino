// SPDX-License-Identifier: MIT
/**
 * @file 01-ecg-openview.ino
 * @brief ECG stream example that sends ECG samples to OpenView (Processing GUI).
 *
 * @author Ashwin Whitchurch <support@protocentral.com>
 * @copyright Copyright (c) 2025 Protocentral Electronics
 * @date 2025-09-12
 *
 * Arduino connections (defaults used in examples):
 *  - MISO : D12 (slave out)
 *  - MOSI : D11 (slave in)
 *  - SCLK : D13 (serial clock)
 *  - CS   : D4  (chip select)
 *  - INT1 : D2  (interrupt 1)
 *  - VCC  : +5V
 *  - GND  : GND
 */

#include <SPI.h>
#include "protocentral_max30003.h"

// Default: use pin 4 for CS and pin 2 for INT1
#define MAX30003_CS_PIN 4

#define CES_CMDIF_PKT_START_1   0x0A
#define CES_CMDIF_PKT_START_2   0xFA
#define CES_CMDIF_TYPE_DATA     0x02
#define CES_CMDIF_PKT_STOP      0x0B

#define DATA_LEN                12

#define ZERO                    0

volatile char DataPacket[DATA_LEN];
const char DataPacketFooter[2] = {ZERO, CES_CMDIF_PKT_STOP};
const char DataPacketHeader[5] = {CES_CMDIF_PKT_START_1, CES_CMDIF_PKT_START_2, DATA_LEN, ZERO, CES_CMDIF_TYPE_DATA};

uint8_t data_len = 0x0C;

MAX30003 max30003(MAX30003_CS_PIN);

void sendDataThroughUART(int32_t ecgSample)
{
  DataPacket[0] = ecgSample & 0xFF;
  DataPacket[1] = (ecgSample >> 8) & 0xFF;
  DataPacket[2] = (ecgSample >> 16) & 0xFF;
  DataPacket[3] = (ecgSample >> 24) & 0xFF;

  uint16_t rr = max30003.rrInterval();
  DataPacket[4] = rr & 0xFF;
  DataPacket[5] = (rr >> 8) & 0xFF;
  DataPacket[6] = 0x00;
  DataPacket[7] = 0x00;

  uint16_t hr = max30003.heartRate();
  DataPacket[8] = hr & 0xFF;
  DataPacket[9] = (hr >> 8) & 0xFF;
  DataPacket[10] = 0x00;
  DataPacket[11] = 0x00;

  //send packet header
  for (int i = 0; i < 5; i++) Serial.write(DataPacketHeader[i]);
  //send data
  for (int i = 0; i < DATA_LEN; i++) Serial.write(DataPacket[i]);
  //send footer
  for (int i = 0; i < 2; i++) Serial.write(DataPacketFooter[i]);
}


void setup()
{
    Serial.begin(57600); //Serial begin

    pinMode(MAX30003_CS_PIN,OUTPUT);
    digitalWrite(MAX30003_CS_PIN,HIGH); //disable device

    SPI.begin();

    bool ret = max30003.readDeviceID();
    if(ret){
      Serial.println("Max30003 read ID Success");
    }else{

      while(!ret){
        //stay here until the issue is fixed.
        ret = max30003.readDeviceID();
        Serial.println("Failed to read ID, please make sure all the pins are connected");
        delay(5000);
      }
    }

    Serial.println("Initialising the chip ...");
    max30003.begin();   // initialize MAX30003
}

void loop()
{
    int32_t sample = 0;
    max30003.readEcgSample(sample);
    max30003.updateHeartRate();
    sendDataThroughUART(sample);
    delay(8);
}
