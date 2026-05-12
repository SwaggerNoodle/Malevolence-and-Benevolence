#include <SDL3/SDL_main.h>

#include "common.h"
#include "game.h"
#include "rendering.h"
#include "helpers.h"



static void initVillager(Villager *villager, float x, float y){
	villager->x = x;
	villager->y = y;
	villager->z = 0.5f;

	villager->size_x = 0.8f;
	villager->size_y = 0.8f;
	villager->size_z = 1.0f;

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
					  game->villagers[i].x,
					  game->villagers[i].y,
					  game->villagers[i].z,
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
	villager->z = 0.5f;
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
	
	float dx = villager->target_x - villager->x;
	float dy = villager->target_y - villager->y;
	float dist = sqrtf(dx * dx + dy * dy);

	villager->z = 0.5f;

	if (dist > 0.05f){
		float nx = dx / dist;
		float ny = dy / dist;

		villager->x += nx * villager->move_speed * game->delta;
		villager->y += ny * villager->move_speed * game->delta;
	}
	else {
		villager->x = villager->target_x;
		villager->y = villager->target_y;
		villager->z = 0.5f;
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
		villager->z = 0.5f;
		return;
	}
	site = &game->work_sites[villager->target_site_index];
	if (!site->active){
		villager->state = VILLAGER_IDLE;
		villager->job = JOB_NONE;
		villager->target_site_index = -1;
		villager->z = 0.5f;
		return;
	}
	switch (villager->job){
		case JOB_GATHER_FOOD: work_cycle = 2.0f; break;
		case JOB_WORSHIP: work_cycle = 3.0f; break;
		default: work_cycle = 2.0f; break;
	}
	
	villager->x = villager->target_x;
	villager->y = villager->target_y;
	villager->z = 0.5f + 0.05f * sinf(villager->work_timer * 8.0f);
	
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

	float villager_held_height = 1.5f;

	if (game->held_villager_index < 0 || game->held_villager_index >= game->villager_count){
		game->held_villager_index = -1;
		return;
	}

	villager = &game->villagers[game->held_villager_index];
	villager->state = VILLAGER_HELD;
	villager->job = JOB_NONE;
	villager->target_site_index = -1;
	villager->z = villager_held_height;

	if (Helper_mouseToWorldOnZPlane(game,
					game->input.mouse_x,
					game->input.mouse_y,
					villager_held_height,
					&mouse_world_x,
					&mouse_world_y)){
		villager->x = mouse_world_x;
		villager->y = mouse_world_y;
	}

	if (game->input.mouse_left_released){
		site_index = findWorkSiteAtScreenPos(game, game->input.mouse_x, game->input.mouse_y);
		if (site_index >= 0){
			assignVillagerToSite(game, game->held_villager_index, site_index);
		}
		else {
			villager->state = VILLAGER_IDLE;
			villager->job = JOB_NONE;
			villager->target_site_index = -1;
			villager->z = 0.5f;
		}
		game->held_villager_index = -1;
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
				villager->z = 0.5f;
				break;
			case VILLAGER_HELD:
				break;
			case VILLAGER_WALKING_TO_TARGET:
				updateWalkingVillager(game, villager);
				break;
			case VILLAGER_WORKING:
				updateWorkingVillager(game, villager);
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

	if (game->input.key_w){ game->camera.y -= camera_speed * game->delta; }
	if (game->input.key_s){ game->camera.y += camera_speed * game->delta; }
	if (game->input.key_a){ game->camera.x -= camera_speed * game->delta; }
	if (game->input.key_d){ game->camera.x += camera_speed * game->delta; }

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

			villager->z = villager_held_height;

			if (Helper_mouseToWorldOnZPlane(game,
							game->input.mouse_x,
							game->input.mouse_y,
							villager_held_height,
							&mouse_world_x,
							&mouse_world_y)){
				villager->x = mouse_world_x;
				villager->y = mouse_world_y;
			}
			game->held_offset_x = 0.0f;
			game->held_offset_y = 0.0f;
			
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


























