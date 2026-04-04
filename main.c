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
	int mouse_z;
	
	int mouse_delta_x;
	int mouse_delta_y;
	int mouse_delta_z;
} InputState;
typedef struct GameState {
	bool running;
	
	SDL_Window *window;
	int window_width;
	int window_height;

	float delta;

	InputState input;
} GameState;


static void processEvents(bool *running){

	SDL_Event event;

	while (SDL_PollEvent(&event)){
		if (event.type == SDL_EVENT_QUIT){
			*running = false;
		}
	}
}




static void update(float delta){
	(void)delta;
}

static void render(void){
}




int main(int argc, char *argv[]){
	(void)argc;
	(void)argv;

	
	if(!SDL_Init(SDL_INIT_VIDEO)){
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	
	SDL_Window *window = SDL_CreateWindow(
		"Main Game Window",
		1280,
		720,
		SDL_WINDOW_RESIZABLE
	);
	if (window == NULL){
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}


	bool running = true;
	Uint64 last_ticks = SDL_GetTicks();

	while (running){
		Uint64 current_ticks = SDL_GetTicks();
		float delta = (float)(current_ticks - last_ticks) / 1000.0f;
		last_ticks = current_ticks;

		processEvents(&running);
		update(delta);
		render();

		SDL_Delay(1);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}







































