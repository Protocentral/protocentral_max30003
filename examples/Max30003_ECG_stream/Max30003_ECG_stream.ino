//////////////////////////////////////////////////////////////////////////////////////////
//
//    Demo code for the MAX30003 breakout board
//
//    Author: Ashwin Whitchurch
//    Copyright (c) 2018 ProtoCentral
//
//    This example sends the ECG data through UART. to view the ECG, open protocentral MAX30003 GUI and select the port . 
//    Arduino connections:
//
//  |ADS1220 pin label| Pin Function         |Arduino Connection|
//  |-----------------|:--------------------:|-----------------:|
//  | MISO            | Slave Out            |  D12             |
//  | MOSI            | Slave In             |  D11             |
//  | SCLK            | Serial Clock         |  D13             |
//  | CS              | Chip Select          |  D7              |
//  | FCLK            | 32k clock            |  D6              |
//  | DVDD            | Digital VDD          |  +5V             |
//  | DGND            | Digital Gnd          |  Gnd             |
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   For information on how to use, visit https://github.com/Protocentral/protocentral_max30003
//
/////////////////////////////////////////////////////////////////////////////////////////

 #include "Protocentral_MAX30003.h"
 #define FCLK_PIN 6
 #define CS_PIN 7

Protocentral_MAX30003 max30003;
bool read_success;

void setup(){
      
  Serial.begin(115200); //Serial begin
  
  read_success = max30003.Read_ID();
  if(read_success){
    Serial.println("initialized the max30003 chip");
    max30003.begin(CS_PIN, FCLK_PIN);
    
  }else{
    Serial.println("max30003 chip not found, check the connections");
  }

  max30003.Set_SamplingRate(SR_512);
}
     
void loop(){
  
  if(read_success){

    max30003.Stream_ECG();
    
  }
 
}