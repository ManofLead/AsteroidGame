/**
  ******************************************************************************
  * @file    game.c 
  * @author  David Webster - 100293854
  * @brief   This file contains some functions and a struct for managing the projectiles in the game. 
  ******************************************************************************
  */

#include <stdint.h>
#ifndef gameHeader
#define gameHeader

/**
	*@brief Projectile struct
	*xpos_start and ypos_start are used to draw the trails. 
*/
typedef struct{
	float xpos_start;/** the x position it started at*/
	float ypos_start;/** the y position it started at*/
	float xpos;/** the x position */
	float ypos;/** the y position */
	float xvel;/** the distance it moves per second along the x axis */
	float yvel;/** the distance it moves per second along the y axis */
}Projectile;

Projectile shoot(int32_t aimX, int32_t aimY, int32_t xpos, int32_t ypos, int32_t vel);
void move(Projectile* proj, int32_t framerate);
Projectile createProjectile(uint32_t xpos, uint32_t ypos, float xvel, float yvel);
#endif
