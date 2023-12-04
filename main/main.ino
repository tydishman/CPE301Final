
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
// #include <rtc.h>

//UART Definitions 
#define RDA 0x80
#define TBE 0x20
//UART Variables
volatile unsigned char *myUCSR0A = (unsigned char*)0x00C0;
volatile unsigned char *myUSCR0B = (unsigned char*)0x00C1;
volatile unsigned char *myUSCR0C = (unsigned char*)0x00C2;
volatile unsigned char *myUBBR0 = (unsigned char*)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char*)0x00C6;


// Analog Comparator Variables
volatile unsigned char *myACSR = (unsigned char*) 0x50; // the analog comparator status register

// External interrupt control registers
volatile unsigned char *myEICRB = (unsigned char*)0x6A;

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

State currentState = DISABLED; // global variable to indicate what state the program is currently in



void setup(){
    *myDDRA |= 0b11111111; // sets those pins as outputs
    
    *myEICRB |= 0b00001100; // rising edge on the interrupt button does interrupt

    // U0Init(9600); //initializes UART w/ 9600 baud
    Serial.begin(9600);
    
    lcd.begin(16, 2); //initializes LCD, 16 columns, 2 rows
    lcd.setCursor(0, 0);

    *myACSR |= 0b00000011;
}
void loop(){

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

   delay(2500);
   if(currentState == DISABLED){
    *myPORTA &= 0b01111111;
    currentState = IDLE;
   }
   else if(currentState == IDLE){
    *myPORTA &= 0b01111111;
    currentState = ERROR;
   }
   else if (currentState == ERROR){
    *myPORTA &= 0b01111111;
    currentState = RUNNING;
   }
   else if(currentState == RUNNING){
    *myPORTA &= 0b01111111;
    currentState = DISABLED;
   }

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

// external interrupt for a rising edge on INT5 (D3) (this will be the stop button)
ISR(INT5_vect){ 
    char statusReg = *mySREG;

    // // do interrupt stuff
    // if(currentState == DISABLED){
    //     return;
    // }
    // set state to DISABLED
    currentState = DISABLED;

    *myPORTA |= 0b10000000;


    // fan off


    // end of interrupt
    *mySREG = statusReg;
}

// // interrupt for analog comparator (AIN0 is +, AIN1 is -); when AIN0 > AIN1, ACO is set. Interrupt can be configured to trigger on output rise, fall, or TOGGLE in this case
// ISR(ANA_COMP){
//     char statusReg = *mySREG;

//     if(waterLevel )

//     *mySREG = statusReg;
// }

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