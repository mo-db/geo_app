#include <SDL3/SDL.h>
#include <stdio.h>

/*
 * I could use a LL instead of array of pointers for objects?
 * So i would be able to delete objects easily without fragmentation
*/

#define TEXTURE_SIZE 150
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define OBJECTS_MAX 1000

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture_window = NULL;
/* SDL_Surface *surface = NULL; */

const bool* keyStates = NULL;
int* numkeys = NULL;
int keep_running = true;
bool lock = 0;
int pix_cnt = 0;
bool line_first_point = 1;

enum mode_list {
	NORMAL,
	POINT,
	LINE,
};
uint8_t mode = NORMAL;

typedef struct {
	int mouse_x;
	int mouse_y;
	bool mouse_left_down;
	bool mouse_right_down;
} Appstate;

typedef struct {
	double x;
	double y;
	uint32_t color;
} Point_2d;

typedef struct {
	Point_2d p1;
	Point_2d p2;
	uint32_t color;
} Line_2d;

typedef struct {
	Point_2d* p2d;
	uint32_t p2d_index;
	Line_2d* l2d;
	uint32_t l2d_index;
} Objects;

int sdl_init();
int appstate_init(Appstate* appstate);
int objects_init(Objects* objects);
int objects_destroy(Objects* objects);
void process_event(Appstate* appstate, Objects* objects);
void draw(Appstate* appstate, Objects* objects);

int objects_create(Appstate* appstate, Objects* objects);

int main()
{
	sdl_init();
	Appstate appstate;
	appstate_init(&appstate);
	Objects objects;
	objects_init(&objects);

	while (keep_running) {
		keyStates = SDL_GetKeyboardState(numkeys);
		process_event(&appstate, &objects);
		objects_create(&appstate, &objects);
		draw(&appstate, &objects);
		SDL_DelayNS(1000);
	}
	objects_destroy(&objects);
	SDL_Quit();
	return 0;
}

int sdl_init()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    texture_window = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!texture_window) {
        SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
	return 1;
}

int appstate_init(Appstate* appstate)
{
	appstate->mouse_x = 0;
	appstate->mouse_y = 0;
	appstate->mouse_left_down = 0;
	appstate->mouse_right_down = 0;
	return 1;
}

int objects_init(Objects* objects)
{
	// init points to 0
	objects->p2d = SDL_malloc(sizeof(Point_2d) * OBJECTS_MAX);
	for (int i = 0; i < OBJECTS_MAX; i++) {
		objects->p2d[i].x = 0.0;
		objects->p2d[i].y = 0.0;
	}
	objects->p2d_index = 0;

	// init lines to 0
	objects->l2d = SDL_malloc(sizeof(Line_2d) * OBJECTS_MAX);
	for (int i = 0; i < OBJECTS_MAX; i++) {
		objects->l2d[i].p1.x = 0.0;
		objects->l2d[i].p1.y = 0.0;
		objects->l2d[i].p2.x = 0.0;
		objects->l2d[i].p2.y = 0.0;
	}
	objects->l2d_index = 0;

	return 1;
}

void process_event(Appstate* appstate, Objects* objects)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
       switch (event.type) {
          case SDL_EVENT_QUIT:
               keep_running = false;
               break;

          case SDL_EVENT_KEY_DOWN:
               switch(event.key.key) {
                  case SDLK_ESCAPE: 
					  if (!event.key.repeat) {
						  mode = NORMAL;
					  }
					  break;
				  case SDLK_P:
					  if (!event.key.repeat) {
						  mode = POINT;
					  }
					  break;
                  case SDLK_L:
					  if (!event.key.repeat) {
						  mode = LINE;
					  }
					  break;
                  case SDLK_D:
					  if (!event.key.repeat) {
						  printf("mode: %d\n", mode);
						  printf("mouse x,y: %d,%d\n", 
								  appstate->mouse_x, appstate->mouse_y);
						  printf("mouse down: %d\n", 
								  appstate->mouse_left_down);
						  printf("p1 x,y: %f,%f\n", objects->p2d[0].x, objects->p2d[0].y);
						  printf("p2 x,y: %f,%f\n", objects->p2d[1].x, objects->p2d[1].y);
						  printf("l1: (%f,%f/%f,%f)\n", 
								  objects->l2d[0].p1.x, objects->l2d[0].p1.y,
								  objects->l2d[0].p2.x, objects->l2d[0].p2.y);
					  }
					  break;
               }
               break;
		  case SDL_EVENT_MOUSE_MOTION:
			   appstate->mouse_x = event.motion.x;
			   appstate->mouse_y = event.motion.y;
			   break;
		  case SDL_EVENT_MOUSE_BUTTON_DOWN:
			   appstate->mouse_left_down = event.button.down;
			   break;
		  case SDL_EVENT_MOUSE_BUTTON_UP:
			   appstate->mouse_left_down = event.button.down;
			   lock = 0;
			   break;
      }
	}
}

// 
int objects_create(Appstate* appstate, Objects* objects)
{
	switch(mode) {
		case NORMAL:
			return 1;
			break;
		case POINT:
			if (appstate->mouse_left_down && lock == 0) {
				lock = 1;
				objects->p2d[objects->p2d_index].x = appstate->mouse_x;
				objects->p2d[objects->p2d_index].y = appstate->mouse_y;
				objects->p2d_index++;
			} else if (!appstate->mouse_left_down) {
				lock = 0;
			}
			break;
		case LINE:
			if (appstate->mouse_left_down && !lock) { // make point
				if (line_first_point) {
					objects->l2d[objects->l2d_index].p1.x = appstate->mouse_x + 0.5;
					objects->l2d[objects->l2d_index].p1.y = appstate->mouse_y + 0.5;
					line_first_point = 0;
					lock = 1;
				} else {
					objects->l2d[objects->l2d_index].p2.x = appstate->mouse_x + 0.5;
					objects->l2d[objects->l2d_index].p2.y = appstate->mouse_y + 0.5;
					line_first_point = 1;
					lock = 1;
					objects->l2d_index++;
				}
			} else if (!appstate->mouse_left_down && lock) {
				lock = 0;
			}
			break;
	}
	return 1;
}

void draw(Appstate* appstate, Objects* objects)
{
    SDL_FRect dst_rect;
	void* pixels;
	int pitch;

    SDL_RenderClear(renderer);  /* start with a blank canvas. */
    if (SDL_LockTexture(texture_window, NULL, &pixels, &pitch)) {
		uint32_t* pixs = pixels;
		if (pix_cnt == (WINDOW_WIDTH * WINDOW_HEIGHT)) { pix_cnt = 0; }
		pixs[pix_cnt++] = 0xFF0000FF;
		/* int px = 0; */
		/* int py = 0; */
		/* for (int i = 0; i < objects->p2d_index; i++) { */
		/* 	px = objects->p2d[i].x; */
		/* 	py = objects->p2d[i].y; */
		/* } */
        SDL_UnlockTexture(texture_window);  /* upload the changes (and frees the temporary surface)! */
    }
    /* Center this one. It'll draw the latest version of the texture we drew while it was locked. */
	dst_rect.x = 0.0;
	dst_rect.y = 0.0;
    dst_rect.w = (float) WINDOW_WIDTH;
    dst_rect.h = (float) WINDOW_HEIGHT;
    SDL_RenderTexture(renderer, texture_window, NULL, &dst_rect);
    SDL_RenderPresent(renderer);  /* put it all on the screen! */
}

int objects_destroy(Objects* objects)
{
	SDL_free(objects->p2d);
	SDL_free(objects->l2d);
	return 1;
}
