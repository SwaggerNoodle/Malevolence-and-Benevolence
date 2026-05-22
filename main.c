#include <SDL3/SDL_main.h>

#include "common.h"
#include "game.h"
#include "rendering.h"
#include "helpers.h"
#include "physics.h"


static void initVillager(Villager *villager, float x, float y){
	villager->body.x = x;
	villager->body.y = y;
	villager->body.z = 0.5f;

	villager->body.size_x = 0.8f;
	villager->body.size_y = 0.8f;
	villager->body.size_z = 1.0f;

	villager->body.vx = 0.0f;
	villager->body.vy = 0.0f;
	villager->body.vz = 0.0f;

	villager->body.mass = 1.0f;
	villager->body.inv_mass = 1.0f;
	villager->body.active = true;
	villager->body.on_ground = true;

	villager->target_x = x;
	villager->target_y = y;
	villager->target_z = 0.5f;

	villager->move_speed = 4.0f;

	villager->work_timer = 0.0f;

	villager->state = VILLAGER_IDLE;
	villager->job = JOB_NONE;
	villager->target_site_index = -1;
	villager->active = true;
}
static void initWorkSite(WorkSite *site, float x, float y, WorkSiteType type){
	site->x = x;
	site->y = y;
	site->z = 0.5f;

	site->size_x = 2.0f;
	site->size_y = 2.0f;
	site->size_z = 1.0f;

	site->type = type;
	site->active = true;
	site->work_progress = 0.0f;
}




static void processEvents(GameState *game){
	SDL_Event event;

	game->input.mouse_delta_x = 0;
	game->input.mouse_delta_y = 0;
	game->input.mouse_wheel_y = 0.0f;
	game->input.mouse_left_pressed = false;
	game->input.mouse_left_released = false;
	game->input.mouse_right_pressed = false;
	game->input.mouse_right_released = false;

	while (SDL_PollEvent(&event)){
		switch (event.type){
			case SDL_EVENT_QUIT:
				game->running = false;
				break;
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			case SDL_EVENT_WINDOW_RESIZED:
				game->vk.framebuffer_resized = true;
				break;
			case SDL_EVENT_KEY_DOWN:
				switch (event.key.key){
					case SDLK_W: game->input.key_w = true; break;
					case SDLK_A: game->input.key_a = true; break;
					case SDLK_S: game->input.key_s = true; break;
					case SDLK_D: game->input.key_d = true; break;
				}
				break;
			case SDL_EVENT_KEY_UP:
				switch (event.key.key){
					case SDLK_W: game->input.key_w = false; break;
					case SDLK_A: game->input.key_a = false; break;
					case SDLK_S: game->input.key_s = false; break;
					case SDLK_D: game->input.key_d = false; break;
				}
				break;
			case SDL_EVENT_MOUSE_MOTION:
				game->input.mouse_x = event.motion.x;
				game->input.mouse_y = event.motion.y;
				game->input.mouse_delta_x = event.motion.xrel;
				game->input.mouse_delta_y = event.motion.yrel;
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				game->input.mouse_x = (int)event.button.x;
				game->input.mouse_y = (int)event.button.y;
				switch (event.button.button){
					case SDL_BUTTON_LEFT:
						game->input.mouse_left_down = true;
						game->input.mouse_left_pressed = true;
						break;
					case SDL_BUTTON_RIGHT:
						game->input.mouse_right_down = true;
						game->input.mouse_right_pressed = true;
						break;
				}				
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				game->input.mouse_x = (int)event.button.x;
				game->input.mouse_y = (int)event.button.y;
				switch (event.button.button){
					case SDL_BUTTON_LEFT:
						game->input.mouse_left_down = false;
						game->input.mouse_left_released = true;
						break;
					case SDL_BUTTON_RIGHT:
						game->input.mouse_right_down = false;
						game->input.mouse_right_released = true;
						break;
				}
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				game->input.mouse_wheel_y += event.wheel.y;
				break;

		}
	}
}




static int findClickedVillager(GameState *game, int mouse_x, int mouse_y){
	int i;
	int best_index = -1;
	int best_dist2 = 0x7fffffff;

	for (i = 0; i < game->villager_count; i++){
		int sx;
		int sy;
		int dx;
		int dy;
		int dist2;
		if (!game->villagers[i].active){
			continue;
		}
		if (!projectWorldToScreen(game,
					  game->villagers[i].body.x,
					  game->villagers[i].body.y,
					  game->villagers[i].body.z,
					  &sx, &sy)){
			continue;
		}
		dx = mouse_x - sx;
		dy = mouse_y - sy;
		if (dx > -40 && dx < 40 && dy > -40 && dy < 40){
			dist2 = dx * dx + dy *dy;
			if (dist2 < best_dist2){
				best_dist2 = dist2;
				best_index = i;
			}
		}
	}
	return best_index;
}
static int findWorkSiteAtScreenPos(GameState *game, int mouse_x, int mouse_y){
        int i;
        int best_index = -1;
        int best_dist2 = 0x7fffffff;

        for (i = 0; i < game->work_site_count; i++){
                int sx;
                int sy;
                int dx;
                int dy;
                int dist2;
                if (!game->work_sites[i].active){
                        continue;
                }
                if (!projectWorldToScreen(game,
                                          game->work_sites[i].x,
                                          game->work_sites[i].y,
                                          game->work_sites[i].z,
                                          &sx, &sy)){
                        continue;
                }
                dx = mouse_x - sx;
                dy = mouse_y - sy;
                if (dx > -45 && dx < 45 && dy > -45 && dy < 45){
                        dist2 = dx * dx + dy *dy;
                        if (dist2 < best_dist2){
                                best_dist2 = dist2;
                                best_index = i;
                        }
                }
	}
	return best_index;
}




static void assignVillagerToSite(GameState *game, int villager_index, int site_index){
	
	Villager *villager;
	WorkSite *site;

	if (villager_index < 0 || villager_index >= game->villager_count){
		return;
	}
	if (site_index < 0 || site_index >= game->work_site_count){
		return;
	}

	villager = &game->villagers[villager_index];
	site = &game->work_sites[site_index];

	if (!villager->active || !site->active){
		return;
	}

	villager->target_x = site->x;
	villager->target_y = site->y;
	villager->target_z = 0.5f;

	villager->target_site_index = site_index;
	villager->work_timer = 0.0f;
	villager->body.z = 0.5f;
	villager->state = VILLAGER_WALKING_TO_TARGET;

	switch (site->type){
		case SITE_FOOD:
			villager->job = JOB_GATHER_FOOD;
			break;
		case SITE_WORSHIP:
			villager->job = JOB_WORSHIP;
			break;
	}
}




static void updateWalkingVillager(GameState *game, Villager *villager){
	
	float dx = villager->target_x - villager->body.x;
	float dy = villager->target_y - villager->body.y;
	float dist = sqrtf(dx * dx + dy * dy);

	villager->body.z = 0.5f;

	if (dist > 0.05f){
		float nx = dx / dist;
		float ny = dy / dist;

		villager->body.x += nx * villager->move_speed * game->delta;
		villager->body.y += ny * villager->move_speed * game->delta;
	}
	else {
		villager->body.x = villager->target_x;
		villager->body.y = villager->target_y;
		villager->body.z = 0.5f;
		villager->state = VILLAGER_WORKING;
		villager->work_timer = 0.0f;
	}
}
static void updateWorkingVillager(GameState *game, Villager *villager){
	WorkSite *site;
	float work_cycle = 2.0f;

	if (villager->target_site_index < 0 || villager->target_site_index >= game->work_site_count){
		villager->state = VILLAGER_IDLE;
		villager->job = JOB_NONE;
		villager->target_site_index = -1;
		villager->body.z = 0.5f;
		return;
	}
	site = &game->work_sites[villager->target_site_index];
	if (!site->active){
		villager->state = VILLAGER_IDLE;
		villager->job = JOB_NONE;
		villager->target_site_index = -1;
		villager->body.z = 0.5f;
		return;
	}
	switch (villager->job){
		case JOB_GATHER_FOOD: work_cycle = 2.0f; break;
		case JOB_WORSHIP: work_cycle = 3.0f; break;
		default: work_cycle = 2.0f; break;
	}
	
	villager->body.x = villager->target_x;
	villager->body.y = villager->target_y;
	villager->body.z = 0.5f + 0.05f * sinf(villager->work_timer * 8.0f);
	
	villager->work_timer += game->delta;
	site->work_progress += game->delta;

	if (villager->work_timer >= work_cycle){
		switch (villager->job){
			case JOB_GATHER_FOOD:
				game->food_amount += 1;
				printf("Food increased to %d\n", game->food_amount);
				break;
			case JOB_WORSHIP:
				game->belief_amount += 1;
				printf("Belief increased to %d\n", game->belief_amount);
				break;
			default:
				break;
		}
		villager->work_timer = 0.0f;
		site->work_progress = 0.0f;
	}
}
static void updateHeldVillager(GameState *game){
	Villager *villager;
	int site_index;
	
	float mouse_world_x;
	float mouse_world_y;
	bool got_mouse_world = false;

	float instant_vx;
	float instant_vy;
	float smoothing;

	float throw_vx;
	float throw_vy;
	float throw_speed;
	float throw_scale;

	const float villager_held_height = 1.5f;
	const float throw_strength = 1.0f;
	const float max_throw_speed = 18.0f;
	

	if (game->held_villager_index < 0 || game->held_villager_index >= game->villager_count){
		game->held_villager_index = -1;
		game->held_has_prev_position = false;
		return;
	}

	villager = &game->villagers[game->held_villager_index];

	villager->state = VILLAGER_HELD;
	villager->job = JOB_NONE;
	villager->target_site_index = -1;
	villager->body.z = villager_held_height;

	if (Helper_mouseToWorldOnZPlane(game,
					game->input.mouse_x,
					game->input.mouse_y,
					villager_held_height,
					&mouse_world_x,
					&mouse_world_y)){
		got_mouse_world = true;
		
		villager->body.x = mouse_world_x;
		villager->body.y = mouse_world_y;
	}

	if (got_mouse_world){
		if (game->held_has_prev_position && game->delta > 0.0001f){
			instant_vx = (mouse_world_x - game->held_prev_x) / game->delta;
			instant_vy = (mouse_world_y - game->held_prev_y) / game->delta;

			smoothing = 0.35f;

			game->held_throw_vx = game->held_throw_vx * (1.0f - smoothing) + instant_vx * smoothing;
			game->held_throw_vy = game->held_throw_vy * (1.0f - smoothing) + instant_vy * smoothing;
		}
		else {
			game->held_throw_vx = 0.0f;
			game->held_throw_vy = 0.0f;
			game->held_has_prev_position = true;
		}
		game->held_prev_x = mouse_world_x;
		game->held_prev_y = mouse_world_y;
	}
	if (game->input.mouse_left_released){
		site_index = findWorkSiteAtScreenPos(game, game->input.mouse_x, game->input.mouse_y);

		if (site_index >= 0){
			villager->body.vx = 0.0f;
			villager->body.vy = 0.0f;
			villager->body.vz = 0.0f;
			villager->body.on_ground = true;

			assignVillagerToSite(game, game->held_villager_index, site_index);
		}
		else {
			villager->state = VILLAGER_THROWN;
			villager->job = JOB_NONE;
			villager->target_site_index = -1;

			throw_vx = game->held_throw_vx * throw_strength;
			throw_vy = game->held_throw_vy * throw_strength;

			throw_speed = sqrtf(throw_vx * throw_vx + throw_vy * throw_vy);

			if (throw_speed > max_throw_speed){
				throw_scale = max_throw_speed / throw_speed;
				throw_vx *= throw_scale;
				throw_vy *= throw_scale;
			}

			villager->body.vx = throw_vx;
			villager->body.vy = throw_vy;
			villager->body.vz = 8.0f;
			villager->body.on_ground = false;
		}

		game->held_villager_index = -1;
		game->held_has_prev_position = false;
		game->held_throw_vx = 0.0f;
		game->held_throw_vy = 0.0f;
	}
}
static void updateThrownVillager(GameState *game, Villager *villager){
	PhysicsBody *body = &villager->body;

	float old_x = body->x;
	float old_y = body->y;
	float old_z = body->z;

	Physics_applyGravity(&villager->body, game->delta);
	Physics_integrate(&villager->body, game->delta);

	if (Physics_bodyIntersectsAABB(body,
				       game->temple.x,
				       game->temple.y,
				       game->temple.z,
				       game->temple.size_x,
				       game->temple.size_y,
				       game->temple.size_z)){

		float normal_x = 0.0f;
		float normal_y = 0.0f;
		float normal_z = 0.0f;

		float dx = old_x - game->temple.x;
		float dy = old_y - game->temple.y;

		if (fabsf(dx) > fabsf(dy)){
			if (dx > 0.0f){
				normal_x = 1.0f;
			}
			else {
				normal_x = -1.0f;
			}
		}
		else {
			if (dy > 0.0f){
				normal_y = 1.0f;
			}
			else {
				normal_y = -1.0f;
			}
		}

		body->x = old_x;
		body->y = old_y;
		body->z = old_z;

		Physics_reflectVelocity(body, normal_x, normal_y, normal_z, 0.25f);
	}

	Physics_resolveGroundCollision(&villager->body);

	if (body->on_ground &&
	    fabsf(body->vx) < 0.05f &&
	    fabsf(body->vy) < 0.05f &&
	    fabsf(body->vz) < 0.05f){

	    villager->state = VILLAGER_IDLE;
	}
}
static void updateVillagers(GameState *game){
	int i;
	
	if (game->held_villager_index >= 0){
		updateHeldVillager(game);
	}

	for (i = 0; i < game->villager_count; i++){
		Villager *villager = &game->villagers[i];

		if (!villager->active){
			continue;
		}
		if (i == game->held_villager_index){
			continue;
		}
		
		switch (villager->state){
			case VILLAGER_IDLE:
				villager->body.z = 0.5f;
				break;
			case VILLAGER_HELD:
				break;
			case VILLAGER_WALKING_TO_TARGET:
				updateWalkingVillager(game, villager);
				break;
			case VILLAGER_WORKING:
				updateWorkingVillager(game, villager);
				break;
			case VILLAGER_THROWN:
				updateThrownVillager(game, villager);
				break;
		}
	}
}
static void updateWindowTitle(GameState *game){
	char title[256];
	snprintf(title, sizeof(title),
			"B&M Vulkan Prototype | Food: %d | Belief: %d | Render Objects:%d",
			game->food_amount,
			game->belief_amount,
			game->render_object_count);
	SDL_SetWindowTitle(game->window, title);
}
static void update(GameState *game){
	const float camera_speed = 10.0f;

	float yaw_rad = game->camera.yaw * (3.14159265 / 180.0f);

	float right_x = cosf(yaw_rad);
	float right_y = sinf(yaw_rad);

	float forward_x = -sinf(yaw_rad);
	float forward_y = cosf(yaw_rad);

	if (game->input.key_w){ 
		game->camera.x -= forward_x * camera_speed * game->delta;
		game->camera.y -= forward_y * camera_speed * game->delta;
	}
	if (game->input.key_s){ 
		game->camera.x += forward_x * camera_speed * game->delta;
		game->camera.y += forward_y * camera_speed * game->delta; 
	}
	if (game->input.key_a){
		game->camera.x -= right_x * camera_speed * game->delta;
		game->camera.y -= right_y * camera_speed * game->delta;
	}
	if (game->input.key_d){
		game->camera.x += right_x * camera_speed * game->delta;
		game->camera.y += right_y * camera_speed * game->delta;
	}

	if (game->input.mouse_wheel_y != 0.0f){
		const float zoom_speed = 1.0f;
		const float min_camera_z = 3.0f;
		const float max_camera_z = 25.0f;

		game->camera.z -= game->input.mouse_wheel_y * zoom_speed;

		if (game->camera.z < min_camera_z){
			game->camera.z = min_camera_z;
		}
		if (game->camera.z > max_camera_z){
			game->camera.z = max_camera_z;
		}
	}

	if (game->input.mouse_right_pressed){
		int center_x = (int)game->vk.swapchain_extent.width / 2;
		int center_y = (int)game->vk.swapchain_extent.height / 2;

		if (!Helper_mouseToWorldOnZPlane(game,
						 center_x,
						 center_y,
						 0.0f,
						 &game->orbit_anchor_x,
						 &game->orbit_anchor_y)){
			game->orbit_anchor_x = game->camera.x;
			game->orbit_anchor_y = game->camera.y;
		}
	}

	if (game->input.mouse_right_down && game->input.mouse_delta_x != 0){
		const float rotate_speed = 0.25f;

		float old_yaw = game->camera.yaw;
		float new_yaw = old_yaw + (float)game->input.mouse_delta_x * rotate_speed;

		float old_yaw_rad = old_yaw * (3.14159265f / 180.0f);
		float new_yaw_rad = new_yaw * (3.14159265f / 180.0f);
		float yaw_delta = new_yaw_rad - old_yaw_rad;

		float dx = game->camera.x - game->orbit_anchor_x;
		float dy = game->camera.y - game->orbit_anchor_y;

		float dist = sqrtf(dx * dx + dy * dy);
		float angle = atan2f(dy, dx);

		angle += yaw_delta;

		game->camera.x = game->orbit_anchor_x + cosf(angle) * dist;
		game->camera.y = game->orbit_anchor_y + sinf(angle) * dist;
		game->camera.yaw = new_yaw;
	}

	if (game->held_villager_index < 0 && game->input.mouse_left_pressed){
		int clicked_index = findClickedVillager(game, game->input.mouse_x, game->input.mouse_y);
		if (clicked_index >= 0){
			Villager *villager = &game->villagers[clicked_index];

			float mouse_world_x;
			float mouse_world_y;

			float villager_held_height = 1.5f;

			game->held_villager_index = clicked_index;
			
			villager->state = VILLAGER_HELD;
			villager->work_timer = 0.0f;
			villager->job = JOB_NONE;
			villager->target_site_index = -1;

			villager->body.z = villager_held_height;

			if (Helper_mouseToWorldOnZPlane(game,
							game->input.mouse_x,
							game->input.mouse_y,
							villager_held_height,
							&mouse_world_x,
							&mouse_world_y)){
				villager->body.x = mouse_world_x;
				villager->body.y = mouse_world_y;
			}
			game->held_offset_x = 0.0f;
			game->held_offset_y = 0.0f;
			
			game->held_prev_x = villager->body.x;
			game->held_prev_y = villager->body.y;
			game->held_throw_vx = 0.0f;
			game->held_throw_vy = 0.0f;
			game->held_has_prev_position = true;

			villager->body.vx = 0.0f;
			villager->body.vy = 0.0f;
			villager->body.vz = 0.0f;
			villager->body.on_ground = false;

			game->input.mouse_delta_x = 0;
			game->input.mouse_delta_y = 0;
		}
	}
	updateVillagers(game);
	buildRenderList(game);
	updateWindowTitle(game);
}




int main(int argc, char *argv[]){
	GameState game = {0};

	(void)argc;
	(void)argv;

	if (!SDL_Init(SDL_INIT_VIDEO)){
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	game.window_width = 1280;
	game.window_height = 720;
	game.running = true;
	game.held_villager_index = -1;

	game.window = SDL_CreateWindow(
		"Malevolence_and_Benevolence",
		game.window_width,
		game.window_height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);

	if (!game.window){
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	if (!createInstance(&game)){
		SDL_DestroyWindow(game.window);
		SDL_Quit();
		return 1;
	}
	if (!createSurface(&game) ||
	    !pickPhysicalDevice(&game) ||
	    !createLogicalDevice(&game) ||
	    !createSwapchain(&game) ||
	    !createImageViews(&game) ||
	    !createRenderPass(&game) ||
	    !createDescriptorSetLayout(&game) ||
	    !createGraphicsPipeline(&game) ||
	    !createDepthResources(&game) ||
	    !createFramebuffers(&game) ||
	    !createCommandPool(&game) ||
	    !createMeshBuffers(&game) ||
	    !createInstanceBuffer(&game) ||
	    !createUniformBuffers(&game) ||
	    !createDescriptorPool(&game) ||
	    !createDescriptorSets(&game) ||
	    !createCommandBuffers(&game) ||
	    !createSyncObjects(&game)) {

		cleanupVulkan(&game);
		SDL_DestroyWindow(game.window);
		SDL_Quit();
		return 1;
	}

	game.camera.x = 0.0f;
	game.camera.y = 0.0f;
	game.camera.z = 8.0f;
	game.camera.pitch = 55.0f;
	game.camera.yaw = 0.0f;

	game.temple.x = 0.0f;
	game.temple.y = 0.0f;
	game.temple.z = 2.0f;
	game.temple.size_x = 4.0f;
	game.temple.size_y = 4.0f;
	game.temple.size_z = 4.0f;

	game.food_amount = 0;
	game.belief_amount = 0;

	game.villager_count = 3;
	initVillager(&game.villagers[0], 6.0f, 5.0f);
	initVillager(&game.villagers[1], 8.0f, 3.0f);
	initVillager(&game.villagers[2], 4.0f, 7.0f);

	game.work_site_count = 2;
	initWorkSite(&game.work_sites[0], 12.0f, 2.0f, SITE_FOOD);
	initWorkSite(&game.work_sites[1], -6.0f, 2.0f, SITE_WORSHIP);

	buildRenderList(&game);
	updateWindowTitle(&game);
	game.last_ticks = SDL_GetTicks();

	while (game.running){
		Uint64 current_ticks = SDL_GetTicks();
		game.delta = (float)(current_ticks - game.last_ticks) / 1000.0f;
		game.last_ticks = current_ticks;

		processEvents(&game);
		update(&game);
		if (!drawFrame(&game)){
			fprintf(stderr, "drawFrame failed\n");
			break;
		}
		SDL_Delay(1);
	}

	cleanupVulkan(&game);
	SDL_DestroyWindow(game.window);
	SDL_Quit();
	return 0;
}


























