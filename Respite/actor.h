#pragma once
#include <bx/math.h>
struct actor
{
	float speed;
	float heading;
	bx::Vec3 target;
	bx::Vec3 pos;
	// bx::Vec3 velocity;
};