//////////////////////////////////////////////////////////////////////////////////////////
//
//  Demo code for the MAX30003 breakout board
//
//  Arduino connections:
//
//  |MAX30003 pin label| Pin Function         |Arduino Connection|
//  |----------------- |:--------------------:|-----------------:|
//  | MISO             | Slave Out            |  D12             |
//  | MOSI             | Slave In             |  D11             |
//  | SCLK             | Serial Clock         |  D13             |
//  | CS               | Chip Select          |  D4              |
//  | VCC              | Digital VDD          |  +5V             |
//  | GND              | Digital Gnd          |  Gnd             |
//  | FCLK             | 32K CLOCK            |  -               |
//  | INT1             | Interrupt1           |  D2               |
//  | INT2             | Interrupt2           |  -               |
//
//  This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//  For information on how to use, visit https://github.com/Protocentral/protocentral-max30003-arduino
//
/////////////////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: MIT
/**
 * @file protocentral_Max30003.cpp
 * @brief Implementation of the MAX30003 Arduino library.
 *
 * @author Ashwin Whitchurch <support@protocentral.com>
 * @copyright Copyright (c) 2025 Protocentral Electronics
 */

#include <SPI.h>
#include "protocentral_max30003.h"

static constexpr uint32_t MAX30003_SPI_SPEED = 2000000UL;

using namespace ::std;

MAX30003::MAX30003(int cs_pin, SPIClass &spi) noexcept
    : _spi(spi), _settings(MAX30003_SPI_SPEED, MSBFIRST, SPI_MODE0), _cs_pin(cs_pin)
{
    pinMode(_cs_pin, OUTPUT);
    digitalWrite(_cs_pin, HIGH);
}

void MAX30003::chipSelect(bool active) const noexcept
{
    digitalWrite(_cs_pin, active ? LOW : HIGH);
}

bool MAX30003::writeRegister(uint8_t WRITE_ADDRESS, uint32_t data)
{
    uint8_t header = (WRITE_ADDRESS << 1) | WREG;
    _spi.beginTransaction(_settings);
    chipSelect(true);
    delay(2);
    _spi.transfer(header);
    _spi.transfer((data >> 16) & 0xFF);
    _spi.transfer((data >> 8) & 0xFF);
    _spi.transfer(data & 0xFF);
    delay(2);
    chipSelect(false);
    _spi.endTransaction();
    return true;
}

bool MAX30003::readRegister(uint8_t Reg_address, uint8_t *buff, size_t len)
{
    uint8_t header = (Reg_address << 1) | RREG;
    _spi.beginTransaction(_settings);
    chipSelect(true);
    _spi.transfer(header);
    for (size_t i = 0; i < len; ++i)
    {
        buff[i] = _spi.transfer(0xFF);
    }
    chipSelect(false);
    _spi.endTransaction();
    return true;
}

bool MAX30003::readDeviceID()
{
    uint8_t buf[3] = {0};
    if (!readRegister(REG_INFO, buf, 3)) return false;
    // device revision in upper nibble of first byte is 0x5 for MAX30003
    return ((buf[0] & 0xF0) == 0x50);
}

void MAX30003::reset()
{
    writeRegister(REG_SW_RST, 0x000000);
    delay(100);
}

bool MAX30003::sync()
{
    return writeRegister(REG_SYNCH, 0x000000);
}

bool MAX30003::begin()
{
    reset();
    delay(100);
    writeRegister(REG_CNFG_GEN, 0x081007);
    delay(50);
    writeRegister(REG_CNFG_CAL, 0x720000);
    delay(50);
    writeRegister(REG_CNFG_EMUX, 0x0B0000);
    delay(50);
    writeRegister(REG_CNFG_ECG, 0x805000);
    delay(50);
    writeRegister(REG_CNFG_RTOR1, 0x3FC600);
    sync();
    delay(50);
    return true;
}

bool MAX30003::setSamplingRate(SamplingRate rate)
{
    uint8_t reg[3] = {0};
    if (!readRegister(REG_CNFG_ECG, reg, 3)) return false;
    // modify bits in MSB according to rate
    reg[0] &= 0x3F; // clear sample rate bits (example mask)
    switch (rate)
    {
        case SR_128: reg[0] |= 0x80; break;
        case SR_256: reg[0] |= 0x40; break;
        case SR_512: /* leave 0 */ break;
    }
    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_CNFG_ECG, value);
}

bool MAX30003::readEcgBurst(uint16_t count, uint8_t *out)
{
    // read count*3 bytes from ECG FIFO burst
    _spi.beginTransaction(_settings);
    chipSelect(true);
    uint8_t header = (REG_ECG_FIFO_BURST << 1) | RREG;
    _spi.transfer(header);
    for (uint32_t i = 0; i < (uint32_t)count * 3U; ++i)
    {
        out[i] = _spi.transfer(0x00);
    }
    chipSelect(false);
    _spi.endTransaction();
    return true;
}

bool MAX30003::readEcgSample(int32_t &sample)
{
    uint8_t b[3] = {0};
    if (!readRegister(REG_ECG_FIFO, b, 3)) return false;
    // 24-bit signed value: b0 MSB .. b2 LSB
    int32_t val = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
    // sign-extend 24-bit to 32-bit
    if (val & 0x800000) val |= 0xFF000000;
    sample = val;
    return true;
}

bool MAX30003::updateHeartRate()
{
    uint8_t reg[3] = {0};
    if (!readRegister(REG_RTOR, reg, 3)) return false;
    uint32_t rtor = ((uint32_t)reg[0] << 8) | reg[1];
    rtor = (rtor >> 2) & 0x3FFF;
    if (rtor == 0) return false;
    float hr = 60.0F / ( (float)rtor * 0.0078125F );
    _heartRate = (uint16_t)hr;
    _rrInterval = (uint16_t)( (float)rtor * 7.8125F );
    return true;
}
