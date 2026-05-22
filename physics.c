#include "physics.h"

void Physics_applyGravity(PhysicsBody *body, float dt){
	const float gravity = -18.0f;

	if (!body || !body->active){
		return;
	}

	if (body->inv_mass <= 0.0f){
		return; //immovable object
	}

	body->vz += gravity *dt;
}


void Physics_integrate(PhysicsBody *body, float dt){
	if (!body || !body->active){
		return;
	}

	body->x += body->vx * dt;
	body->y += body->vy * dt;
	body->z += body->vz * dt;

	//body->rot_x += body->avx * dt;
	//body->rot_y += body->avy * dt;
	//body->rot_z += body->avz * dt;
}

void Physics_resolveGroundCollision(PhysicsBody *body){
	const float ground_z = 0.0f;
	const float bounce = 0.35f;
	const float friction = 0.80f;

	float bottom_z;

	if (!body || !body->active){
		return;
	}

	bottom_z = body->z - body->size_z * 0.5f;

	if (bottom_z < ground_z){
		body->z = ground_z + body->size_z *0.5f;

		if (body->vz < 0.0f){
			body->vz = -body->vz *bounce;
			body->vx *= friction;
			body->vy *= friction;
		}

		body->on_ground = true;

		if (fabsf(body->vz) < 0.25f){
			body->vz = 0.0f;
		}
		if (fabsf(body->vx) < 0.05f){
			body->vx = 0.0f;
		}
		if (fabsf(body->vy) < 0.05f){
			body->vy = 0.0f;
		}
	}
	else {
		body->on_ground = false;
	}
}


bool Physics_bodyIntersectsAABB(PhysicsBody *body,
                                float x,  float y,  float z,
                                float sx, float sy, float sz){

	if (!body || !body->active){
		return false;
	}


	float body_min_x;
	float body_min_y;
	float body_min_z;

	float body_max_x;
	float body_max_y;
	float body_max_z;

	float box_min_x;
	float box_min_y;
	float box_min_z;

	float box_max_x;
	float box_max_y;
	float box_max_z;

	body_min_x = body->x - body->size_x * 0.5f;
	body_min_y = body->y - body->size_y * 0.5f;
	body_min_z = body->z - body->size_z * 0.5f;

	body_max_x = body->x + body->size_x * 0.5f;
	body_max_y = body->y + body->size_y * 0.5f;
	body_max_z = body->z + body->size_z * 0.5f;

	box_min_x = x - sx * 0.5f;
	box_min_y = y - sy * 0.5f;
	box_min_z = z - sz * 0.5f;

	box_max_x = x + sx * 0.5f;
	box_max_y = y + sy * 0.5f;
	box_max_z = z + sz * 0.5f;

	if (body_max_x < box_min_x || body_min_x > box_max_x ||
	    body_max_y < box_min_y || body_min_y > box_max_y ||
	    body_max_z < box_min_z || body_min_z > box_max_z){

		return false;
	}
	return true;
}



void Physics_reflectVelocity(PhysicsBody *body,
                             float normal_x,
                             float normal_y,
                             float normal_z,
                             float bounce){

	float dot;

	if (!body || !body->active){
		return;
	}

	dot = body->vx * normal_x + 
	      body->vy * normal_y + 
	      body->vz * normal_z;

	if (dot >= 0.0f){
		return;
	}

	body->vx = body->vx - (1.0f + bounce) * dot * normal_x;
	body->vy = body->vy - (1.0f + bounce) * dot * normal_y;
	body->vz = body->vz - (1.0f + bounce) * dot * normal_z;
}





















