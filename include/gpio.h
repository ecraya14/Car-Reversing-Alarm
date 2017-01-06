#include <stdbool.h>

// Definitions for GPIO
#ifndef GPIO_DEFS_H
#define GPIO_DEFS_H


#define MASK(x) (1UL << (x))

#define GREEN_LED_POS (19)	// on port B

// Button is on port D, pin 6
#define BUTTON_POS (6)

// GPIO output used for the frequency, port A pin 2
#define AUDIO_POS (2)
//IR emitting diode, port B pin 8
#define IR_POS (8)

extern int systemOn;


// Function prototypes
void init_ButtonGPIO(void) ;       // Initialise button
void configureGPIOoutput(void) ;      // Initialise output audio	
void audioToggle(void) ;          // Toggle the output GPIO
bool isPressed(void) ;

#endif
