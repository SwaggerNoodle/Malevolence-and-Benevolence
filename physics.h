#ifndef MALEVOLENCE_AND_BENEVOLENCE_PHYSICS_H
#define MALEVOLENCE_AND_BENEVOLENCE_PHYSICS_H

#include "common.h"

typedef struct PhysicsBody{
	float x;
	float y;
	float z;

	float vx;
	float vy;
	float vz;

	float size_x;
	float size_y;
	float size_z;

	//float rot_x;
	//float rot_y;
	//float rot_z;

	//float avx;
	//float avy;
	//float avz;
	
	float mass;
	float inv_mass;

	bool active;
	bool on_ground;
} PhysicsBody;

void Physics_applyGravity(PhysicsBody *body, float dt);
void Physics_integrate(PhysicsBody *body, float dt);
void Physics_resolveGroundCollision(PhysicsBody *body);

bool Physics_bodyIntersectsAABB(PhysicsBody *body,
				float x,  float y,  float z,
				float sx, float sy, float sz);

#endif
