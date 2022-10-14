/**
  ******************************************************************************
  * @file    game.c 
  * @author  David Webster - 100293854
  * @brief   This file contains some functions for managing the projectiles in the game. 
  ******************************************************************************
  */

#include "game.h"
#include "math_functions.h"

/**
	* @brief Move a projectile by 1 frame
*/
void move(Projectile* proj, int32_t framerate){
	proj->xpos += proj->xvel / framerate;
	proj->ypos += proj->yvel / framerate;
}

/**
	* @brief Create and populate a new projectile struct
*/
Projectile createProjectile(uint32_t xpos, uint32_t ypos, float xvel, float yvel){
	Projectile proj;
	proj.xpos = (float)xpos;
	proj.ypos = (float)ypos;
	proj.xpos_start = xpos;
	proj.ypos_start = ypos;
	proj.xvel = xvel;
	proj.yvel = yvel;
	return proj;
}

/**
	* @brief Shoot a projectile from the specified position (xpos, ypos), with the direction (aimX, aimY) with a speed of vel.  
	*aimX and aimY are from 0, not from xpos and ypos. Shooting with aimX of 10 and aimY of 0 will always shoot horizontally, for example. 
*/
Projectile shoot(int32_t aimX, int32_t aimY, int32_t xpos, int32_t ypos, int32_t vel){
	float velArr[2];
	normalizeToCircle(aimX, aimY, vel, velArr);
	return createProjectile(xpos, ypos, velArr[0], velArr[1]);;
}

