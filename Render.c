/**
  ******************************************************************************
  * @file    Render.c 
  * @author  David Webster - 100293854
  * @brief   This file contains a library for drawing vector graphics to the GLCD panel. 
	*The main things that make this differ from the GLCD_746G_Discovery.c version is the inclusion of double buffering
	*And functions for drawing anti-aliased lines or circles. 
  ******************************************************************************
  */



#define GLCD_LANDSCAPE 0

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "stm32f746xx.h"
#include "GLCD_Config.h"
#include "stm32746g_discovery_sdram.h"
#include "Render.h"
#include "Fonts.h"
#include "math_functions.h"


#ifndef SDRAM_BASE_ADDR
#define SDRAM_BASE_ADDR       0xC0000000
#endif

#define Buffer1_address SDRAM_BASE_ADDR
#define Buffer2_address SDRAM_BASE_ADDR + GLCD_SIZE_X * GLCD_SIZE_Y * 2
extern GLCD_FONT GLCD_Font_16x24;

/*---------------------------- Global variables ------------------------------*/
static uint16_t frame_buf_1[GLCD_WIDTH*GLCD_HEIGHT] __attribute__((at(Buffer1_address)));
static uint16_t frame_buf_2[GLCD_WIDTH*GLCD_HEIGHT] __attribute__((at(Buffer2_address)));
static uint16_t* frame_buf; 
static uint16_t foreground_color = GLCD_COLOR_WHITE;
static uint16_t background_color = GLCD_COLOR_BLACK;
static LTDC_HandleTypeDef LTDC_Handle;
static int32_t stride;
static GLCD_FONT *active_font = &GLCD_Font_16x24;
static enum framebuffer active = buffer1;

/**
	*@brief Initialize the SDRAM and LCD-TFT Display Controller.
	*Almost identical to the version in the GLCD API; it simply initializes both buffers as well. 
*/
void GLCD_Initialize_Doublebuffer(void){
  GPIO_InitTypeDef         GPIO_InitStructure;
  RCC_PeriphCLKInitTypeDef RCC_PeriphClkInitStructure;
  LTDC_LayerCfgTypeDef     LTDC_LayerCfg;

#if !defined(DATA_IN_ExtSDRAM)
  /* Initialize the SDRAM */
  BSP_SDRAM_Init();
#endif

	//initialise areas of SDRAM to 0
	memset((uint16_t*)Buffer1_address, 0, GLCD_SIZE_X * GLCD_SIZE_Y * 2);
	memset((uint16_t*)Buffer2_address, 0, GLCD_SIZE_X * GLCD_SIZE_Y * 2);
	
  /* Enable GPIOs clock */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();

  /* GPIOs configuration */
  /*
   +------------------+-------------------+-------------------+
   +                   LCD pins assignment                    +
   +------------------+-------------------+-------------------+
   |  LCD_R0 <-> PI15 |  LCD_G0 <-> PJ7   |  LCD_B0 <-> PE4   |
   |  LCD_R1 <-> PJ0  |  LCD_G1 <-> PJ8   |  LCD_B1 <-> PJ13  |
   |  LCD_R2 <-> PJ1  |  LCD_G2 <-> PJ9   |  LCD_B2 <-> PJ14  |
   |  LCD_R3 <-> PJ2  |  LCD_G3 <-> PJ10  |  LCD_B3 <-> PJ15  |
   |  LCD_R4 <-> PJ3  |  LCD_G4 <-> PJ11  |  LCD_B4 <-> PG12  |
   |  LCD_R5 <-> PJ4  |  LCD_G5 <-> PK0   |  LCD_B5 <-> PK4   |
   |  LCD_R6 <-> PJ5  |  LCD_G6 <-> PK1   |  LCD_B6 <-> PK5   |
   |  LCD_R7 <-> PJ6  |  LCD_G7 <-> PK2   |  LCD_B7 <-> PK6   |
   ------------------------------------------------------------
   |  LCD_HSYNC <-> PI10         |  LCD_VSYNC <-> PI9         |
   |  LCD_CLK   <-> PI14         |  LCD_DE    <-> PK7         |
   |  LCD_DISP  <-> PI12 (GPIO)  |  LCD_INT   <-> PI13        |
   ------------------------------------------------------------
   |  LCD_SCL <-> PH7 (I2C3 SCL) | LCD_SDA <-> PH8 (I2C3 SDA) |
   ------------------------------------------------------------
   |  LCD_BL_CTRL <-> PK3 (GPIO) |
   -------------------------------
  */
  GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FAST;

  GPIO_InitStructure.Alternate = GPIO_AF9_LTDC;

  /* GPIOG configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_12;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

  GPIO_InitStructure.Alternate = GPIO_AF14_LTDC;

  /* GPIOE configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* GPIOI configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);

  /* GPIOJ configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  |
                           GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_7  |
                           GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 |
                                         GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStructure);

  /* GPIOK configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  |
                           GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_7;
  HAL_GPIO_Init(GPIOK, &GPIO_InitStructure);

  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;

  /* GPIOI PI12 configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_12;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);

  /* GPIOK PK3 configuration */
  GPIO_InitStructure.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOK, &GPIO_InitStructure);

  /* LCD clock configuration 
       PLLSAI_VCO Input = HSE_VALUE / PLL_M = 1MHz
       PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192MHz
       PLLLCDCLK = PLLSAI_VCO Output / PLLSAIR = 192/5 = 38.4MHz
       LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6MHz
  */
  RCC_PeriphClkInitStructure.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  RCC_PeriphClkInitStructure.PLLSAI.PLLSAIN = 192;
  RCC_PeriphClkInitStructure.PLLSAI.PLLSAIR = 5;
  RCC_PeriphClkInitStructure.PLLSAIDivR = RCC_PLLSAIDIVR_4;
  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure); 

  // Enable the LTDC Clock
  __HAL_RCC_LTDC_CLK_ENABLE();

  // LTDC configuration
  LTDC_Handle.Instance = LTDC;

  // Configure horizontal synchronization width
  LTDC_Handle.Init.HorizontalSync = 40;
  // Configure vertical synchronization height
  LTDC_Handle.Init.VerticalSync = 9;
  // Configure accumulated horizontal back porch
  LTDC_Handle.Init.AccumulatedHBP = 53;
  // Configure accumulated vertical back porch
  LTDC_Handle.Init.AccumulatedVBP = 11;
  // Configure accumulated active width
  LTDC_Handle.Init.AccumulatedActiveW = 533;
  // Configure accumulated active height
  LTDC_Handle.Init.AccumulatedActiveH = 283;
  // Configure total width
  LTDC_Handle.Init.TotalWidth = 565;
  // Configure total height
  LTDC_Handle.Init.TotalHeigh = 285;

  // Configure R,G,B component values for LCD background color
  LTDC_Handle.Init.Backcolor.Red   = 0;
  LTDC_Handle.Init.Backcolor.Blue  = 0;
  LTDC_Handle.Init.Backcolor.Green = 0;

  // Polarity
  LTDC_Handle.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  LTDC_Handle.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  LTDC_Handle.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  LTDC_Handle.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    
  HAL_LTDC_Init(&LTDC_Handle); 

  LTDC_LayerCfg.WindowX0 = 0;
  LTDC_LayerCfg.WindowX1 = GLCD_SIZE_X - 1;
  LTDC_LayerCfg.WindowY0 = 0;
  LTDC_LayerCfg.WindowY1 = GLCD_SIZE_Y - 1;
  LTDC_LayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  LTDC_LayerCfg.Alpha  = 255;
  LTDC_LayerCfg.Alpha0 = 0;
  LTDC_LayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  LTDC_LayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  LTDC_LayerCfg.ImageWidth  = GLCD_SIZE_X;
  LTDC_LayerCfg.ImageHeight = GLCD_SIZE_Y;
  LTDC_LayerCfg.Backcolor.Red   = 0;
  LTDC_LayerCfg.Backcolor.Green = 0;
  LTDC_LayerCfg.Backcolor.Blue  = 0;
	LTDC_LayerCfg.FBStartAdress = Buffer1_address;
  HAL_LTDC_ConfigLayer(&LTDC_Handle, &LTDC_LayerCfg, 0);
	
	frame_buf = frame_buf_2;
	active = buffer1;
	
  /* Turn display and backlight on */
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3,  GPIO_PIN_SET);

	#if(GLCD_LANDSCAPE == 0)
		stride = GLCD_HEIGHT;
	#else
		stride = GLCD_WIDTH;
	#endif
}

/**
	*@brief Switchs the front and back frame buffers
	*Points the LTDC's front buffer start address to the back buffer
	*then sets the active frame buffer pointer to the new back buffer. 
	*Finally, it waits for the LCD panel's vertical synchronisation signal. 
	*This is necessary because the LCD only switches frame buffers once it's finished drawing the current frame. 
	*Otherwise, switching buffer then immediately writing to the buffer would change the front buffer. 
*/
void switchBuffer(void){
	if(active == buffer1){
		HAL_LTDC_SetAddress(&LTDC_Handle, Buffer2_address, 0);
		frame_buf = frame_buf_1;
		active = buffer2;
	}
	else{
		HAL_LTDC_SetAddress(&LTDC_Handle, Buffer1_address, 0);
		frame_buf = frame_buf_2;
		active = buffer1;
	}
	while(!(LTDC_Handle.Instance->CDSR & LTDC_CDSR_VSYNCS));
}

void setBuffer(enum framebuffer buff){
	if(buff == buffer1){
		HAL_LTDC_SetAddress(&LTDC_Handle, Buffer1_address, 0);
		frame_buf = frame_buf_2;
		active = buffer1;
	}
	else{
		HAL_LTDC_SetAddress(&LTDC_Handle, Buffer2_address, 0);
		frame_buf = frame_buf_1;
		active = buffer2;
	}
	while(!(LTDC_Handle.Instance->CDSR & LTDC_CDSR_VSYNCS));
}

void clearScreen (void) {
  uint32_t  i;
  for (i = 0; i < (GLCD_WIDTH * GLCD_HEIGHT); i++) {
    frame_buf[i] = background_color;
  }
}

void setBackgroundColor(uint16_t color){
	background_color = color;
}
void setForegroundColor(uint16_t color){
	foreground_color = color;
}


/**
	* @brief Linear interpolation of foreground_color onto specified pixel.
	* Hardcoded for GLCD_LANDSCAPE = 0. Carefully note how dot is calculated; it is not the same as GLCD_DrawPixel(). 
	* Applies linear interpolation onto the specified pixel; the background colour is that present on the canvas, the foreground colour is foreground_color. 
*/
int32_t blendPixel(uint32_t x, uint32_t y, uint8_t alpha){
	uint32_t dot = x + (stride*y);
	uint16_t bg = frame_buf[dot];
	
	//split foreground and background into rgb components
	uint16_t fg_r = foreground_color >> 11;
	uint16_t fg_g = (foreground_color >> 5) & ((1u << 6) - 1);
	uint16_t fg_b = foreground_color & ((1u << 5) - 1);
	
  uint16_t bg_r = bg >> 11;
  uint16_t bg_g = (bg >> 5) & ((1u << 6) - 1);
  uint16_t bg_b = bg & ((1u << 5) - 1);
	
  uint16_t out_r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
  uint16_t out_g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
  uint16_t out_b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;
	
	uint16_t out = ((out_r << 11) | (out_g << 5) | out_b);
	frame_buf[dot] = out;
	return 0;
}

/**
	* @brief Linear interpolation of foreground_color onto specified pixel. A faster version of blendPixel(). 
	* Hardcoded for GLCD_LANDSCAPE = 0. Carefully note how dot is calculated; it is not the same as GLCD_DrawPixel(). 
	* Avoids doing three multiplications and divisions by doing a parallel multiply, at the cost of a little bit of precision. 
	* The produced colour may be slightly inaccurate. This is imperceptible, and therefore acceptable. 
*/
int32_t blendPixelFast(uint32_t x, uint32_t y, uint8_t alpha){
	uint32_t dot = x + (stride*y);

	uint32_t bg = (uint32_t)frame_buf[dot];
	uint32_t fg = (uint32_t)foreground_color;
	uint32_t out;
	uint8_t beta;
	
	if(alpha == 255){
		frame_buf[dot] = foreground_color;
		return 0;
	}
	//convert alpha to 5-bit and add one. 
	alpha = (alpha+4) >> 3;
	//such that alpha + beta is full opacity.
	beta = 32 - alpha;
	
	//expand RGB565 to GRB655, with 5/5/6 0s padding. 
	bg = (bg | (bg << 16)) & 0x7E0F81F;
	fg = (fg | (fg << 16)) & 0x7E0F81F;
	
	//apply interpolation formula. Alpha is 0-32 instead of 0-1
	//Shift right 5 in place of division by 32. 
	//(alpha * fg) + ((1-alpha) * bg)
	out = ((alpha * fg) + (beta * bg)) >> 5;
	//mask out fractional results
	out &= 0x7E0F81F;
	//Revert to RGB565; shifting right 16 put R and B in the least
	//significant 16 bits. Then, just mask the green part into the middle. 
	frame_buf[dot] = (uint16_t)((out >> 16) | out);
	return 0;
}

/**
	* @brief Xiaolin Wu algorithm, draws an anti-aliased line from (x0,y0) to (x1,y1). 
	* Implemented using purely integer math. Uses fixed-point unsigned integers in their place. 
	* If x0, y0, x1 or y1 are off the screen, it will either segfault or write outside of the frame buffer. 
*/
void drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1){
	int32_t temp, dX, dY, xDir;
	uint32_t gradient, subPixel;
	uint8_t alpha;

	#if(GLCD_LANDSCAPE == 0)
		temp = y0; y0 = GLCD_WIDTH - x0; x0 = temp;
		temp = y1; y1 = GLCD_WIDTH - x1; x1 = temp;
	#endif
	
	//Line must be top to bottom
	if(y1 < y0){
		temp = y0; y0 = y1; y1 = temp;
		temp = x0; x0 = x1; x1 = temp;
	}
	//draw the ends of the line
	frame_buf[x0 + (y0 * stride)] = foreground_color;
	frame_buf[x1 + (y1 * GLCD_WIDTH)] = foreground_color;
	
	dX = x1 - x0;
	dY = y1 - y0;
	
	if(dX >= 0){ xDir = 1;}
	else{ xDir = -1; dX = -dX;}
	
	if(dY == 0){
		while((dX+=xDir)){
			frame_buf[x0+(y0*stride)] = foreground_color;
			x0+=xDir;
		}
		return;
	}
	if(dX == 0){
		while(dY--){
			frame_buf[x0+(y0*stride)] = foreground_color;
			y0++;
		}
		return;
	}
	
	subPixel = 0;
	
	if(dY > dX){
		//(1/gradient) * 65536
		//Reciprocal of gradient, with fixed-point at 16 bits. 
		gradient = (dX << 16) / dY;
		while(--dY){
			subPixel += gradient;
			//When subPixel overflows to a whole pixel, step the whole pixel. 
			if(subPixel >= (2<<15)){
				x0+=xDir;
				subPixel -= (2<<15);
			}
			y0++;
			
			alpha = (uint8_t)((subPixel >> 8) & 0xFF);
			
			blendPixelFast(x0, y0, alpha);
			blendPixelFast(x0-xDir, y0, alpha^255);
		}
	}
	else{
		//Same as other case, but iterate over x axis. 
		//gradient * 65536
		gradient = (dY << 16) / dX;
		while(--dX){
			subPixel += gradient;
			if(subPixel >= 65536){
				y0++;
				subPixel -= 65536;
			}
			x0 += xDir;
			alpha = (uint8_t)((subPixel >> 8) & 0xFF);
			
			blendPixel(x0,y0, alpha);
			blendPixel(x0,y0+1, alpha^0xFF);
		}
	}
	return;
}

/**
	* @brief Xiaolin Wu algorithm, draws an anti-aliased line from (x0,y0) to (x1,y1). Stretches the line to a specified width.
	* The ends of the line are flat, as strictly speaking the Xiaolin Wu algorithm is not appropriate for this. 
	* If x0, y0, x1 or y1 are off the screen, it will write outside of the frame buffer. 
	* If the thickness causes a pixel to go off the screen, it will once again write outside of the frame buffer. 
	* The thickness is all on one side of the line; which direction this is depends on the gradient. 
*/
void drawThickLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t thickness){
	int32_t temp, dX, dY, xDir;
	uint32_t gradient, subPixel;
	uint8_t alpha;
	int32_t i;


	#if(GLCD_LANDSCAPE == 0)
		temp = y0; y0 = GLCD_WIDTH-x0; x0 = GLCD_HEIGHT-temp;
		temp = y1; y1 = GLCD_WIDTH-x1; x1 = GLCD_HEIGHT-temp;
	#endif
	
	

	//Line must be top to bottom
	if(y1 < y0){
		temp = y0; y0 = y1; y1 = temp;
		temp = x0; x0 = x1; x1 = temp;
	}
	
	dX = x1 - x0;
	dY = y1 - y0;
	
	if((dX == 0) && (dY == 0)){return;}
	
	if(dX >= 0){ 
	xDir = 1;
	}
	else{ 
	xDir = -1; dX = -dX;
	}
	
	subPixel = 0;
	
	if(dY > dX){
		//(1/gradient) * 65536
		//Reciprocal of gradient, with fixed-point at 16 bits. 
		gradient = (dX << 16) / dY;
		while(--dY){
			subPixel += gradient;
			//When subPixel overflows to a whole pixel, step the whole pixel. 
			if(subPixel >= (2<<15)){
				x0+=xDir;
				subPixel -= (2<<15);
			}
			y0++;
			
			alpha = (uint8_t)((subPixel >> 8) & 0xFF);
			
			blendPixelFast(x0, y0, alpha);
			blendPixelFast(x0+(xDir*thickness), y0, alpha^255);
			i = xDir*thickness;
			while(i){
				i -= xDir;
				blendPixelFast(x0+i, y0, 0xFF);
			}
		}
	}
	else{
		//Same as other case, but iterate over x axis. 
		//gradient * 65536
		gradient = (dY << 16) / dX;
		while(--dX){
			subPixel += gradient;
			if(subPixel >= 65536){
				y0++;
				subPixel -= 65536;
			}
			x0 += xDir;
			alpha = (uint8_t)((subPixel >> 8) & 0xFF);
			
			blendPixel(x0,y0, alpha);
			blendPixel(x0,y0-thickness, alpha^0xFF);
			i = thickness;
			while(i){
				i--;
				blendPixelFast(x0, y0-i, 0xFF);
			}
		}
	}
}

/**
	* @brief Draws a filled circle
	* Safe to use at the edges of the screen
	* Aliased; the circles will have jaggies. 
	* Unlikely to work for GLCD_LANDSCAPE == 1. 
*/
void drawFilledCircle(int32_t origin_x, int32_t origin_y, int32_t radius){
	int32_t height, rad_y, dot, y;
	int32_t y_squared;	int32_t radius_squared = radius * radius;
	uint32_t buffsize = GLCD_WIDTH * GLCD_HEIGHT;

	#if(GLCD_LANDSCAPE == 0)
	origin_y = GLCD_HEIGHT-origin_y;
	origin_x = GLCD_WIDTH-origin_x;
	#endif
	
	for(y = -radius; y < radius; y++){
		rad_y = y + origin_y;
		//if x is off the right side, end the function. 
		if((rad_y >= GLCD_HEIGHT) || (rad_y <= 0)){
			continue;
		}
		y_squared = y * y;
		height = fastIntSqrt(radius_squared - y_squared);
		//Iterate in increments of GLCD_WIDTH, to avoid multiplying by it for each pixel.
		//Add rad_x to the iterator, to avoid adding it for each pixel. Vroom vroom!
		for(dot = (stride*(origin_x-height)) + rad_y; dot < (stride * (origin_x + height)) + rad_y; dot+=stride){
			//if the position's off the bottom of the screen, jump up to the bottom. 
			if(dot < 0){
				dot += -(dot/stride) * stride;
			}
			//Don't draw pixels off the top of the screen. 
			if(dot > buffsize){
				continue;
			}
			frame_buf[dot] = foreground_color;
		}
	}

	return;
}
/**
	* @brief Fills a rectangle with solid colour. 
	* An input which attempts to draw pixels off the screen will write outside the frame buffer. 
*/
void fillRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	uint32_t  i, j, temp, dot;
	#if(GLCD_LANDSCAPE == 0)
		temp = x; x = GLCD_WIDTH - y; y=temp;
		temp = width; width = height; height = temp;
	#endif
	dot = x + y*stride;
	for(i=0; i < height; i++){
		for(j = 0; j<width; j++){
			dot++;
			frame_buf[dot] = foreground_color;
		}
		dot+=stride-width;
	}
}


//--------------------------
//Ripped from GLCD_746G_Discovery.c


/**
  \fn          int32_t GLCD_DrawHLine (uint32_t x, uint32_t y, uint32_t length)
  \brief       Draw horizontal line (in active foreground color)
  \param[in]   x      Start x position in pixels (0 = left corner)
  \param[in]   y      Start y position in pixels (0 = upper corner)
  \param[in]   length Line length
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t GLCD_DrawHLine (uint32_t x, uint32_t y, uint32_t length) {
  uint32_t dot;

#if (GLCD_LANDSCAPE != 0)
  dot = (y * GLCD_WIDTH) + x;
#else
  dot = ((GLCD_WIDTH - x) * GLCD_HEIGHT) + y;
#endif

  while (length--) { 
    frame_buf[dot] = foreground_color;
#if (GLCD_LANDSCAPE != 0)
    dot += 1;
#else
    dot -= GLCD_HEIGHT;
#endif
  }

  return 0;
}


int32_t GLCD_DrawVLine (uint32_t x, uint32_t y, uint32_t length) {
  uint32_t dot;

#if (GLCD_LANDSCAPE != 0)
  dot = (y * GLCD_WIDTH) + x;
#else
  dot = ((GLCD_WIDTH - x) * GLCD_HEIGHT) + y;
#endif

  while (length--) { 
    frame_buf[dot] = foreground_color;
#if (GLCD_LANDSCAPE != 0)
    dot += GLCD_WIDTH;
#else
    dot += 1;
#endif
  }

  return 0;
}


int32_t GLCD_DrawRectangle (uint32_t x, uint32_t y, uint32_t width, uint32_t height) {

  GLCD_DrawHLine (x,         y,          width);
  GLCD_DrawHLine (x,         y + height, width);
  GLCD_DrawVLine (x,         y,          height);
  GLCD_DrawVLine (x + width, y,          height);

  return 0;
}

/**
  * @brief Draw character (in active foreground color)
	* Modified to leave background pixels as-is, rather than writing the background colour. 
*/
int32_t GLCD_DrawChar (uint32_t x, uint32_t y, int32_t ch) {
  uint32_t i, j;
  uint32_t wb, dot;
  uint8_t *ptr_ch_bmp;

  if (active_font == NULL) return -1;

  ch        -= active_font->offset;
  wb         = (active_font->width + 7)/8;
  ptr_ch_bmp = (uint8_t *)active_font->bitmap + (ch * wb * active_font->height);
#if (GLCD_LANDSCAPE != 0)
  dot        = (y * GLCD_WIDTH) + x;
#else
  dot        = ((GLCD_WIDTH - x) * GLCD_HEIGHT) + y;
#endif

  for (i = 0; i < active_font->height; i++) {
    for (j = 0; j < active_font->width; j++) {
      frame_buf[dot] = (((*ptr_ch_bmp >> (j & 7)) & 1) ? foreground_color : frame_buf[dot]);
#if (GLCD_LANDSCAPE != 0)
      dot += 1;
#else
      dot -= GLCD_HEIGHT;
#endif
      if (((j & 7) == 7) && (j != (active_font->width - 1))) ptr_ch_bmp++;
    }
#if (GLCD_LANDSCAPE != 0)
    dot +=  GLCD_WIDTH - j;
#else
    dot += (GLCD_HEIGHT * j) + 1;
#endif
    ptr_ch_bmp++;
  }

  return 0;
}


int32_t GLCD_DrawString (uint32_t x, uint32_t y, const char *str) {

  while (*str) { GLCD_DrawChar(x, y, *str++); x += active_font->width; }

  return 0;
}
