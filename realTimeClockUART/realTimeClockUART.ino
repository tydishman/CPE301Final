#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

tmElements_t tm;

#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

void setup() {
  U0init(9600);
  //while (!Serial) ; // wait for serial
  delay(200);
  //getDate(__DATE__);
  //getTime(__TIME__);
  //RTC.write(tm);
}

void loop() {
  tmElements_t tm;
  
  if(RTC.read(tm)){
    print2digits(tm.Hour);
    putChar(':');
    print2digits(tm.Minute);
    putChar(':');
    print2digits(tm.Second);
    putChar('\n');
  }
  delay(1000);
}

void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char kbhit(){
    return *myUCSR0A & RDA;
}

unsigned char getChar(){
    return *myUDR0;
}

void putChar(unsigned char U0data){
    while((*myUCSR0A & TBE) == 0);
    *myUDR0 = U0data;
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    putChar('0');
    char numToChar = number + '0';
    putChar(numToChar);
  }
  else{
    int numTen = number / 10;
    int numOne = number % 10;
    char numToCharTen = numTen + '0';
    char numToCharOne = numOne + '0';
    putChar(numToCharTen);
    putChar(numToCharOne);
  }
}