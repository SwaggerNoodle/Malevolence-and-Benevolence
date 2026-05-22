#ifndef MALEVOLENCE_AND_BENEVOLENCE_GAME_H
#define MALEVOLENCE_AND_BENEVOLENCE_GAME_H

#include "common.h"
#include "rendering.h"
#include "physics.h"

typedef enum VillagerState {
        VILLAGER_IDLE = 0,
        VILLAGER_HELD,
	VILLAGER_THROWN,
        VILLAGER_WALKING_TO_TARGET,
        VILLAGER_WORKING
} VillagerState;
typedef enum VillagerJob {
        JOB_NONE = 0,
        JOB_GATHER_FOOD,
        JOB_WORSHIP
} VillagerJob;
typedef enum WorkSiteType {
        SITE_FOOD = 0,
        SITE_WORSHIP
} WorkSiteType;


typedef struct InputState {
        bool key_w;
        bool key_a;
        bool key_s;
        bool key_d;

        bool mouse_left_down;
        bool mouse_left_pressed;
        bool mouse_left_released;

        bool mouse_right_down;
        bool mouse_right_pressed;
        bool mouse_right_released;

	float mouse_wheel_y;

        int mouse_x;
        int mouse_y;
        int mouse_delta_x;
        int mouse_delta_y;
} InputState;
typedef struct Camera {
        float x;
        float y;
        float z;
        float pitch;
	float yaw;
} Camera;
typedef struct WorldObject {
        float x;
        float y;
        float z;

        float size_x;
        float size_y;
        float size_z;
} WorldObject;

typedef struct Villager {
	PhysicsBody body;

        float target_x;
        float target_y;
        float target_z;

	float velocity_x; 
	float velocity_y; 
	float velocity_z; 

        float move_speed;

        float work_timer;

        VillagerState state;
        VillagerJob job;

        int target_site_index;
        bool active;
} Villager;


typedef struct WorkSite {
        float x;
        float y;
        float z;

        float size_x;
        float size_y;
        float size_z;

        WorkSiteType type;
        bool active;
        float work_progress;
} WorkSite;

typedef struct GameState {
        bool running;
	
        SDL_Window *window;
        int window_width;
        int window_height;

	float orbit_anchor_x;
	float orbit_anchor_y;

        float delta;
        Uint64 last_ticks;

        InputState input;
        Camera camera;

        WorldObject temple;

        Villager villagers[MAX_VILLAGERS];
        int villager_count;
        int held_villager_index;

	float held_offset_x;
        float held_offset_y;

	float held_prev_x;
	float held_prev_y;
	float held_throw_vx;
	float held_throw_vy;
	float held_has_prev_position;

        WorkSite work_sites[MAX_WORK_SITES];
        int work_site_count;

        int food_amount;
        int belief_amount;

        RenderObject render_objects[MAX_RENDER_OBJECTS];
        int render_object_count;

        VulkanState vk;
} GameState;

#endif
