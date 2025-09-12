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
    // RATE field is MSB bits 7..5. Map SamplingRate to rate_code.
    // Mapping: SR_512 -> code 0, SR_256 -> code 1, SR_128 -> code 2
    reg[0] &= ~0xE0; // clear bits 7..5
    uint8_t code = 0;
    switch (rate)
    {
        case SR_512: code = 0; break;
        case SR_256: code = 1; break;
        case SR_128: code = 2; break;
        default: code = 0; break;
    }
    reg[0] |= ((code & 0x07) << 5);
    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_CNFG_ECG, value);
}

bool MAX30003::setPgaGain(PgaGain gain)
{
    // ECG channel GAIN field is in CNFG_ECG MSB bits [4:3] per datasheet summary.
    uint8_t reg[3] = {0};
    if (!readRegister(REG_CNFG_ECG, reg, 3)) return false;

    // Clear bits 4..3 (0x18) and set according to gain
    reg[0] &= ~0x18;
    reg[0] |= ((uint8_t)gain & 0x03) << 3;

    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_CNFG_ECG, value);
}

bool MAX30003::setPowerMode(PowerMode mode)
{
    // Power mode bits are in CNFG_GEN (typical). We'll toggle low-power/shutdown bits
    uint8_t reg[3] = {0};
    if (!readRegister(REG_CNFG_GEN, reg, 3)) return false;

    // Use bits 0-1 as mode field (placeholder mapping)
    reg[0] &= 0xFC;
    reg[0] |= (uint8_t)mode & 0x03;

    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_CNFG_GEN, value);
}

bool MAX30003::configureRtor(RtorPreset preset)
{
    // RTOR threshold/tuning lives in CNFG_RTOR1/2. We'll adjust a few sensitivity bits.
    uint8_t reg[3] = {0};
    if (!readRegister(REG_CNFG_RTOR1, reg, 3)) return false;

    // Example: bits [5:3] control sensitivity. Map presets to small values.
    reg[0] &= 0xC7; // clear bits 5..3 (0b0011 1000)
    switch (preset)
    {
        case RtorPreset::AGGRESSIVE: reg[0] |= (0x03 << 3); break; // more sensitive
        case RtorPreset::CONSERVATIVE: reg[0] |= (0x01 << 3); break; // less sensitive
        case RtorPreset::DEFAULT:
        default: /* leave default bits */ break;
    }

    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_CNFG_RTOR1, value);
}

bool MAX30003::writeRawRegister(uint8_t regAddr, uint32_t value)
{
    // Convenience wrapper for raw 3-byte register writes
    return writeRegister(regAddr, value);
}

// New: filter configuration
bool MAX30003::setFilter(bool enableHighPassHalfHz, uint8_t lowPassHz)
{
    // Configure high-pass and low-pass in CNFG_ECG register.
    uint8_t reg[3] = {0};
    if (!readRegister(REG_CNFG_ECG, reg, 3)) return false;

    // DHPF (0.5Hz) is mid-byte bit 7, DLPF codes are mid-byte bits 6..5
    if (enableHighPassHalfHz) reg[1] |= 0x80; else reg[1] &= ~0x80;
    uint8_t code = 0;
    if (lowPassHz >= 140) code = 2;
    else if (lowPassHz >= 80) code = 1;
    else code = 0;
    reg[1] &= ~0x60;
    reg[1] |= ((code & 0x03) << 5);

    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_CNFG_ECG, value);
}

// Configure FIFO behaviour: enable/disable and watermark
bool MAX30003::configureFifo(bool enable, uint8_t watermark)
{
    // MNGR_DYN register controls FIFO watermark/interrupts (per datasheet)
    uint8_t reg[3] = {0};
    if (!readRegister(REG_MNGR_DYN, reg, 3)) return false;

    // bit0: FIFO enable, lower 6 bits of reg[1] = watermark level
    if (enable) reg[0] |= 0x01; else reg[0] &= ~0x01;
    reg[1] &= 0xC0;
    reg[1] |= (watermark & 0x3F);

    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return writeRegister(REG_MNGR_DYN, value);
}

// Helper to modify a single byte of a 3-byte register
bool MAX30003::modifyRegisterByte(uint8_t reg, uint8_t byteIndex, uint8_t mask, uint8_t value)
{
    uint8_t buf[3] = {0};
    if (!readRegister(reg, buf, 3)) return false;
    buf[byteIndex] = (buf[byteIndex] & ~mask) | (value & mask);
    uint32_t val = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    return writeRegister(reg, val);
}

// Modify an arbitrary bitfield inside a 3-byte register by absolute bit positions (23..0)
bool MAX30003::modifyRegisterField(uint8_t regAddr, uint8_t msb, uint8_t lsb, uint32_t value)
{
    if (msb > 23 || lsb > 23 || msb < lsb) return false;
    uint8_t buf[3] = {0};
    if (!readRegister(regAddr, buf, 3)) return false;
    uint32_t regval = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    uint32_t width = msb - lsb + 1;
    uint32_t mask = ((1UL << width) - 1) << lsb;
    regval = (regval & ~mask) | ((value << lsb) & mask);
    return writeRegister(regAddr, regval);
}

// Now reimplement functions using the exact bit positions from your mapping

bool MAX30003::enableUltraLowPowerLeadOn(bool enable)
{
    // EN_ULP_LON bits [23:22]
    return modifyRegisterField(REG_CNFG_GEN, 23, 22, enable ? 0x3 : 0x0);
}

bool MAX30003::enableEcgChannel(bool enable)
{
    // EN_ECG bit 19
    return modifyRegisterField(REG_CNFG_GEN, 19, 19, enable ? 1 : 0);
}

bool MAX30003::enableResistiveBias(bool enable, uint8_t rbiasv_selector, bool rbiasp, bool rbiasn)
{
    // RBIAS enable bits [10:9], RBIASV at [8:7], RBIASP at bit6, RBIASN at bit5
    if (!modifyRegisterField(REG_CNFG_GEN, 10, 9, enable ? 0x3 : 0x0)) return false;
    if (!modifyRegisterField(REG_CNFG_GEN, 8, 7, rbiasv_selector & 0x3)) return false;
    if (!modifyRegisterField(REG_CNFG_GEN, 6, 6, rbiasp ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_GEN, 5, 5, rbiasn ? 1 : 0)) return false;
    return true;
}

bool MAX30003::configureCalibration(bool enableVcal, uint8_t vmode, uint8_t vmag, uint8_t fcal, bool fifty, uint8_t thigh)
{
    // CNFG_CAL mapping: EN_VCAL bit22, VMODE bit21, VMAG bit20, FCAL bits19:17, FIFTY bit16, THIGH bits15:0
    if (!modifyRegisterField(REG_CNFG_CAL, 22, 22, enableVcal ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_CAL, 21, 21, vmode & 0x1)) return false;
    if (!modifyRegisterField(REG_CNFG_CAL, 20, 20, vmag & 0x1)) return false;
    if (!modifyRegisterField(REG_CNFG_CAL, 19, 17, fcal & 0x7)) return false;
    if (!modifyRegisterField(REG_CNFG_CAL, 16, 16, fifty ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_CAL, 15, 0, thigh & 0xFFFF)) return false;
    return true;
}

bool MAX30003::configureEmux(bool polarityInvert, bool openp, bool openn, uint8_t calp_sel, uint8_t caln_sel)
{
    // CNFG_EMUX mapping: POL bit23, OPENP 21, OPENN 20, CALP_SEL bits19:18, CALN_SEL bits17:16
    if (!modifyRegisterField(REG_CNFG_EMUX, 23, 23, polarityInvert ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_EMUX, 21, 21, openp ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_EMUX, 20, 20, openn ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_EMUX, 19, 18, calp_sel & 0x3)) return false;
    if (!modifyRegisterField(REG_CNFG_EMUX, 17, 16, caln_sel & 0x3)) return false;
    return true;
}

bool MAX30003::configureEcgChannel(uint8_t rate_code, uint8_t gain_code, bool dhpf_enable, uint8_t dlpf_code)
{
    // CNFG_ECG mapping: RATE bits23:22, GAIN bits17:16, DHPF bit14, DLPF bits13:12
    if (!modifyRegisterField(REG_CNFG_ECG, 23, 22, rate_code & 0x3)) return false;
    if (!modifyRegisterField(REG_CNFG_ECG, 17, 16, gain_code & 0x3)) return false;
    if (!modifyRegisterField(REG_CNFG_ECG, 14, 14, dhpf_enable ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_ECG, 13, 12, dlpf_code & 0x3)) return false;
    return true;
}

bool MAX30003::configureRtorFull(uint8_t wndw, uint8_t gain, bool enableRtor, uint8_t pavg, uint8_t ptsf, uint8_t hoff, uint8_t ravg, uint8_t rhsf)
{
    // CNFG_RTOR1: WNDW bits23:20, GAIN bits19:16, EN_RTOR bit15, PAVG bits13:12, PTSF bits11:8
    if (!modifyRegisterField(REG_CNFG_RTOR1, 23, 20, wndw & 0xF)) return false;
    if (!modifyRegisterField(REG_CNFG_RTOR1, 19, 16, gain & 0xF)) return false;
    if (!modifyRegisterField(REG_CNFG_RTOR1, 15, 15, enableRtor ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_CNFG_RTOR1, 13, 12, pavg & 0x3)) return false;
    if (!modifyRegisterField(REG_CNFG_RTOR1, 11, 8, ptsf & 0xF)) return false;
    // CNFG_RTOR2: HOFF bits21:16, RAVG bits13:12, RHSF bits10:8
    if (!modifyRegisterField(REG_CNFG_RTOR2, 21, 16, hoff & 0x3F)) return false;
    if (!modifyRegisterField(REG_CNFG_RTOR2, 13, 12, ravg & 0x3)) return false;
    if (!modifyRegisterField(REG_CNFG_RTOR2, 10, 8, rhsf & 0x7)) return false;
    return true;
}

bool MAX30003::configureInterrupts(uint8_t efit, bool clr_fast, bool clr_rrint, bool clr_samp, uint8_t samp_it)
{
    // MNGR_INT: EFIT bits23:19, CLR_FAST bit6, CLR_RRINT bits5:4, CLR_SAMP bit2, SAMP_IT bits1:0
    if (!modifyRegisterField(REG_MNGR_INT, 23, 19, efit & 0x1F)) return false;
    if (!modifyRegisterField(REG_MNGR_INT, 6, 6, clr_fast ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_MNGR_INT, 5, 4, clr_rrint ? 0x3 : 0x0)) return false;
    if (!modifyRegisterField(REG_MNGR_INT, 2, 2, clr_samp ? 1 : 0)) return false;
    if (!modifyRegisterField(REG_MNGR_INT, 1, 0, samp_it & 0x3)) return false;
    return true;
}

bool MAX30003::configureDynamicManager(bool fast_auto, uint8_t fast_th)
{
    // MNGR_DYN: FAST bits23:22, FAST_TH bits21:16
    if (!modifyRegisterField(REG_MNGR_DYN, 23, 22, fast_auto ? 0x3 : 0x0)) return false;
    if (!modifyRegisterField(REG_MNGR_DYN, 21, 16, fast_th & 0x3F)) return false;
    return true;
}
// CNFG_GEN helpers (removed older implementations; using bitfield modify variants above)

bool MAX30003::fifoReset()
{
    return writeRegister(REG_FIFO_RST, 0x000000);
}

bool MAX30003::syncCommand()
{
    return sync();
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
