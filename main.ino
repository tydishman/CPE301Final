/*!

*/

/*
* Pin setups:
*  @author Tyler Dishman
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

// LED pins
byte PORTA = 0x02;
byte DDRA = 0x01;
byte PINA = 0x00;
// PA0:3 will be used for the LEDs


State currentState = DISABLED;


void setup(){

}
void loop(){

    // use a switch-case block to do state management
    switch (currentState)
    {
    case DISABLED:
        // Yellow LED on
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
}

// Helper functions
Color driveLED(State currState){
    switch (currentState)
    {
    case DISABLED:
        // Yellow LED on
        break;
    case IDLE:
        break;
    case ERROR:
        break;
    case RUNNING:
        break;
    }
}