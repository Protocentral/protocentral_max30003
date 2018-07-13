#ifndef Protocentral_MAX30003_H
#define Protocentral_MAX30003_H
 
#include <Arduino.h>
 
#define WREG 0x00
#define RREG 0x01

#define   NO_OP           0x00
#define   STATUS          0x01
#define   EN_INT          0x02
#define   EN_INT2         0x03
#define   MNGR_INT        0x04
#define   MNGR_DYN        0x05
#define   SW_RST          0x08
#define   SYNCH           0x09
#define   FIFO_RST        0x0A
#define   INFO            0x0F
#define   CNFG_GEN        0x10
#define   CNFG_CAL        0x12
#define   CNFG_EMUX       0x14
#define   CNFG_ECG        0x15
#define   CNFG_RTOR1      0x1D
#define   CNFG_RTOR2      0x1E
#define   ECG_FIFO_BURST  0x20
#define   ECG_FIFO        0x21
#define   RTOR            0x25
#define   NO_OP           0x7F
#define SAMPLES_PER_PACKET 8
#define ECG_CONFIG_SRMASK 0x3F
#define SR_512 0x00
#define SR_256 0x40
#define SR_128 0x80

class Protocentral_MAX30003 {

private:
    volatile char SPI_RX_Buff[5] ;
    volatile char *SPI_RX_Buff_Ptr;
    bool FIFO_EMPTY = true;
    
    int i=0;
    unsigned long data, uintECGraw = 0, intECGraw=0;
    signed long ecgdata;
    uint8_t max30003_buff[8];
    
    uint8_t m_cs_pin=7;
    uint8_t DataPacketHeader[20];

public:
    Protocentral_MAX30003();
    ~Protocentral_MAX30003();
    char SPI_temp_32b[4], SPI_temp_Burst[100];
    uint8_t m_fclk_pin=6;
    void Reg_Write (uint8_t WRITE_ADDRESS, unsigned long data);
    void begin(uint8_t cs_pin, uint8_t fclk_pin);
    void Synch(void);
    void SW_Reset(void);
    void Read_Data(int num_samples);
    void Reg_Read(uint8_t Reg_address);
    void Stream_ECG(void);
    void send_data_serial(void);
    bool Read_ID(void);
    void Set_SamplingRate(uint8_t sampling_rate);
};
 
#endif
