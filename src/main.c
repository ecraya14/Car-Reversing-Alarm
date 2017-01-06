	/*  -------------------------------------------
    Project: Reversing Alarm

    
		
	The system uses code from 
	gpio.c - provides 4 functions to configure and control GPIO ports.
	TPM_PWM.c - provides 2 functions for using the PWM
	adc.c   - provides 4 functions for using the ADC
	SysTick.c  - provides 2 functions for using SysTick
	PIT.c - provides 5 functions for using PIT

    ------------------------------------------- */

#include <MKL25Z4.H>
#include <stdbool.h>
#include <stdint.h>
#include "../include/gpio.h"

#include "../include/adc_defs.h"
#include "../include/SysTick.h"
#include "../include/tpmPwm.h"
#include "../include/pit.h"

#define UP (0)
#define UPBOUNCE (1)
#define DOWN (2)
#define DOWNBOUNCE (3)
#define BOUNCE (3)  // x delay give bounce time out


#define ARMED (1);
#define DISARMED (0);

#define ON (1);
#define OFF (0);

int ALARM;
// declare volatile to ensure changes seen in debugger

volatile double measured_voltage_diff ;  // scaled value
volatile double final_measured_voltage_diff ;  // average value

volatile double distanceMeasurement = 0; 
volatile int transformNow = 0;

volatile uint16_t distance; //max difference, volume level

volatile int system; //holds current state of system
void initSystem(void) {
	system = 0;
		ALARM = OFF;
}



	int bState = UP ;
	int bCounter = 0 ;
void buttonTask (void) {


		//osDelay(10) ;
		//waitSysTickCounter(10) ;
		switch (bState) {
			case UP:
				if (isPressed()) {
					bCounter = 0 ;
					bState = DOWNBOUNCE ;
				}
				break ;
			case DOWN:
				if (!isPressed()) {
					bCounter = 0 ;
					bState = UPBOUNCE ;
				}
				break ;
			case UPBOUNCE:
				if (isPressed()) {
					bState = DOWN ;
				} else {
					if (bCounter > BOUNCE) {
						bState = UP ;
					} else {
						bCounter++ ;
					}
				}
				break ;
			case DOWNBOUNCE:
				if (isPressed()) {
					if (bCounter > BOUNCE) {
						bState = DOWN ;
						
						//switch state so that we perform 
						//detectObjectTask() meaning system is armed 
						//and give an audible confirmation
							system = ARMED;
						
							ALARM = ON; 
						//later, add functionality for OFF alarm
					} else {
						bCounter++ ;
					}
				} else {
					bState = UP ;
				}
				break ;
		}
}


/*  -----------------------------------------
     Task 2: detectObjectTask, MeasureVoltage
       res - raw result
       measured_voltage_diff - scaled
    -----------------------------------------   */
volatile int measurementNo;
void detectObjectTask(void) {
measurementNo = 0; 
distanceMeasurement = 0;
		if (system) {
			
			//number of measurements and take avg when done
			while (measurementNo < 7) {
				MeasureVoltageDiff();

				// scale to an actual voltage, assuming VREF accurate
    				measured_voltage_diff = ((double) VREF * difference) / (double) ADCRANGE ;
			
				distanceMeasurement += measured_voltage_diff;
				measurementNo++;
			}
			final_measured_voltage_diff = distanceMeasurement / (double) measurementNo;
			
			transformNow = 1;
			systemOn=0;
		}
}





/*------------------------------------------------------------
//transform measured voltage difference to object distance 

*****really unsure about the numbers I compare to, optimise these by testing

 *------------------------------------------------------------*/
void transform(void) {
	
	
		if (transformNow) {
			if(final_measured_voltage_diff < 0.00005016 ) {
				distance = 1;
			} else if (final_measured_voltage_diff < 0.00005018) {
				distance = 4;
			} else if (final_measured_voltage_diff < 0.00005018) {
				distance = 8;
			} else if (final_measured_voltage_diff < 0.00005020) {
				distance = 12;
			} else if (final_measured_voltage_diff < 0.00005021) {
				distance = 16;
			} else if (final_measured_voltage_diff < 0.000050217) {
				distance = 18;
			} else {
			//final_measured_voltage_diff > 0.00005022
				distance = 21;
			}
		}
}

/*------------------------------------------------------------
 *     Task 3: alarmTask - control tone volume - the audible tone is generated
		and becomes increasingly noticeable as the object gets closer.
 *------------------------------------------------------------*/

void alarmTask(void) {
    //int dutyParameter = 0; //volume
    
    //need to put,set timer and start timer for audio here.
   if (system) {
     stopTimer(0) ;
            
    switch(distance) {
      case 1:
				setTimer(0, 20978) ;
						PTB->PCOR = MASK(GREEN_LED_POS) ;
				startTimer(0) ;
			setPWMDuty(128) ;     // 64 is 50% volume
         	                       // Max is 128; off is 0
       break;
			case 4:
				setTimer(0, 22097) ;
				startTimer(0) ;
				setPWMDuty(115) ;
				break;
			case 8:
				setTimer(0, 24297) ;
				startTimer(0) ;
				setPWMDuty(95) ;
				break;
			case 12:
				setTimer(0, 25742) ;
				startTimer(0) ;
				setPWMDuty(70) ;
				break;
			case 16:
				setTimer(0, 27273) ;
				startTimer(0) ;
				setPWMDuty(55) ;
				break;
			case 18:
				setTimer(0, 40863) ;
				startTimer(0) ;
				setPWMDuty(30) ;
				break;
			case 21:
				setTimer(0, 45867) ;
				startTimer(0) ;
				setPWMDuty(15) ;
				break;
       		}
        
    	}
    
}



/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
volatile uint8_t calibrationFailed ; // zero expected
int main (void) {
	// Enable clock to ports B, D and E
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK ;
	initSystem();
    configureGPIOoutput(); //set audio
	init_ButtonGPIO() ; // initialise GPIO input
	Init_ADC() ; // Initialise ADC
	calibrationFailed = ADC_Cal(ADC0) ; // calibrate the ADC 
	while (calibrationFailed) ; // block progress if calibration failed
	Init_ADC() ; // Reinitialise ADC
    configurePIT(0) ;
    setTimer(0, 45867) ; // Frequency for MIDI 60 - middle C
                        // One octave up is 22934
    configureTPM0forPWM() ;
	Init_SysTick(1000) ; // initialse SysTick every 1ms
	waitSysTickCounter(10) ;

	while (1) {	
		buttonTask();	
		detectObjectTask();
		transform();
		alarmTask();
		// delay
  	waitSysTickCounter(50) ;  // cycle every 50 ms
		//PTB->PTOR = MASK(GREEN_LED_POS) ;
	}
}


