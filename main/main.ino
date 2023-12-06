
/*
* @author Tyler Dishman
* @author Matthew Gaskell
*/

// https://www.electronicshub.org/wp-content/uploads/2021/01/Arduino-Mega-Pinout.jpg
// https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf


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

float humidity, temperature;

//Arduino Libraries
#include <LiquidCrystal.h>
#include <Stepper.h>
// #include <dht.h>

//INCLUDES FOR CLOCK, NEED TO DOWNLOAD ARDUINO LIBRARIES: Time and DS1307RTC
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>


const int stepsPerRev = 2038;
//Initialize stepper, feel free to change pin #'s if differing pins used 
Stepper myStepper = Stepper(stepsPerRev, 8, 10, 9, 11);

//This here declaration shouldn't be necessary, but if clock doesn't work, uncomment this line
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
// AC notes:
/*
The internal "bandgap" reference of 1.1 volts will be used as the threshold for the water. If this turns out to not be good enough, another solution will need to be used
*/


// External interrupt control registers
volatile unsigned char *myEICRB = (unsigned char*)0x6A;
volatile unsigned char *myEIMSK = (unsigned char*)0x3D;

// LED pins
volatile unsigned char *myPORTA = (unsigned char*) 0x22;
volatile unsigned char *myDDRA = (unsigned char*) 0x21;
volatile unsigned char *myPINA = (unsigned char*) 0x20;
// PA0:3 will be used for the LEDs
// Yellow, Red, Green. Blue

//Button Pin
volatile unsigned char *myPORTE = (unsigned char*) 0x2E;
volatile unsigned char *myDDRE = (unsigned char*) 0x2D;
volatile unsigned char *myPINE = (unsigned char*) 0x2C;


volatile unsigned char *myPORTF = (unsigned char*) 0x31;
volatile unsigned char *myDDRF = (unsigned char*) 0x30;
volatile unsigned char *myPINF = (unsigned char*) 0x2F;

//ADC Registers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//LCD Pins and Arduino Pins MIGHT NEED TO CHANGE
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Status register
volatile unsigned char *mySREG = (unsigned char*)0x3f;

volatile State currentState; // global variable to indicate what state the program is currently in
volatile bool stateChange; // global variable to indicate a state change has occurred
const float TEMP_THRESH = 500.0; // the threshold for the temperature sensor, idk what to set at initially. This will be unable to change via hardware, and recompilation is required to reset this threshold


void setup(){
    *myDDRA |= 0b00001111; // sets those pins as outputs
    *myDDRF &= 0b11111110;

    *myDDRE &= 0b11000111; // PE3:5 as inputs
    
    *myEICRB |= 0b00001100; // rising edge on the interrupt button does interrupt
    *myEIMSK |= 0b00110000;
    // U0Init(9600); //initializes UART w/ 9600 baud
    Serial.begin(9600);
    
    lcd.begin(16, 2); //initializes LCD, 16 columns, 2 rows
    lcd.setCursor(0, 0);


    *myACSR |= 0b01001010; // sets bandgap reference, enables interrupts, and does comparator interrupt on falling edge

    adc_init();
    currentState = DISABLED;
}
void loop(){

    // GET TEMPERATURE HERE
    // THE FOLLOWING LINE IS TEMPORARY AND SHOULD BE REPLACED WITH A CALL TO THE TEMPERATURE SENSOR
    temperature = adc_read(0x01);

    if((currentState == IDLE) && (temperature > TEMP_THRESH)){
        currentState = RUNNING;
        stateChange = true;
    }
    else if((currentState == RUNNING) && (temperature <= TEMP_THRESH)){
        currentState = IDLE;
        stateChange = true;
    }

    if(stateChange){
        enableDisableInterrupts(currentState);
        
        stateChange = false;
    }

    // use a switch-case block to do state management
    switch (currentState)
    {
    case DISABLED:
        break;
    case IDLE:
        break;
    case ERROR:
        break;
    case RUNNING:
        break;
    
    default:
        break;
    }
    driveLED(currentState);
    // ALL STATES OTHER THAN DISABLED
    if(currentState != DISABLED){
        /*
        – Humidity and temperature should be continuously monitored and reported on the LCD screen. Updates should occur once per minute.
        – System should respond to changes in vent position control
        – Stop button should turn fan motor off (if on) and system should go to DISABLED state
        */
    }

    // ALL STATES
    /*
    * The realtime clock must be used to report (via the Serial port) the time of each state transition, and any changes to the stepper motor position for the vent.
    */

   bigStep();
   

}

// Helper functions
Color driveLED(State currState){
    switch (currState)
    {
    case DISABLED:
        // Yellow LED on
        Serial.println("DISABLED: Yellow");
        *myPORTA &= 0b11110001; // turn other colors off

        *myPORTA |= 0b00000001; // set yellow LED
        lcd.clear();
        break;
    case IDLE:
        // Green LED on
        Serial.println("IDLE: Green");
        *myPORTA &= 0b11110100;

        *myPORTA |= 0b00000100; // set green LED
        displayMonitoring(humidity, temperature);
        break;
    case ERROR:
        // Red LED on
        Serial.println("ERROR: Red");
        *myPORTA &= 0b11110010;

        *myPORTA |= 0b00000010; // set red LED
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR");
        break;
    case RUNNING:
        // Blue LED on
        Serial.println("RUNNING: Blue");
        *myPORTA &= 0b11111000;

        *myPORTA |= 0b00001000; // set blue LED
        displayMonitoring(humidity, temperature);
        break;
    }
}

void enableDisableInterrupts(State currState){
    switch (currState)
    {
    case DISABLED:
        // disable threshold interrupt (comparator interrupt)
        *myACSR &= 0b11110111;
        // START/stop button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt disable
        *myEIMSK &= 0b11101111;
        break;
    case IDLE:
        // enable threshold interrupt (comparator interrupt)
        *myACSR |= 0b00001000;
        // start/STOP button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt disable
        *myEIMSK &= 0b11101111;
        break;
    case ERROR:
        // enable threshold interrupt (comparator interrupt)
        *myACSR &= 0b11110111;
        // start/STOP button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt enable
        *myEIMSK |= 0b00010000;
        break;
    case RUNNING:
        // enable threshold interrupt (comparator interrupt)
        *myACSR |= 0b00001000;
        // start/STOP button interrupt enable
        *myEIMSK |= 0b00100000;
        // reset button interrupt disable
        *myEIMSK &= 0b11101111;
        break;
    default:
        break;
    }
}

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

        // TURN FAN OFF
    }
    *mySREG = statusReg;
}
// external interrupt for a rising edge on INT4 (D2) (this will be the RESET button)
ISR(INT4_vect){ 
    char statusReg = *mySREG;

    // if water level > baseline/bandgap voltage
    if(adc_read(0x00) > analogRead(INTERNAL1V1)){
        currentState = IDLE;
    }

    *mySREG = statusReg;
}

// // interrupt for analog comparator (AIN0 is +, AIN1 is -); when AIN0 > AIN1, ACO is set. Interrupt can be configured to trigger on output rise, fall, or TOGGLE in this case
ISR(ANALOG_COMP_vect){
    char statusReg = *mySREG;

    currentState = ERROR;
    stateChange = true;

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
    for(int i = 0; i = stringLength; i++){
        putChar(s[i]);
    }
}

void displayMonitoring(float h, float t){
    lcd.clear();
    lcd.setCursor(0,0);
    // lcd.print("Humidity: " + h);
    lcd.setCursor(1,0);
    // lcd.print("Temp: " + t);
}

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

//Clock function: prints the real time using the DS1307 RTC, in hours:minutes:seconds
//FOR THIS TO WORK INITAL CLOCK SET NEEDS TO RUN, SHOULD ONLY HAVE TO HAPPEN ONCE EVER, RUN timerInitializationCode
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

//Clock function: takes the number of hours, minutes, seconds, and separates the two numbers, converts to char, and then sents them through the UART. 
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
void bigStep(){
    myStepper.setSpeed(5);
    myStepper.step(stepsPerRev);
    delay(1000);

    myStepper.setSpeed(10);
    myStepper.step(-stepsPerRev);
    delay(1000);
}

// // for the merge later when the 1 minute timer interrupts:
// lcd.print("Humidity: " + humidity + "\n" + "Temp: " + temp);
// //or
// lcd.setCursor(0,0);
// lcd.print("Humidity: " + humidity);
// lcd.setCursor(1,0);
// lcd.print("Temp: " + temperature);

// /* How do we want to tackle state transitions?

// Interrupts?
// Like use the comparator interrupts to see if, for example, the water level is less than the threshold, and if so, during the interrupt we set the current state to ERROR
// The purpose of this would be to avoid polling? 

// */
// #include <this is here for the error squiggles>