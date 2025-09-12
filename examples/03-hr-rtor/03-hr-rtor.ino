// SPDX-License-Identifier: MIT
/**
 * @file 03-hr-rtor.ino
 * @brief Example demonstrating heart-rate (RTOR) interrupt handling.
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

#define INT_PIN 2

#define MAX30003_CS_PIN 4

MAX30003 max30003(MAX30003_CS_PIN);

bool rtorIntrFlag = false;
uint8_t statusReg[3];

void rtorInterruptHndlr(){
  rtorIntrFlag = true;
}

void enableInterruptPin(){

  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), rtorInterruptHndlr, CHANGE);
}

void setup()
{
    Serial.begin(115200); //Serial begin

    pinMode(MAX30003_CS_PIN,OUTPUT);
    digitalWrite(MAX30003_CS_PIN,HIGH); //disable device

    SPI.begin();

    bool ret = max30003.readDeviceID();
    if(ret){
      Serial.println("Max30003 ID Success");
    }else{

      while(!ret){
        //stay here untill the issue is fixed.
        ret = max30003.readDeviceID();
        Serial.println("Failed to read ID, please make sure all the pins are connected");
        delay(5000);
      }
    }

    Serial.println("Initialising the chip ...");
    // initialize MAX30003 in RTOR mode
    max30003.begin();
    max30003.writeRegister(REG_CNFG_GEN, 0x080004);
    max30003.writeRegister(REG_CNFG_CAL, 0x720000);
    max30003.writeRegister(REG_CNFG_EMUX, 0x0B0000);
    max30003.writeRegister(REG_CNFG_ECG, 0x805000);
    max30003.writeRegister(REG_CNFG_RTOR1, 0x3FC600);
    max30003.writeRegister(REG_EN_INT, 0x000401);
    max30003.writeRegister(REG_SYNCH, 0x000000);
    enableInterruptPin();
    max30003.readRegister(REG_STATUS, statusReg, 3);
}

void loop()
{
    if(rtorIntrFlag){
      rtorIntrFlag = false;
    max30003.readRegister(REG_STATUS, statusReg, 3);

      if(statusReg[1] & 0x04){
        max30003.updateHeartRate();
        Serial.print("Heart Rate  = ");
        Serial.println(max30003.heartRate());

        Serial.print("RR interval  = ");
        Serial.println(max30003.rrInterval());
      }
    }
}
