/**
  ******************************************************************************
  * @file    math_functions.c 
  * @author  David Webster - 100293854
  * @brief   This file contains an assortment of miscellaneous math functions used in game.c and mainloop.c. 
  ******************************************************************************
  */


#include <stdint.h>
#include <math.h>
#include "math_functions.h"

/**
	* @brief Approximate truncated integer square root. 
	* Runs some newton-raphson iterations. 
	* Good enough precision for when I only need an integer (such as in a circle), and much faster. 
*/
uint32_t fastIntSqrt(uint32_t x){
	uint32_t a, b, i;
	if(x < 2) {return x;} //Avoid division by 0
	a = x>>2;
	for(i = 0; i < 6; i++){
		b = x/a;
		a = (a + b)/2;
	}
	return a;
}

/**
	* @brief Normalizes input vector to a circle
	* Output is written to out[2]. 
*/
void normalizeToCircle(float x, float y, float radius, float out[2]){
	float root;
	
	if(x == 0){
		out[0] = 0; 
		out[1] = (y<0) ? -radius : radius;
		return;
	}
	if(y == 0){
		out[1] = 0;
		out[1] = (x<0) ? -radius : radius;
		return;
	}
	root = sqrt(x*x + y*y);
	out[0] = (x/root)*radius;
	out[1] = (y/root)*radius;
}

/**
	* @brief Check if two positions are within distance of each other through comparison of euclidean distance. 
*/
int isInRadius(float x0, float y0, float x1, float y1, float distance){
	//x^2 + y^2 > r^2
	float x,y;
	x = x0 - x1;
	y = y0 - y1;
	x = x*x; y = y*y;
	return ((x + y) < distance*distance);
}

