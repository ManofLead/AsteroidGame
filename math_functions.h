/**
  ******************************************************************************
  * @file    math_functions.c 
  * @author  David Webster - 100293854
  * @brief   This file contains an assortment of miscellaneous math functions used in game.c and mainloop.c. 
  ******************************************************************************
  */


#include <stdint.h>

uint32_t fastIntSqrt(uint32_t x);
int32_t scaleTriangle(int32_t x, int32_t y, int32_t length);
void normalizeToCircle(float x, float y, float radius, float out[2]);
int isInRadius(float x0, float y0, float x1, float y1, float distance);
