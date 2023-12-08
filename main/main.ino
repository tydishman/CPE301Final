
/*
* @author Tyler Dishman
* @author Matthew Gaskell
*/

// https://www.electronicshub.org/wp-content/uploads/2021/01/Arduino-Mega-Pinout.jpg
// https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf


// PROJECT DELIVERABLE LINK //
// https://docs.google.com/document/d/1Zz8_r925z1ssLB7JXb_pX80VczjSrMZtFJcOyECdHFM/edit?usp=sharing

enum State {
    DISABLED,
    IDLE,
    ERROR,
    RUNNING
};
enum Color {
    Red,
    Green,
    Blue,
    Yellow
};

float humidity, temperature, water_level;

//Arduino Libraries
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <dht.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

//Temp/Humidity Sensor
dht DHT;
#define DHT11_PIN 7

//Stepper Initialization
const int stepsPerRev = 2038;
Stepper myStepper = Stepper(stepsPerRev, 8, 10, 9, 11);

tmElements_t tm;

//UART Definitions 
#define RDA 0x80
#define TBE 0x20
//UART Variables
volatile unsigned char *myUCSR0A = (unsigned char*)0x00C0;
volatile unsigned char *myUSCR0B = (unsigned char*)0x00C1;
volatile unsigned char *myUSCR0C = (unsigned char*)0x00C2;
volatile unsigned char *myUBBR0 = (unsigned char*)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char*)0x00C6;


// Analog Comparator registers
volatile unsigned char *myACSR = (unsigned char*) 0x50; // the analog comparator status register
volatile unsigned char *myDIDR1 = (unsigned char*) 0x7F;

// External interrupt control registers
volatile unsigned char *myEICRB = (unsigned char*)0x6A;
volatile unsigned char *myEIMSK = (unsigned char*)0x3D;

// LED pins: PA0:3 will be used for the LEDs
volatile unsigned char *myPORTA = (unsigned char*) 0x22;
volatile unsigned char *myDDRA = (unsigned char*) 0x21;
volatile unsigned char *myPINA = (unsigned char*) 0x20;

//Start/Stop and Reset Button Pins
volatile unsigned char *myPORTE = (unsigned char*) 0x2E;
volatile unsigned char *myDDRE = (unsigned char*) 0x2D;
volatile unsigned char *myPINE = (unsigned char*) 0x2C;

//Fan Controller Buttons: PB0-1
volatile unsigned char *myPORTB = (unsigned char*) 0x25;
volatile unsigned char *myDDRB = (unsigned char*) 0x24;
volatile unsigned char *myPINB = (unsigned char*) 0x23;

//Water Sensor Pins
volatile unsigned char *myPORTF = (unsigned char*) 0x31;
volatile unsigned char *myDDRF = (unsigned char*) 0x30;
volatile unsigned char *myPINF = (unsigned char*) 0x2F;

//ADC Registers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//LCD Pins
const int RS = 30, EN = 31, D4 = 32, D5 = 33, D6 = 34, D7 = 35;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Status register
volatile unsigned char *mySREG = (unsigned char*)0x3f;

volatile State currentState; // global variable to indicate what state the program is currently in
volatile bool stateChange = true; // global variable to indicate a state change has occurred
const float TEMP_THRESH = 19.0; // the threshold for the temperature sensor
const int WATER_THRESH = 100; // the threshold for the water sensor
unsigned long lastMillis = 0; // global variable for time measurement for the minute delay


void setup(){
    *mySREG |= 0b10000000; // enables global interrupts
    *myDDRA |= 0b00001111; // sets those pins as outputs
    *myDDRF &= 0b11111110; // pf0 as input 

    *myDDRE &= 0b11000111; // PE3:5 as inputs

    *myDDRB &= 0b00111111; //PB6:7 as inputs , PB0:1 are outputs (fan controller)
    
    *myEICRB |= 0b00001111; // rising edge on the interrupt button does interrupt
    *myEIMSK |= 0b00110000;
    U0Init(9600); //initializes UART w/ 9600 baud
    
    lcd.begin(16, 2); //initializes LCD, 16 columns, 2 rows
    lcd.setCursor(0, 0);

    *myACSR |= 0b10000000; // disable analog comparator

    adc_init();
    currentState = DISABLED;
    customPrintFunc("UART initialized.\n",19);
}
void loop(){    
    waterLevelCheck();
    temperatureCheck();
    ventCheck();

    if(stateChange){
        enableDisableInterrupts(currentState);
        stateChange = false;
    }

    // use a switch-case block to do state management
    switch (currentState)
    {
    case DISABLED:
        fanControl(false);
        lcd.clear();
        break;
    case IDLE:
        fanControl(false);
        break;
    case ERROR:
        fanControl(false);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR");
        break;
    case RUNNING:
        fanControl(true);
        break;
    
    default:
        break;
    }
    driveLED(currentState);
    // ALL STATES OTHER THAN DISABLED
    if(currentState != DISABLED){
        if(millis() - lastMillis >= 60000){
            lastMillis = millis();
            // Update LCD screen
            displayMonitoring(humidity, temperature);
        }
    }
}

// Helper functions
Color driveLED(State currState){
    switch (currState)
    {
    case DISABLED:
        // Yellow LED on
        *myPORTA &= 0b11110001; // turn other colors off

        *myPORTA |= 0b00000001; // set yellow LED
        break;
    case IDLE:
        // Green LED on
        *myPORTA &= 0b11110100;

        *myPORTA |= 0b00000100; // set green LED
        break;
    case ERROR:
        // Red LED on
        *myPORTA &= 0b11110010;

        *myPORTA |= 0b00000010; // set red LED
        break;
    case RUNNING:
        // Blue LED on
        *myPORTA &= 0b11111000;

        *myPORTA |= 0b00001000; // set blue LED
        break;
    }
}

void enableDisableInterrupts(State currState){
    customPrintFunc("STATE CHANGE: ", 15);
    switch (currState)
    {
    case DISABLED:
        // START/stop button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt disable
        *myEIMSK &= 0b11101111;

        customPrintFunc("DISABLED", 9);

        break;
    case IDLE:
        // start/STOP button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt disable
        *myEIMSK &= 0b11101111;

        customPrintFunc("IDLE", 4);
        displayMonitoring(humidity, temperature);

        break;
    case ERROR:
        // start/STOP button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt enable
        *myEIMSK |= 0b00010000;

        customPrintFunc("ERROR", 5);

        break;
    case RUNNING:
        // start/STOP button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt disable
        *myEIMSK &= 0b11101111;

        customPrintFunc("RUNNING", 8);
        displayMonitoring(humidity, temperature);

        break;
    default:
        break;
    }
    customPrintFunc(": ", 2);
    reportTime();
    // report time automatically writes a newline after
}

//Checking Functions 
void waterLevelCheck(){
    int chk = DHT.read11(DHT11_PIN);
    temperature = DHT.temperature;
    humidity = DHT.humidity;
    
    water_level = adc_read(0x00);
    if(currentState == IDLE || currentState == RUNNING){
        if(water_level < WATER_THRESH){
            currentState = ERROR;
            stateChange = true;
        }
    }
}
void temperatureCheck(){
    if((currentState == IDLE) && (temperature > TEMP_THRESH)){
        currentState = RUNNING;
        stateChange = true;
    }
    else if((currentState == RUNNING) && (temperature <= TEMP_THRESH)){
        currentState = IDLE;
        stateChange = true;
    }
}
void ventCheck(){
    static bool alreadyPrinted = false;
    if(currentState == ERROR){
        return;
    }

    if(*myPINB & 0b01000000){
        bigStep(true);

        if(!alreadyPrinted){
            customPrintFunc("Vent opening\n", 14);
            alreadyPrinted = true;
        }
    }
    else if(*myPINB & 0b10000000){
        bigStep(false);

        if(!alreadyPrinted){
            customPrintFunc("Vent closing\n", 14);
            alreadyPrinted = true;
        }
    }
    else{
        alreadyPrinted = false;
    }
}

//Button Interrupts
// external interrupt for a rising edge on INT5 (D3) (this will be the START/STOP button)
ISR(INT5_vect){ 
    char statusReg = *mySREG;

    if(currentState == DISABLED){
        currentState = IDLE;
        stateChange = true;
    }
    else{
        currentState = DISABLED;
        stateChange = true;
    }
    
    *mySREG = statusReg;
}
// external interrupt for a rising edge on INT4 (D2) (this will be the RESET button)
ISR(INT4_vect){ 
    char statusReg = *mySREG;

    // if water level >= threshold
    if((currentState == ERROR) && (adc_read(0x00) >= WATER_THRESH)){ //  && (adc_read(0x00) >= WATER_THRESH)
        currentState = IDLE;
        stateChange = true;
    }

    *mySREG = statusReg;
}

//Functions for the UART
void U0Init(int U0baud){
    unsigned long FCPU = 16000000;
    unsigned long tbaud;
    tbaud = (FCPU / 16 / U0baud - 1);
    *myUCSR0A = 0x20;
    *myUSCR0B = 0x18;
    *myUSCR0C = 0x06;
    *myUBBR0 = tbaud;
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

// prints a string over UART to the host pc
void customPrintFunc(String s, int stringLength){
    for(int i = 0; i < stringLength; i++){
        putChar(s[i]);
    }
}

//Function to display the humidity and temperature on the LCD
void displayMonitoring(float h, float t){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Humidity: ");
    lcd.setCursor(10, 0);
    lcd.print(h);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.setCursor(6, 1);
    lcd.print(t);
}

//ADC Functions
//Function to initialize the ADC
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
//Function to read from the ADC
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

//Real-Time Clock Functions
//Prints the real time using the DS1307 RTC, in hours:minutes:seconds
void reportTime(){
    tmElements_t tm;
    
    if(RTC.read(tm)){
        print2digits(tm.Hour);
        putChar(':');
        print2digits(tm.Minute);
        putChar(':');
        print2digits(tm.Second);
        putChar('\n');
    }
}
//Takes the number of hours, minutes, seconds, and separates the two numbers, converts to char, and then sents them through the UART. 
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

//Stepper function
void bigStep(bool open){
    myStepper.setSpeed(10);
    if(open){
        myStepper.step(-stepsPerRev/36);
    }
    else{
        myStepper.step(stepsPerRev/36);
    }
}

// Fan function
void fanControl(bool on){
    if(on){
        *myPORTB |= 0b00000001;
        *myPORTB &= 0b11111101;
    }
    else{
        *myPORTB &= 0b11111100;
    }
    // PB1 always kept low; no need for bi-directional fan
}