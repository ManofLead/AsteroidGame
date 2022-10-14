
/**
  ******************************************************************************
  * @file    Mainloop.c 
  * @author  David Webster - 100293854
  * @brief   This file contains the core logic of the program
  ******************************************************************************
  */

/* Set screen to landscape*/
#define GLCD_LANDSCAPE 0

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "stm32f7xx_hal.h"

#include "GLCD_Config.h"
#include "Board_Touch.h"

#include "poll.h"
#include "Render.h"
#include "game.h"
#include "math_functions.h"
#include "list.h"


/* Defines ------------------------------------------------------------------*/
#ifdef __RTX
extern uint32_t os_time;
uint32_t HAL_GetTick(void) {
	return os_time;
}
#endif 


#define COUNTERMAX 10
#define AIM_HEIGHT 160
#define BULLET_EXPLOSION_RADIUS 60
#define BULLET_RADIUS 10
#define BULLET_TRAIL_THICKNESS 3

/** Enumerator representing the different screens */
enum stateEnum{
	start, game, lose, win
};
static enum stateEnum state = start;


static pin sevenSegmentDisplay[7]; /** Array of seven segment display pins, from a to g*/
static buttonStruct touchSensor; /** Struct representing the touch sensor */
static buttonStruct button; /** Struct representing the user button */
static rotaryEncoderStruct rotaryEncoder; /** Struct representing the rotary encoder */
static TOUCH_STATE tsc_state; /** Touchscreen state struct */

static int enemiesRemaining; 
static Projectile bullet; 
static list enemyList;
static int explosionTimer;
static int enemyTimer;
static int rand;
static int wasTouched;
/**
* @}
*/

/**
* @brief System Clock Configuration, as given in the GLCD labsheet
*/
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();
	/* The voltage scaling allows optimizing the power
	consumption when the device is clocked below the
	maximum system frequency. */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/* Enable HSE Oscillator and activate PLL
	with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	/* Select PLL as system clock source and configure
	the HCLK, PCLK1 and PCLK2 clocks dividers */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | 
	RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/**
* @brief Function that handles drawing and input for the start screen
*/
void startLoop(){
	/* Read touchscreen */
	Touch_GetState(&tsc_state);
	/* Draw box and text */
	setForegroundColor(GLCD_COLOR_NAVY);
	fillRectangle(136-100, 240-120, 200, 160);
	setForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString(136-64, 240-12, "Touch to");
	GLCD_DrawString(136-32, 240+12, "Play");
	/* Wait for touch to be released */ 
	if(wasTouched && !tsc_state.pressed){
		wasTouched = 0;
	}
	if(tsc_state.pressed && !wasTouched){
		/* Initialise game variables, set state to game */
		state = game;
		rotaryEncoder.counter = 0;
		bullet = shoot(10, 10, 131, 0, 0);
		readButton(&touchSensor);
		deleteList(&enemyList);
		enemiesRemaining = 9;
		explosionTimer = 0;
		enemyTimer = 60;
	}
}

/**
* @brief Function that handles drawing and input for the win screen
*/
void winLoop(){
	/* Read touchscreen */
	Touch_GetState(&tsc_state);
	/* Draw box and text */
	setForegroundColor(GLCD_COLOR_DARK_GREEN);
	fillRectangle(136-100, 240-80, 200, 160);
	setForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString(136-64, 240-12, "You win!");
	/* Switch to start screen */
	if(tsc_state.pressed){
		state = start;
		wasTouched = 1;
	}
}

/**
* @brief Function that handles drawing and input for the lose screen.
*/
void loseLoop(){
	/* Read touchscreen */
	Touch_GetState(&tsc_state);
	/* Draw box and text */
	setForegroundColor(GLCD_COLOR_MAROON);
	fillRectangle(136-100, 240-80, 200, 160);
	setForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString(136-72, 240-12, "You lose!");
	/* Switch to start screen */
	if(tsc_state.pressed){
		state = start;
		wasTouched = 1;
	}
}

/**
* @brief Function that handles drawing, input and logic for the game.
*/
void gameLoop(){
	/* Local variables */
	int32_t aimPos, rand1;
	float gunTip[2];
	iterator enemyIter;
	Projectile *curEnemy;
	
	/* Poll inputs */
	
	/* Clamp rotary encoder counter to +-10 */
	rotaryEncoder.counter = (rotaryEncoder.counter > COUNTERMAX) ? COUNTERMAX : rotaryEncoder.counter;
	rotaryEncoder.counter = (rotaryEncoder.counter < -COUNTERMAX) ? -COUNTERMAX : rotaryEncoder.counter;
	
	/* Get aim position. Player gun will aim at a point AIM_HEIGHT pixels up and aimPos pixels left/right
	 Doing it like this avoids doing trigonometry.*/
	aimPos = 136 + (15*rotaryEncoder.counter);
	
	/** Poll touch sensor and button */
	readButton(&touchSensor);
	readButton(&button);
	
	
	
	
	/* Move and draw projectiles */
	
	/* Bounce player bullet; not fully implemented, as the trail will follow it, but better than segfaulting. */
	if(bullet.xpos<5 || bullet.xpos > 267){
		bullet.xvel = -bullet.xvel;
	}
	/* Move player bullet one frame */
	move(&bullet, 30);
	/* Draw player bullet trail and circle*/
	setForegroundColor(GLCD_COLOR_NAVY);
	drawThickLine(bullet.xpos_start, bullet.ypos_start, bullet.xpos, bullet.ypos, BULLET_TRAIL_THICKNESS);
	setForegroundColor(GLCD_COLOR_CYAN);
	drawFilledCircle(bullet.xpos, bullet.ypos, BULLET_RADIUS);
	
	/* Iterate over enemy bullets*/
	enemyIter = getIterator(&enemyList);
	while((curEnemy = getNext(&enemyIter)) != NULL){
		/* Move then draw each bullet */
		move(curEnemy, 30);
		setForegroundColor(GLCD_COLOR_PURPLE);
		drawThickLine(curEnemy->xpos_start, curEnemy->ypos_start, curEnemy->xpos, curEnemy->ypos, BULLET_TRAIL_THICKNESS);
		setForegroundColor(GLCD_COLOR_RED);
		drawFilledCircle(curEnemy->xpos, curEnemy->ypos, BULLET_RADIUS);
	}
	
	/* Meteor shooting */
	
	/* Shoot a meteor if there are enemies remaining, and either the button is pressed or the timer has elapsed*/
	if((enemiesRemaining!=0) && ((button.changed && button.state) || 
		(enemyTimer < 0))){
		/** Acquire a couple random-ish numbers*/
		rand1 = (rand * HAL_GetTick() + aimPos) % 272;
		rand = (rand1 * HAL_GetTick() + aimPos) % 260;
		/** Create the new meteor, add it to the list*/
		pushItem(&enemyList, shoot(rand-(rand1), 480, rand+6, 478, -(20 + rand1%60)));
		enemiesRemaining--; /**Decrement remaining enemies */
		enemyTimer = 300; /** Start 300-frame timer to spawn next meteor */
	}
	enemyTimer--; /**Decrement timer to spawn next meteor */
	
	/* Player gun */
	/* Draw explosion effect */
	if(explosionTimer != 0){
		/* Swap explosion colour every frame */
		if(explosionTimer%2) setForegroundColor(GLCD_COLOR_CYAN);
		else setForegroundColor(GLCD_COLOR_DARK_GREEN);
		/* Draw the circle */
		drawFilledCircle(bullet.xpos, bullet.ypos, BULLET_EXPLOSION_RADIUS);
		/* Move the player bullet under the turret when the explosion ends */
		if(!(--explosionTimer)){
			bullet.xpos = 136; bullet.ypos = 0;
		}
	}
	else if(touchSensor.changed != 0){ /* If not already exploding */
		if(touchSensor.state != 0){ /* Shoot on a rising edge */
			bullet = shoot(aimPos-136, AIM_HEIGHT, 136, 7, 150);
		}
		else{ /* Explode on a falling edge */
			/* Stop bullet's movement */
			bullet.xvel = 0; bullet.yvel = 0;
			
			/* Check for and remove destroyed meteors */
			enemyIter = getIterator(&enemyList);			
			while((curEnemy = getNext(&enemyIter)) != NULL){
				/* If a meteor is in the explosion radius, remove it */
				if(isInRadius(curEnemy->xpos, curEnemy->ypos, bullet.xpos, bullet.ypos, BULLET_EXPLOSION_RADIUS)){
					removeItem(&enemyIter, &enemyList);
				}
			}
			
			/* Set 10-frame timer of explosion effect and draw the first frame of it */
			explosionTimer = 30;
			setForegroundColor(GLCD_COLOR_CYAN);
			drawFilledCircle(bullet.xpos, bullet.ypos, BULLET_EXPLOSION_RADIUS);
		}
	}

	/* Draw player turret */
	setForegroundColor(GLCD_COLOR_BLUE);
	drawFilledCircle(136, 0, 40); /**Turret body */
	/* Point barrel at the aim point; scale it to be 100 pixels long */
	normalizeToCircle(aimPos-136, AIM_HEIGHT, 100, gunTip);
	drawThickLine(136, 7, 136 + (int)gunTip[0], (int)gunTip[1], 7);
	
	/* Draw player reticule */
	setForegroundColor(GLCD_COLOR_WHITE);
	drawFilledCircle(aimPos, AIM_HEIGHT, 10);
	
	/* Write remaining enemies to 7-segment display */
	sevenSegmentDisplayNumber(enemiesRemaining, sevenSegmentDisplay);
	
	/* Check victory and defeat conditions */
	
	enemyIter = getIterator(&enemyList);
	/* Check if enemy list is empty */
	curEnemy = getNext(&enemyIter);
	if(curEnemy == NULL){
		if(enemiesRemaining == 0){ /** If so and none are remaining, the player has won */
			state = win;
			resetPins(7, sevenSegmentDisplay);
		}
	}
	else{
		/* Otherwise, check none are less than 20 pixels off the bottom of the screen. 
		If one is, the player loses. */
		do{
			if(curEnemy->ypos <= 20){
				state = lose;
				resetPins(7, sevenSegmentDisplay);
			}
		}while((curEnemy = getNext(&enemyIter)) != NULL);
	}
	

}

/**
* @brief Main function. Holds the main superloop structure, handles frame timing and buffer switching. 
*/
int main(void){
	uint32_t frameStartTime, frameTime;
	int32_t delay;
	/* Initialization functions. */
	HAL_Init();
	SystemClock_Config();
	GLCD_Initialize_Doublebuffer();
	initializePins(sevenSegmentDisplay, &touchSensor, &button, &rotaryEncoder);

	/* Frame loop */
	while(1){ 
		/* Mark current time */
		frameStartTime = HAL_GetTick();
		/* Wipe the back buffer */
		clearScreen();
		/* Run appropriate frame function */
		switch(state){
			case start:
				startLoop();
				break;
			case game:
				gameLoop();
				break;
			case lose:
				loseLoop();
				break;
			case win:
				winLoop();
				break;
		}

		/* Switch newly drawn frame to front buffer. Synchronises to LCD's vsync. */
		switchBuffer();
		/* Get the time taken to render the last frame */
		frameTime = HAL_GetTick() - frameStartTime;
		/* Time to wait, in order to make the frame time total to 30ms. 
		 Total frame time is 33; the last 3 are left loose, and will be spent waiting for vsync. */
		delay = 30 - frameTime;
		/*Ideally, this would be PCLK1 set to count microseconds
		 However, PCLK1 is already in use. Therefore, frame time is done imprecisely with the system tick. */
		if(delay > 0) HAL_Delay(delay);
	}
}

/**
* @brief Boilerplate external interrupt request handler, for pin 2. 
*/
void EXTI2_IRQHandler(void){
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}
/**
* @brief Boilerplate external interrupt request handler, for pin 15. 
*/
void EXTI15_10_IRQHandler(void){
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

/**
* @brief External interrupt callback. 
*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	/* If pins 2 or 15 (clk or data, respectively) caused the interrupt, read the rotary encoder. */
	if((GPIO_Pin == GPIO_PIN_2) || (GPIO_Pin == GPIO_PIN_15)){
		readEncoder(&rotaryEncoder);
	}
}

