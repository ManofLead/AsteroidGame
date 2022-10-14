/**
  ******************************************************************************
  * @file    poll.c 
  * @author  David Webster - 100293854
  * @brief   This file contains functions for polling the peripherals used for inputs. 
  ******************************************************************************
  */


#include "poll.h"
#include "stm32746g_discovery_ts.h"     // Keil.STM32F746G-Discovery::Board Support:Drivers:Touch Screen
#include "Board_Touch.h"

//valid motions
//cw: 11->01, 01->00, 00->10, 10->11
//ccw: 10->00, 00->01, 01->11, 11->10
//encode prevclk, prevdt, curclk, curdt as int, then index into this table to get rotation
static int rotaryEncoderMotionTable[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

/**
	*@brief Initialize GPIO pins
	*@param sevenSegmentDisplay An array of 7 pin structs, representing the inputs of a 7-seg from a to g. 
	*@param touchSensor pointer to a struct to represent the touch sensor.
	*@param rotaryEncoder pointer to a struct represent the rotary encoder.
	*All inputs must be allocated. In my project's case, they are all static in mainloop.c. 
	*Sets 7-seg pins to push-pull output
	*Sets rotary encoder to interrupt on rising/falling
	*Sets button to input pull-up
	*Sets touch sensor to input no pull
	*Polls buttons and rotary encoder to set their initial previous states. 
*/
void initializePins(pin *sevenSegmentDisplay, buttonStruct *touchSensor, buttonStruct *button, rotaryEncoderStruct *rotaryEncoder){
	GPIO_InitTypeDef gpio;
	int i;
	
	//*In order, a b c d e f g = H6, I0, G7, B4, G6, C6, C7. 
	pin sevenSegment[] = {
	{GPIOH, GPIO_PIN_6}, 
	{GPIOI, GPIO_PIN_0}, 
	{GPIOG, GPIO_PIN_7}, 
	{GPIOB, GPIO_PIN_4}, 
	{GPIOG, GPIO_PIN_6}, 
	{GPIOC, GPIO_PIN_6}, 
	{GPIOC, GPIO_PIN_7}};
	
	//In order, clk dt
	rotaryEncoderStruct rotEncode = {0, 0, 0,
		{GPIOI, GPIO_PIN_2},
		{GPIOA, GPIO_PIN_15}};
	
	buttonStruct touchSens = {0, {GPIOA, GPIO_PIN_8}};
	buttonStruct but = {0, {GPIOI, GPIO_PIN_11}}; 
	
	//all the banks used by CN4 + CN7
	//__HAL_RCC_GPIOA_CLK_ENABLE(); //only used if keypad initialized
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	//write data to structs. This will not work if any of the structs contain a non-static pointer. 
	for(i = 0; i<7; i++){
		sevenSegmentDisplay[i] = sevenSegment[i];
	}
	*rotaryEncoder = rotEncode;
	*touchSensor = touchSens;
	*button = but;
			
	//7-segment display
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_LOW;
	
	for(i = 0; i < 7; i++){
		gpio.Pin = sevenSegmentDisplay[i].pin;
		HAL_GPIO_Init(sevenSegmentDisplay[i].bank, &gpio);
	}
	
	//rotary encoder
	gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
	gpio.Pull = GPIO_NOPULL;
	
	gpio.Pin = rotaryEncoder->clk.pin;
	HAL_GPIO_Init(rotaryEncoder->clk.bank, &gpio);
	
	gpio.Mode = GPIO_MODE_INPUT;
	
	gpio.Pin = rotaryEncoder->dt.pin;
	HAL_GPIO_Init(rotaryEncoder->dt.bank, &gpio);

	rotaryEncoder->clkPreviousState = HAL_GPIO_ReadPin(rotaryEncoder->clk.bank, rotaryEncoder->clk.pin);
	rotaryEncoder->dtPreviousState = HAL_GPIO_ReadPin(rotaryEncoder->dt.bank, rotaryEncoder->dt.pin);
	
	HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	
	//buttons
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_NOPULL;
	
	gpio.Pin = touchSensor->signal.pin;
	HAL_GPIO_Init(touchSensor->signal.bank, &gpio);
	
	gpio.Pull = GPIO_PULLUP;
	gpio.Pin = button->signal.pin;
	HAL_GPIO_Init(button->signal.bank, &gpio);
	
	touchSensor->state = HAL_GPIO_ReadPin(touchSensor->signal.bank, touchSensor->signal.pin);
	
	Touch_Initialize();
}
/**
	* @brief Reads a rotary encoder's output.
	* Reads a rotary encoder's output through a table lookup. 
	* The table is created from the following valid motions, from previous clk/dt state to current clk/dt state: 
	* cw: 11->01, 01->00, 00->10, 10->11
	* ccw: 10->00, 00->01, 01->11, 11->10
	* Encoding these as the index of an array, with cw as 1 and ccw as -1 gives the table:
	* {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}
	* This gets rid of errors caused by switch bouncing in the encoder. It may still fail to pick up a rotation if the signal is too noisy, however. 
	* It also must be polled enough to not miss any signal changes; this means it is best used with interrupts. 
*/
int32_t readEncoder(rotaryEncoderStruct* rotaryEncoder){
	int clk = HAL_GPIO_ReadPin(rotaryEncoder->clk.bank, rotaryEncoder->clk.pin);
	int dt = HAL_GPIO_ReadPin(rotaryEncoder->dt.bank, rotaryEncoder->dt.pin);
	int index = (rotaryEncoder->clkPreviousState<<3) + (rotaryEncoder->dtPreviousState<<2) + (clk<<1) + dt;
	rotaryEncoder->counter += rotaryEncoderMotionTable[index];
	rotaryEncoder->clkPreviousState = clk;
	rotaryEncoder->dtPreviousState = dt;
	return rotaryEncoderMotionTable[index];
}

/**
	* @brief Reads debounced button input.
*/
int32_t readButton(buttonStruct* button){
	int signalCurrentState = HAL_GPIO_ReadPin(button->signal.bank, button->signal.pin);
	button->changed = (button->state != signalCurrentState) ? 1 : 0;
	button->state = signalCurrentState;
	return signalCurrentState;
}

/**
	* @brief Displays number on 7-segment display. 
	* Uses lookup table for the states. 
*/void sevenSegmentDisplayNumber(int number, pin* segments){
	//state lookup table. 
	static const int states[10][7] = 
{{GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET}, //0
{GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET},//1
{GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET},//2
{GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET},//3
{GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET},//4
{GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET},//5
{GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET},//6
{GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET},//7
{GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET},//8
{GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET}};//9

	int i;
	for(i = 0; i < 7; i++){
		HAL_GPIO_WritePin(segments[i].bank, segments[i].pin, states[number][i]);
	}
}

/**
	* @brief Sets array of pins to 0. 
*/
void resetPins(int size, pin* pins){
	int i;
	for(i = 0; i < size; i++){
		HAL_GPIO_WritePin(pins[i].bank, pins[i].pin, GPIO_PIN_RESET);
	}
}
