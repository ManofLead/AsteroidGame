/**
  ******************************************************************************
  * @file    poll.c 
  * @author  David Webster - 100293854
  * @brief   This file contains functions for polling the peripherals used for inputs. 
	* Ignore the include chain errors the includes throw if you open the source code in uvision; it compiles fine. 
  ******************************************************************************
  */

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_gpio.h"

/**
*@brief GPIO pin struct
*/
typedef struct{
	GPIO_TypeDef *bank; /** GPIO bank */
	uint16_t pin; /** GPIO pin */
}pin;

/**
*@brief Rotary encoder struct
*/
typedef struct{
	int32_t counter; /** Current position of the encoder */
	int clkPreviousState; /**Previous state of clk */
	int dtPreviousState; /**Previous state of data */
	pin clk; /** clk pin */
	pin dt; /** data pin */
}rotaryEncoderStruct;

/**
*@brief Button struct
*/
typedef struct{
	int state; /** Last read state of the button */
	pin signal; /** Signal pin of the button */
	int changed; /** Flag for if the last read changed state */
}buttonStruct;


void initializePins(pin *sevenSegmentDisplay, buttonStruct *touchSensor, buttonStruct *button, rotaryEncoderStruct *rotaryEncoder);
int32_t readEncoder(rotaryEncoderStruct* rotaryEncoder);
int32_t readButton(buttonStruct* button);
void sevenSegmentDisplayNumber(int number, pin* segments);
void resetPins(int size, pin* pins);
