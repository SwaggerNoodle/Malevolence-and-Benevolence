#include <stdbool.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>

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
	float z;
	float pitch;
} Camera;
typedef struct WorldObject {
	float x;
	float y;
	float z;
	float size_x;
	float size_y;
	float size_z;
} WorldObject;

typedef struct GameState {
	bool running;
	
	SDL_Window *window;
	SDL_GLContext gl_context;

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
	float camera_speed = 10.0f;

	if (game->input.key_w){
		game->camera.y += camera_speed * game->delta;
	}
	if (game->input.key_s){
		game->camera.y -= camera_speed * game->delta;
	}
	if (game->input.key_a){
		game->camera.x -= camera_speed * game->delta;
	}
	if (game->input.key_d){
		game->camera.x += camera_speed * game->delta;
	}
}


static void drawCube(float x, 	   float y,      float z,       //Position
		     float size_x, float size_y, float size_z,  //Size
		     float r,      float g,      float b){      //Color

	glPushMatrix();

	glTranslatef(x, y, z);
	glScalef(size_x, size_y, size_z);

	glBegin(GL_QUADS);

	//Top
	glColor3f(r * 1.0f, g * 1.0f, b *1.0f);
	glVertex3f(-0.5f, 0.5f, -0.5f);
	glVertex3f(0.5f, 0.5f, -0.5f);
	glVertex3f(0.5f, 0.5f, 0.5f);
	glVertex3f(-0.5f, 0.5f, 0.5f);

	//bottom
	glColor3f(r * 0.45f, g * 0.45f, b * 0.45f);
	glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);

	//front
	glColor3f(r * 0.90f, g * 0.90f, b * 0.90f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);

	//back
	glColor3f(r * 0.60f, g * 0.60f, b * 0.60f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
	
	//left
	glColor3f(r * 0.75f, g * 0.75f, b * 0.75f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);

	//right
	glColor3f(r * 0.82f, g * 0.82f, b * 0.82f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, -0.5f);


	glEnd();
	glPopMatrix();
}


static void render(GameState *game){
	int i;
	int pixel_w;
	int pixel_h;
	float aspect;
	float near_clip;
	float far_clip;
	float half_top;
	float half_right;

	if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)){
		pixel_w = game->window_width;
		pixel_h = game->window_height;
	}
	if (pixel_h <= 0){
		pixel_h = 1;
	}
	aspect = (float)pixel_w / (float)pixel_h;
	near_clip = 0.1f;
	far_clip = 1000.0f;
	half_top = 0.1f;
	half_right = half_top * aspect;

	glViewport(0, 0, pixel_w, pixel_h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-half_right, half_right, -half_top, half_top, near_clip, far_clip);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	/*
	 * Fixed angled overview camera:
	 *  - move the whole world away from the viewer
	 *  - tilt it downward
	 *  - then pan across the world with camera x/y
	 */
	glTranslatef(0.0f, 0.0f, -28.0f);
	glRotatef(-game->camera.pitch, 1.0f, 0.0f, 0.0f);
	glTranslatef(-game->camera.x, -game->camera.y, -game->camera.z);

	//Ground Plane
	glBegin(GL_QUADS);
	glColor3f(0.25f, 0.60f, 0.25f);
	glVertex3f(-50.0f, -50.0f, 0.0f);
	glVertex3f( 50.0f, -50.0f, 0.0f);
	glVertex3f( 50.0f, 50.0f,  0.0f);
	glVertex3f(-50.0f, 50.0f,  0.0f);
	glEnd();
	

	//World Grid
	glBegin(GL_LINES);
	glColor3f(0.15f, 0.35f, 0.15f);

	for (i = -50; i <= 50; i += 2){
		glVertex3f((float)i, -50.0f, 0.01f);
		glVertex3f((float)i,  50.0f,  0.01f);
		
		glVertex3f(-50.0f, (float)i, 0.01f);
		glVertex3f( 50.0f, (float)i, 0.01f);
	}
	glEnd();

	//Temple Marker
	drawCube(
		game->temple.x,
		game->temple.y,
		game->temple.z,
		game->temple.size_x,
		game->temple.size_y,
		game->temple.size_z,
		0.70f, 0.70f, 0.70f
		);

	//Villager Marker
	drawCube(
		game->villager.x,
		game->villager.y,
		game->villager.z,
		game->villager.size_x,
		game->villager.size_y,
		game->villager.size_z,
		1.00f, 0.90f, 0.25f
		);

	SDL_GL_SwapWindow(game->window);
}






int main(int argc, char *argv[]){
	
	GameState game = {0};

	(void)argc;
	(void)argv;
	if(!SDL_Init(SDL_INIT_VIDEO)){
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	game.window_width = 1280;
	game.window_height = 720;
	game.running = true;

	game.window = SDL_CreateWindow(
		"Main Game Window",
		game.window_width,
		game.window_height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if (game.window == NULL){
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	game.gl_context = SDL_GL_CreateContext(game.window);
	if (game.gl_context == NULL){
		fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(game.window);
		SDL_Quit();
		return 1;
	}

	SDL_GL_SetSwapInterval(1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0);
	glClearColor(0.39f, 0.58f, 0.94f, 1.0f);

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

	game.villager.x = 6.0f;
	game.villager.y = 5.f;
	game.villager.z = 0.5f;
	game.villager.size_x = 0.8f;
	game.villager.size_y = 0.8f;
	game.villager.size_z = 1.0f;
	
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
	
	SDL_GL_DestroyContext(game.gl_context);
	SDL_DestroyWindow(game.window);
	SDL_Quit();
	return 0;
}



























