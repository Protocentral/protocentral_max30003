#include "Protocentral_MAX30003.h" 
#include <SPI.h>
#include <TimerOne.h>

uint8_t global_fclk ;
    //<<constructor>> 
Protocentral_MAX30003::Protocentral_MAX30003(){
}
     
    //<<destructor>>
Protocentral_MAX30003::~Protocentral_MAX30003(){/*nothing to destruct*/}

void timerIsr(void)
{
  digitalWrite( global_fclk, digitalRead(global_fclk ) ^ 1 ); // toggle Digital6 attached to FCLK  of MAX30003
}
  
bool Protocentral_MAX30003::begin(uint8_t cs_pin, uint8_t fclk_pin)
{
  m_fclk_pin = fclk_pin;
  global_fclk = fclk_pin;
  m_cs_pin = cs_pin;

  pinMode(m_cs_pin,OUTPUT);
  digitalWrite(m_cs_pin,HIGH); //disable device

  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  
  pinMode(m_fclk_pin,OUTPUT);

  Timer1.initialize(16);              // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( timerIsr ); // attach the service routine here
  
/*  //read the chip id
  if(!(Read_ID())){
    return false;
  }
*/
  SW_Reset();
  delay(100);
  Reg_Write(CNFG_GEN, 0x080004);
  delay(100);
  Reg_Write(CNFG_CAL, 0x720000);  // 0x700000  
  delay(100);
  Reg_Write(CNFG_EMUX,0x000000);
  delay(100);
  Reg_Write(CNFG_ECG, 0x401000);  // d23 - d22 : 10 for 250sps , 00:500 sps
  delay(100);

  Reg_Write(CNFG_RTOR1,0x3fc600);
  Synch();
  delay(100);

  Reg_Write(FIFO_RST,0x000000);

  return true;
}

void Protocentral_MAX30003::SW_Reset(void)
{
  Reg_Write(SW_RST,0x000000);     
  delay(100);
}

void Protocentral_MAX30003::Synch(void)
{
  Reg_Write(SYNCH,0x000000);
}

void Protocentral_MAX30003::Reg_Write (uint8_t WRITE_ADDRESS, unsigned long data)
{
 
  // now combine the register address and the command into one byte:
   byte dataToSend = (WRITE_ADDRESS<<1) | WREG;

   // take the chip select low to select the device:
   digitalWrite(m_cs_pin, LOW);
   
   delay(2);
   SPI.transfer(dataToSend);   //Send register location
   SPI.transfer(data>>16);     //number of register to wr
   SPI.transfer(data>>8);      //number of register to wr
   SPI.transfer(data);      //Send value to record into register
   delay(2);
   
   // take the chip select high to de-select:
    digitalWrite(m_cs_pin, HIGH);
}

void Protocentral_MAX30003::Read_Data(int num_samples)
{
  uint8_t SPI_TX_Buff;

  digitalWrite(m_cs_pin, LOW);   

  SPI_TX_Buff = (ECG_FIFO_BURST<<1 ) | RREG;
  SPI.transfer(SPI_TX_Buff); //Send register location

  for ( i = 0; i < num_samples*3; ++i)
  {
    SPI_temp_Burst[i] = SPI.transfer(0x00);
  }
  
  digitalWrite(m_cs_pin, HIGH);  
}

void Protocentral_MAX30003::send_data_serial(void)
{
  unsigned long data0 = (unsigned long) (SPI_temp_32b[0]);
  data0 = data0 <<24;
  unsigned long data1 = (unsigned long) (SPI_temp_32b[1]);
  data1 = data1 <<16;
  unsigned long data2 = (unsigned long) (SPI_temp_32b[2]);
  data2 = data2 & 0xc0;
  data2 = data2 << 8;
  data = (unsigned long) (data0 | data1 | data2);
  ecgdata = (signed long) (data);

  DataPacketHeader[0] = 0x0A;
  DataPacketHeader[1] = 0xFA;
  DataPacketHeader[2] = 0x0C;
  DataPacketHeader[3] = 0;
  DataPacketHeader[4] = 0x02;

  DataPacketHeader[5] = ecgdata;
  DataPacketHeader[6] = ecgdata>>8;
  DataPacketHeader[7] = ecgdata>>16;
  DataPacketHeader[8] = ecgdata>>24; 

  DataPacketHeader[9] =  0;
  DataPacketHeader[10] = 0;
  DataPacketHeader[11] = 0x00;
  DataPacketHeader[12] = 0x00; 

  DataPacketHeader[13] = 0;
  DataPacketHeader[14] = 0;
  DataPacketHeader[15] = 0x00;
  DataPacketHeader[16] = 0x00;

  DataPacketHeader[17] = 0x00;
  DataPacketHeader[18] = 0x0b;

  for(i=0; i<19; i++) // transmit the data
  {
    Serial.write(DataPacketHeader[i]);
  }

}

void Protocentral_MAX30003::Set_SamplingRate(uint8_t sampling_rate){

  Reg_Read(CNFG_ECG);
  Serial.println(SPI_temp_32b[1]);
  reg_write_value = SPI_temp_32b[1] & ECG_CONFIG_SRMASK;
  reg_write_value |= sampling_rate;

  Reg_Write(CNFG_ECG, sampling_rate);
}

bool Protocentral_MAX30003::Read_ID(void){

  Reg_Read(STATUS);

  if(SPI_temp_32b[1] == 0x51)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void Protocentral_MAX30003::Reg_Read(uint8_t Reg_address)
{
  uint8_t SPI_TX_Buff;
  digitalWrite(m_cs_pin, LOW);
  
  SPI_TX_Buff = (Reg_address<<1 ) | RREG;
  SPI.transfer(SPI_TX_Buff); //Send register location
   
  for ( i = 0; i < 3; i++)
  {
    SPI_temp_32b[i] = SPI.transfer(0xff);
  }
  Serial.println(SPI_temp_32b[1]);
  digitalWrite(m_cs_pin, HIGH);
}

 void Protocentral_MAX30003::Stream_ECG(void){

  while(FIFO_EMPTY){

    Reg_Read(STATUS);
    uint8_t status_bits = (SPI_temp_32b[0] >> 4) & 0xF;
    
    if ((status_bits & 0x4) == 0x4) {

      Serial.println("FIFO Reset");
      Reg_Write(FIFO_RST,0x000000);
      
    }
    else if ((status_bits & 0x8) == 0) {
        // No data present in FIFO
      Serial.println("no data in FIFO ");
    }
    else{
      FIFO_EMPTY = false;
    }
  }

  for (i=0; i <SAMPLES_PER_PACKET; i++) {

    Reg_Read(ECG_FIFO);
    send_data_serial();
  }

  FIFO_EMPTY = true;
  delayMicroseconds(1);
}
