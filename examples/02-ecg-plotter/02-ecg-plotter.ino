// SPDX-License-Identifier: MIT
/**
 * @file 02-ecg-plotter.ino
 * @brief ECG stream example that prints ECG samples to Arduino Serial Plotter.
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

#define MAX30003_CS_PIN 4

MAX30003 max30003(MAX30003_CS_PIN);

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
        //stay here untill the issue is fixed.
        ret = max30003.readDeviceID();
        Serial.println("Failed to read ID, please make sure all the pins are connected");
        delay(10000);
      }
    }

    Serial.println("Initialising the chip ...");
  max30003.begin();   // initialize MAX30003
  // configure using new API
  max30003.setSamplingRate(SR_256);
  max30003.setPgaGain(PgaGain::GAIN_80);
  max30003.configureEcgChannel(1, static_cast<uint8_t>(PgaGain::GAIN_80), true, 0);
}

void loop()
{
    int32_t sample = 0;
    max30003.readEcgSample(sample);
    Serial.println(sample);
    delay(8);
}
