#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>

#define MAX_VILLAGERS 32
#define MAX_WORK_SITES 16

typedef enum VillagerState {
	VILLAGER_IDLE = 0,
	VILLAGER_HELD,
	VILLAGER_WALKING_TO_TARGET,
	VILLAGER_WORKING
} VillagerState;
typedef enum VillagerJob {
	JOB_NONE = 0;
	JOB_GATHER_FOOD,
	JOB_WORSHIP
} VillagerJob;
typedef enum WorkSiteType {
	SITE_FOOD = 0;
	SITE_WORSHIP
} WorkSiteType;


typedef struct InputState {
	bool key_w;
	bool key_a;
	bool key_s;
	bool key_d;
	
	bool mouse_left_down;
	bool mouse_right_down;

	bool mouse_left_pressed;
	bool mouse_right_pressed;
	bool mouse_left_released;
	bool mouse_right_released;

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


typedef struct Villager {
	float x;
	float y;
	float z;

	float size_x;
	float size_y;
	float size_z;

	float target_x;
	float target_y;
	float target_z;
	
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

	Villager villagers [MAX_VILLAGERS];
	int villager_count;
	int held_villager_index;

	WorkSite work_sites[MAX_WORK_SITES];
	int work_site_count;

	int food_amount;
	int belief_amount;
} GameState;


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
static void initWorkSite(WorkSite *site, float x, float y, WorkSiteType){
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
	game->input.mouse_right_pressed = false;
	game->input.mouse_left_released = false;
	game->input.mouse_right_released = false;

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
						game->input.mouse_left_pressed = true;
						game->input.mouse_left_down = true;
						break;
					case SDL_BUTTON_RIGHT:
						game->input.mouse_right_pressed = true;
						game->input.mouse_right_down = true;
						break;
				}
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
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

static bool projectWorldToScreen(GameState *game, float wx, float wy, float wz, int *screen_x, int *screen_y){
	int pixel_w;
	int pixel_h;
	float aspect;
	float pitch_rad;
	float x;
	float y;
	float z;
	float view_x;
	float view_y;
	float view_z;
	float ndc_x;
	float ndc_y;

	if (!SDL_GetWindowSizeInPixels(game->window, &pixel_w, &pixel_h)){
		pixel_w = game->window_width;
		pixel_h = game->window_height;
	}
	if (pixel_h <= 0){
		pixel_h = 1;
	}
	aspect = (float)pixel_w / (float)pixel_h;
	pitch_rad = game->camera.pitch * (3.14159265f / 180.0F);

	//Move into camera-relative space
	x = wx - game->camera.x;
	y = wy - game->camera.y;
	z = wz - game->camera.z;

	//Match the same x-axis rotation used in render()
	view_x = x;
	view_y = y *cosf(pitch_rad) + z * sinf(pitch_rad);
	view_z = -y *sinf(pitch_rad) + z * cosf(pitch_rad);

	//Match the fixed camera pull-back in render()
	view_z -= 28.0f;

	//If too close or behind camera, do not project
	if (view_z >= -0.1f){
		return false;
	}


	/*
	 * render() uses:
	 * - near_clip = 0.1f
	 * - half_top  = 0.1f
	 * - half_right = aspect * 0.1f
	 *
	 * That simplifies the perspective divide to:
	 */
	ndc_x = view_x / (aspect * -view_z);
	ndc_y = view_y / (-view_z);

	*screen_x = (int)((ndc_x * 0.5f + 0.5f) * (float)pixel_w);
	*screen_y = (int)((1.0f - (ndc_y * 0.5f + 0.5f)) * (float)pixel_h);

	return true;
}


static void findClickedVillager(GameState *game, int mouse_x, int mouse_y){
	int i;
	int best_index = -1;
	int best_dist2 = 0x7fffffff;
	
	for (i = 0; game->villager_count; i++){
		int screen_x;
		int screen_y;
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
					  &screen_x,
					  &screen_y)){
			continue;
		}
		dx = mouse_x - screen_x;
		dy = mouse_y - screen_y;

		if (dx > -40 && dx < 40 && dy > -40 && dy < 40){
			dist2 = dx * dx + dy * dy;
			if (dist2 < best_dist2){
				best_dist2 = dist2;
				best_index = i;
			}
		}
	}
	return best_index = i;
}
static int findWorkSiteAtScreenPos(Gamestate *game, int mouse_x, int mouse_y){
	int i;
	int best_index = -1;
	int best_dist2 = 0x7fffffff;

	for (i = 0; i < game->work_site_count; i++){
		int screen_x;
		int screen_y;
		int dx;
		int dy;
		int dist2;

		if (!game->work_site[i].active){
			continue;
		}
		if (!projectWorldToScreen(game,
					  game->work_sites[i].x,
					  game->work_sites[i].y,
					  game->work_sites[i].z,
					  &screen_x,
					  &screen_y)){
			coninue;
		}

		dx = mouse_x - screen_x;
		dy = mouse_y - screen_y;

		if (dx > -45 && dx < 45 && dy > -45 && dy < 45){
			dist2 = dx * dx + dy * dy;
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

	if (villager_index < 0 || Villager_index >= game->villager_count){
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
	villager->targey_z = 0.5f;
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
	float dx;
	float dy;
	float dist;
	float nx;
	float ny;

	(void)game;

	dx = villager->target_x - villager->x;
	dy = villager->target_y - villager->y;
	dist = sqrtf(dx * dx + dy * dy);

	villager->z = 0.5f;

	if (dist > 0.05f){
		nx = dx / dist;
		ny = dy / dist;

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



static void update(GameState *game){
	float camera_speed = 10.0f;
	int villager_screen_x;
	int villager_screen_y;
	int dx;
	int dy;
	float drag_scale = 0.08f;

	//Camera movement
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

	//Villager pickup on left click
	if (!game->hand_holding_villager && game->input.mouse_left_pressed){
		if (projectWorldToScreen(
					game,
					game->villager.x,
					game->villager.y,
					game->villager.z,
					&villager_screen_x,
					&villager_screen_y)){
			dx = game->input.mouse_x - villager_screen_x;
			dy = game->input.mouse_y - villager_screen_y;

			//Square click area around villager
			if (dx > -40 && dx < 40 && dy > -40 && dy < 40){
				game->hand_holding_villager = true;
			}
		}
	}
	//If villager held, can drag around
	if (game->hand_holding_villager){
		game->villager.x += (float)game->input.mouse_delta_x * drag_scale;
		game->villager.y -= (float)game->input.mouse_delta_y * drag_scale;

		//lift villager while held
		game->villager.z = 3.0f;

		//Drop villager on release
		if (game->input.mouse_left_released){
			game->hand_holding_villager = false;
			game->villager.z = 0.5f;
		}
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
	if (game->hand_holding_villager){
		drawCube(
			game->villager.x,
			game->villager.y,
			game->villager.z,
			game->villager.size_x,
			game->villager.size_y,
			game->villager.size_z,
			1.00f, 0.40f, 0.25f
			);
	}
	else {
		drawCube(
			game->villager.x,
			game->villager.y,
			game->villager.z,
			game->villager.size_x,
			game->villager.size_y,
			game->villager.size_z,
			1.00f, 0.90f, 0.25f
			);
	}
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



























