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
//  | CS               | Chip Select          |  D7              |
//  | VCC              | Digital VDD          |  +5V             |
//  | GND              | Digital Gnd          |  Gnd             |
//  | FCLK             | 32K CLOCK            |  -               |
//  | INT1             | Interrupt1           |  02               |
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

#include<SPI.h>
#include "protocentral_max30003.h"

#define MAX30003_SPI_SPEED 2000000

SPISettings SPI_SETTINGS(MAX30003_SPI_SPEED, MSBFIRST, SPI_MODE0); 

MAX30003::MAX30003(int cs_pin)
{
    _cs_pin=cs_pin;
    pinMode(_cs_pin, OUTPUT);
    digitalWrite(_cs_pin,HIGH);
}

void MAX30003::max30003RegWrite (unsigned char WRITE_ADDRESS, unsigned long data)
{
    // now combine the register address and the command into one byte:
    byte dataToSend = (WRITE_ADDRESS<<1) | WREG;

    SPI.beginTransaction(SPI_SETTINGS);
     // take the chip select low to select the device:
    digitalWrite(_cs_pin, LOW);

    delay(2);
    SPI.transfer(dataToSend);
    SPI.transfer(data>>16);
    SPI.transfer(data>>8);
    SPI.transfer(data);
    delay(2);

    // take the chip select high to de-select:
    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();
}


void MAX30003::max30003SwReset(void)
{
    max30003RegWrite(SW_RST,0x000000);
    delay(100);
}

void MAX30003::max30003Synch(void)
{
    max30003RegWrite(SYNCH,0x000000);
}

void MAX30003::max30003RegRead(uint8_t Reg_address, uint8_t * buff)
{
    uint8_t spiTxBuff;

    SPI.beginTransaction(SPI_SETTINGS);
    digitalWrite(_cs_pin, LOW);

    spiTxBuff = (Reg_address<<1 ) | RREG;
    SPI.transfer(spiTxBuff); //Send register location

    for ( int i = 0; i < 3; i++)
    {
       buff[i] = SPI.transfer(0xff);
    }

    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();
}


bool MAX30003::max30003ReadInfo(void)
{
    uint8_t spiTxBuff;
    uint8_t readBuff[4] ;

    SPI.beginTransaction(SPI_SETTINGS);
    digitalWrite(_cs_pin, LOW);

    spiTxBuff = (INFO << 1 ) | RREG;
    SPI.transfer(spiTxBuff); //Send register location

    for ( int i = 0; i < 3; i++)
    {
       readBuff[i] = SPI.transfer(0xff);
    }

    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();

    if((readBuff[0]&0xf0) == 0x50 ){

      Serial.println("max30003 is ready");
      Serial.print("Rev ID:  ");
      Serial.println((readBuff[0]&0xf0));

      return true;
    }else{

      Serial.println("max30003 read info error\n");
      return false;
    }

    return false;
}

void MAX30003::max30003ReadData(int num_samples, uint8_t * readBuffer)
{
    uint8_t spiTxBuff;

    SPI.beginTransaction(SPI_SETTINGS);
    digitalWrite(_cs_pin, LOW);

    spiTxBuff = (ECG_FIFO_BURST<<1 ) | RREG;
    SPI.transfer(spiTxBuff); //Send register location

    for ( int i = 0; i < num_samples*3; ++i)
    {
      readBuffer[i] = SPI.transfer(0x00);
    }

    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();
}

void MAX30003::max30003Begin()
{
    max30003SwReset();
    delay(100);
    max30003RegWrite(CNFG_GEN, 0x081007);
    delay(100);
    max30003RegWrite(CNFG_CAL, 0x720000);  // 0x700000
    delay(100);
    max30003RegWrite(CNFG_EMUX,0x0B0000);
    delay(100);
    max30003RegWrite(CNFG_ECG, 0x805000);  // d23 - d22 : 10 for 250sps , 00:500 sps
    delay(100);

    max30003RegWrite(CNFG_RTOR1,0x3fc600);
    max30003Synch();
    delay(100);
}

void MAX30003::max30003BeginRtorMode()
{
    max30003SwReset();
    delay(100);
    max30003RegWrite(CNFG_GEN, 0x080004);
    delay(100);
    max30003RegWrite(CNFG_CAL, 0x720000);  // 0x700000
    delay(100);
    max30003RegWrite(CNFG_EMUX,0x0B0000);
    delay(100);
    max30003RegWrite(CNFG_ECG, 0x805000);  // d23 - d22 : 10 for 250sps , 00:500 sps
    delay(100);
    max30003RegWrite(CNFG_RTOR1,0x3fc600);
    delay(100);
    max30003RegWrite(EN_INT, 0x000401);
    delay(100);
    max30003Synch();
    delay(100);
}

//not tested
void MAX30003::max30003SetsamplingRate(uint16_t samplingRate)
{
    uint8_t regBuff[4] = {0};
    max30003RegRead(CNFG_ECG, regBuff);

    switch(samplingRate){
        case SAMPLINGRATE_128:
                regBuff[0] = (regBuff[0] | 0x80 );
            break;

        case SAMPLINGRATE_256:
                regBuff[0] = (regBuff[0] | 0x40 );
            break;

        case SAMPLINGRATE_512:
                regBuff[0] = (regBuff[0] | 0x00 );
            break;

        default :
            Serial.println("Wrong samplingRate, please choose between 128, 256 or 512");
            break;
    }

    unsigned long cnfgEcg;
    memcpy(&cnfgEcg, regBuff, 4);

    Serial.print(" cnfg ECG ");
    Serial.println((cnfgEcg));
    max30003RegWrite(CNFG_ECG, (cnfgEcg >> 8));
}

void  MAX30003::getEcgSamples(void)
{
    uint8_t regReadBuff[4];
    max30003RegRead(ECG_FIFO, regReadBuff);

    unsigned long data0 = (unsigned long) (regReadBuff[0]);
    data0 = data0 <<24;
    unsigned long data1 = (unsigned long) (regReadBuff[1]);
    data1 = data1 <<16;
    unsigned long data2 = (unsigned long) (regReadBuff[2]);
    data2 = data2 >>6;
    data2 = data2 & 0x03;

    unsigned long data = (unsigned long) (data0 | data1 | data2);
    ecgdata = (signed long) (data);
}


void MAX30003::getHRandRR(void)
{
    uint8_t regReadBuff[4];
    max30003RegRead(RTOR, regReadBuff);

    unsigned long RTOR_msb = (unsigned long) (regReadBuff[0]);
    unsigned char RTOR_lsb = (unsigned char) (regReadBuff[1]);
    unsigned long rtor = (RTOR_msb<<8 | RTOR_lsb);
    rtor = ((rtor >>2) & 0x3fff) ;

    float hr =  60 /((float)rtor*0.0078125);
    heartRate = (unsigned int)hr;

    unsigned int RR = (unsigned int)rtor* (7.8125) ;  //8ms
    RRinterval = RR;
}
