/**
  ******************************************************************************
  * @file    Render.c 
  * @author  David Webster - 100293854
  * @brief   This file contains a library for drawing vector graphics to the GLCD panel. 
	*The main things that make this differ from the GLCD_746G_Discovery.c version is the inclusion of double buffering
	*And functions for drawing anti-aliased lines or circles. 
  ******************************************************************************
  */



#include <stdint.h>


void GLCD_Initialize_Doublebuffer(void);
void drawFilledCircle(int32_t origin_x, int32_t origin_y, int32_t radius);
void drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
void drawThickLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t thickness);
void switchBuffer(void);
void clearScreen (void);
void setBackgroundColor(uint16_t color);
void setForegroundColor(uint16_t color);
uint32_t fastIntSqrt(uint32_t x);
int32_t GLCD_DrawChar (uint32_t x, uint32_t y, int32_t ch);
int32_t GLCD_DrawString (uint32_t x, uint32_t y, const char *str);
void fillRectangle(volatile uint32_t x, volatile uint32_t y, volatile uint32_t width, volatile uint32_t height);
int32_t GLCD_DrawHLine (uint32_t x, uint32_t y, uint32_t length);
int32_t GLCD_DrawRectangle (uint32_t x, uint32_t y, uint32_t width, uint32_t height);
int32_t GLCD_DrawVLine (uint32_t x, uint32_t y, uint32_t length);

/**
	*@brief frame buffer enumerator
*/
enum framebuffer{
	buffer1, buffer2
};

