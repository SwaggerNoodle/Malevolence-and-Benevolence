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
typedef struct GameState {
	bool running;
	
	SDL_Window *window;
	int window_width;
	int window_height;

	float delta;
	Uint64 last_ticks;

	InputState input;
} GameState;




static void processEvents(GameState *game){

	SDL_Event event;

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
			case SDL_EVENT_MOUSE_BUTTON_UP:
				switch (event.button.button){

					case SDL_BUTTON_LEFT:
						game->input.mouse_left = false;
						break;
					case SDL_BUTTON_RIGHT:
						game->input.mouse_right = false;
						break;
				}
		}
	}
}
static void update(GameState *game){
	
	float speed = 200.0f;

	(void)speed;
}

static void render(GameState *game){
	(void)game;
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

	SDL_DestroyWindow(game.window);
	SDL_Quit();
	return 0;
}







































