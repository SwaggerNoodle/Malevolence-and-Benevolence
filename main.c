#include <stdbool.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


typedef struct InputState {
	bool key_w;
	bool key_a;
	bool key_s;
	bool key_d;

	bool mouse_left;
	bool mouse_right;

	int mouse_x;
	int mouse_y;
	
	int mouse_delta_x;
	int mouse_delta_y;
} InputState;
typedef struct Camera {
	float x;
	float y;
} Camera;
typedef struct WorldObject {
	float x;
	float y;
	float w;
	float h;
} WorldObject;

typedef struct GameState {
	bool running;
	
	SDL_Window *window;
	SDL_Renderer *renderer;

	int window_width;
	int window_height;

	float delta;
	Uint64 last_ticks;

	InputState input;
	Camera camera;

	WorldObject temple;
	WorldObject villager;
} GameState;




static void processEvents(GameState *game){

	SDL_Event event;

	game->input.mouse_delta_x = 0;
	game->input.mouse_delta_y = 0;

	while (SDL_PollEvent(&event)){
		switch (event.type){
			
			case SDL_EVENT_QUIT:
				game->running = false;
				break;
			case SDL_EVENT_KEY_DOWN:
				switch (event.key.key){

					case SDLK_W:
						game->input.key_w = true;
						break;
					case SDLK_A:
						game->input.key_a = true;
						break;
					case SDLK_S:
						game->input.key_s = true;
						break;
					case SDLK_D:
						game->input.key_d = true;
						break;
				}
				break;
			case SDL_EVENT_KEY_UP:			
				switch (event.key.key){

                                        case SDLK_W:
                                                game->input.key_w = false;
                                                break;
                                        case SDLK_A:
                                                game->input.key_a = false;
                                                break;
                                        case SDLK_S:
                                                game->input.key_s = false;
                                                break;
                                        case SDLK_D:
                                                game->input.key_d = false;
						break;
				}
				break;
			case SDL_EVENT_MOUSE_MOTION:
				game->input.mouse_x = event.motion.x;
				game->input.mouse_y = event.motion.y;
				game->input.mouse_delta_x = event.motion.xrel;
				game->input.mouse_delta_y = event.motion.yrel;
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				switch (event.button.button){

					case SDL_BUTTON_LEFT:
						game->input.mouse_left = true;
						break;
					case SDL_BUTTON_RIGHT:
						game->input.mouse_right = true;
						break;
				}
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				switch (event.button.button){

					case SDL_BUTTON_LEFT:
						game->input.mouse_left = false;
						break;
					case SDL_BUTTON_RIGHT:
						game->input.mouse_right = false;
						break;
				}
				break;
		}
	}
}



static void update(GameState *game){
	float camera_speed = 200.0f;

	if (game->input.key_w){
		game->camera.y -= camera_speed * game->delta;
	}
	if (game->input.key_s){
		game->camera.y += camera_speed * game->delta;
	}
	if (game->input.key_a){
		game->camera.x -= camera_speed * game->delta;
	}
	if (game->input.key_d){
		game->camera.x += camera_speed * game->delta;
	}
}



static void render(GameState *game){
	SDL_FRect rect;
	int x;
	int y;

	//Sky/Backround
	SDL_SetRenderDrawColor(game->renderer, 100, 150, 255, 255);
	SDL_RenderClear(game->renderer);

	//Ground plane in world coords
	SDL_SetRenderDrawColor(game->renderer, 40, 140, 60, 255);
	rect.x = 0.0f - game->camera.x;
	rect.y = 0.0f - game->camera.y;
	rect.w = 2000.0f;
	rect.h = 2000.0f;
	SDL_RenderFillRect(game->renderer, &rect);

	//World Grid
	SDL_SetRenderDrawColor(game->renderer, 25, 100, 45, 255);

	for (x = 0; x <= 2000; x += 100){
		SDL_RenderLine(
				game->renderer,
				(float)x - game->camera.x,
				0.0f - game->camera.y,
				(float)x - game->camera.x,
				2000.0f - game->camera.y
			      );
	}
	for (y = 0; y <= 2000; y += 100){
		SDL_RenderLine(
				game->renderer,
				0.0f - game->camera.x,
				(float)y - game->camera.y,
				2000.0f - game->camera.x,
				(float)y - game->camera.y
			      );
	}
	//Temple Marker
	SDL_SetRenderDrawColor(game->renderer, 170, 170, 170, 255);
	rect.x = game->temple.x - game->camera.x;
	rect.y = game->temple.y - game->camera.y;
	rect.w = game->temple.w;
	rect.h = game->temple.h;
	SDL_RenderFillRect(game->renderer, &rect);

	//Villager Marker
	SDL_SetRenderDrawColor(game->renderer, 255, 230, 80, 255);
	rect.x = game->villager.x - game->camera.x;
	rect.y = game->villager.y - game->camera.y;
	rect.w = game->villager.w;
	rect.h = game->villager.h;
	SDL_RenderFillRect(game->renderer, &rect);

	SDL_RenderPresent(game->renderer);
}






int main(int argc, char *argv[]){
	(void)argc;
	(void)argv;
	if(!SDL_Init(SDL_INIT_VIDEO)){
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}


	GameState game = {0};

	game.window_width = 1280;
	game.window_height = 720;
	game.running = true;
	game.window = SDL_CreateWindow(
		"Main Game Window",
		game.window_width,
		game.window_height,
		SDL_WINDOW_RESIZABLE
	);
	if (game.window == NULL){
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	game.renderer = SDL_CreateRenderer(game.window, NULL);
	if (game.renderer == NULL){
		fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(game.window);
		SDL_Quit();
		return 1;
	}

	game.camera.x = 0.0f;
	game.camera.y = 0.0f;

	game.temple.x = 800.0f;
	game.temple.y = 800.0f;
	game.temple.w = 140.0f;
	game.temple.h = 140.0f;

	game.villager.x = 1000.0f;
	game.villager.y = 900.0f;
	game.villager.w = 20.0f;
	game.villager.h = 20.0f;
	
	game.last_ticks = SDL_GetTicks();

	while (game.running){
		Uint64 current_ticks = SDL_GetTicks();
		game.delta = (float)(current_ticks - game.last_ticks) / 1000.0f;
		game.last_ticks = current_ticks;

		processEvents(&game);
		update(&game);
		render(&game);

		SDL_Delay(1);
	}
	
	SDL_DestroyRenderer(game.renderer);
	SDL_DestroyWindow(game.window);
	SDL_Quit();
	return 0;
}



























