#include <SDL3/SDL.h>
#include <stdio.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480 

#define OBJECTS_MAX 1000
#define TEXT_BUF_MAX 1024

/* #define DEBUG */

const bool *g_key_states = NULL;
int *g_num_keys = NULL;
int keep_running = true;
bool lock = 0;
int pix_cnt = 0;
bool line_first_point = 1;
bool first_run = 1;
int scale_var = 3;

typedef enum {
  NORMAL,
  POINT,
  LINE,
	CIRCLE,
} AppMode;

typedef enum {
	SELECTED,
	NOT_SELECTED,
} ObjectStatus;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *window_texture;
	double density;
	uint32_t w_pixels;
	uint32_t h_pixels;
	bool keep_running;
	AppMode mode;

  double mouse_x;
  double mouse_y;
  bool mouse_left_down;
  bool mouse_right_down;

  bool text_input;
  char text[TEXT_BUF_MAX];
} AppState;

typedef struct {
  double x;
  double y;
  uint32_t color;
	ObjectStatus status;
} Point2D;

typedef struct {
  Point2D p0;
  Point2D p1;
  uint32_t color;
	ObjectStatus status;
} Line2D;

typedef struct {
	Point2D center;
	double radius;
  uint32_t color;
	ObjectStatus status;
} Circle2D;

typedef struct {
	void *data;
	uint32_t length;
	uint32_t capacity;
	size_t object_size;
} ObjectBuf;

typedef struct {
	ObjectBuf point_2D_buf;
	ObjectBuf line_2D_buf;
	ObjectBuf circle_2D_buf;
} Objects;

void vector_append(ObjectBuf *ctx);
void vector_pop(ObjectBuf *ctx, uint32_t index);
void vector_destroy(ObjectBuf *ctx);

void calc_intersection(ObjectBuf *buf);

int app_init(AppState *app_state);
int objects_init(Objects *objects);
void process_event(AppState *app_state, Objects *objects);
void draw(AppState *app_state, Objects *objects);
int objects_create(AppState *app_state, Objects *objects);

int main() {
  AppState app_state;
  Objects objects;
  app_init(&app_state);
  objects_init(&objects);

  while (keep_running) {



    g_key_states = SDL_GetKeyboardState(g_num_keys);
    process_event(&app_state, &objects);
    objects_create(&app_state, &objects);
#ifndef DEBUG
    draw(&app_state, &objects);
		/* float x,y; */
		/* SDL_GetGlobalMouseState(&x, &y); */
		/* printf("glob mouse pos: %f, %f\n", x,y); */
#endif
    SDL_DelayNS(1000);
  }
  SDL_Quit();
  return 0;
}

int app_init(AppState *app_state) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app_state->window = NULL;
	app_state->renderer = NULL;
  if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures",
				WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY |
				SDL_WINDOW_MOUSE_CAPTURE, &app_state->window, &app_state->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
	app_state->w_pixels = WINDOW_WIDTH * SDL_GetWindowPixelDensity(app_state->window);
	app_state->h_pixels = WINDOW_HEIGHT * SDL_GetWindowPixelDensity(app_state->window);

	// texture create with pixels and not window size -> retina display scaling
  app_state->window_texture = SDL_CreateTexture(
			app_state->renderer, SDL_PIXELFORMAT_XRGB8888,
			SDL_TEXTUREACCESS_STREAMING, 
			app_state->w_pixels, app_state->h_pixels);

	if (!app_state->window_texture) {
    SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
	}

	app_state->keep_running = true;
  app_state->mouse_x = 0;
  app_state->mouse_y = 0;
  app_state->mouse_left_down = 0;
  app_state->mouse_right_down = 0;
  app_state->density = SDL_GetWindowPixelDensity(app_state->window);
  app_state->text_input = false;
  for (int i = 0; i < TEXT_BUF_MAX; i++) {
    app_state->text[i] = 0;
  }
  return 1;
}

// compound literal use (C99)
int objects_init(Objects *objects) {
	objects->point_2D_buf = (ObjectBuf){ .object_size = sizeof(Point2D) };
	objects->line_2D_buf = (ObjectBuf){ .object_size = sizeof(Line2D) };
	objects->circle_2D_buf = (ObjectBuf){ .object_size = sizeof(Circle2D) };
  return 1;
}

void process_event(AppState *app_state, Objects *objects) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      keep_running = false;
      break;
    case SDL_EVENT_TEXT_INPUT:
      SDL_strlcat(app_state->text, event.text.text, sizeof(app_state->text));
      printf("%s", event.text.text);
      fflush(stdout); // This will flush any pending printf output
      break;
    case SDL_EVENT_KEY_DOWN:
	  if (app_state->text_input) {
        if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN) {
					/* if (app_state->mode == LINE) { */
					/* 	double* p0x = &objects->l2d->p1.x; */
					/* 	double* p0y = &objects->l2d->p1.y; */
					/* 	double* p1x = &objects->l2d->p2.x; */
					/* 	double* p1y = &objects->l2d->p2.y; */
					/* 	int index = 0; */
					/* 	sscanf(app_state->text, "%d:(%lf,%lf)(%lf,%lf)", &index, */
					/* 			&objects->l2d[index].p1.x, &objects->l2d[index].p1.y, */
					/* 			&objects->l2d[index].p2.x, &objects->l2d[index].p2.y); */
					/* 	char* c = app_state->text; */
					/* 	for (int i = 0; *c != '\0' && i < TEXT_BUF_MAX; i++) { */
					/**/
					/* 	} */
					/* } */
					/*      printf("\ntext input stop\n"); */
					/*      app_state->text_input = false; */
					/*      SDL_StopTextInput(window); */
        }
	  } else {
        switch (event.key.key) {
        case SDLK_T:
					app_state->text_input = true;
					for (int i = 0; i < TEXT_BUF_MAX; i++) {
						app_state->text[i] = 0;
					}
					if (app_state->mode == LINE) {
						printf("line:(p0x,p0y)(p1x,p1y):\n");
					} else {
						printf("text input:\n");
					}
          SDL_StartTextInput(app_state->window);
          break;
        case SDLK_Y:
          printf("text:\n %s\n", app_state->text);
          break;
        case SDLK_ESCAPE:
          if (!event.key.repeat) {
            app_state->mode = NORMAL;
          }
          break;
				case SDLK_C:
          if (!event.key.repeat) {
            app_state->mode = CIRCLE;
          }
					break;
        case SDLK_P:
          if (!event.key.repeat) {
            app_state->mode = POINT;
          }
          break;
        case SDLK_L:
          if (!event.key.repeat) {
            app_state->mode = LINE;
          }
          break;
        case SDLK_D:
          if (!event.key.repeat) {
            printf("app_state->mode: %d\n", app_state->mode);
            printf("mouse x,y: %f,%f\n", app_state->mouse_x, app_state->mouse_y);
            printf("mouse down: %d\n", app_state->mouse_left_down);
						/*       printf("p1 x,y: %f,%f\n", objects->p2d[0].x, objects->p2d[0].y); */
						/*       printf("p2 x,y: %f,%f\n", objects->p2d[1].x, objects->p2d[1].y); */
						/*       printf("l1: (%f,%f/%f,%f)\n", objects->l2d[0].p1.x, */
						/*              objects->l2d[0].p1.y, objects->l2d[0].p2.x, */
						/*              objects->l2d[0].p2.y); */
						/* printf("pix dens: %f\n", SDL_GetWindowPixelDensity(window)); */
						/* int w,h; */
						/* SDL_GetWindowSizeInPixels(window, &w, &h); */
						/* printf("displayscale %f\n", SDL_GetWindowDisplayScale(window)); */
						/* printf("size in pix: %d,%d\n", w, h); */
						/* SDL_GetCurrentRenderOutputSize(renderer, &w, &h); */
						/* printf("output size w,h: %d,%d\n", w,h); */
						/* float x,y; */
						/* SDL_GetGlobalMouseState(&x, &y); */
						/* printf("glob mouse pos: %f, %f\n", x,y); */
          }
          break;
        case SDLK_R:
          draw(app_state, objects);
          break;
				case SDLK_H:
					SDL_SetWindowMouseGrab(app_state->window, 1);
					SDL_HideCursor();
					break;
				case SDLK_J:
					SDL_SetWindowMouseGrab(app_state->window, 0);
					SDL_ShowCursor();
					break;
        case SDLK_S:
          /* SDL_sscanf(stdin, "%lf,%lf %lf,%lf", &objects->l2d->p1.x, */
          /*            &objects->l2d->p1.y, &objects->l2d->p2.x, */
          /*            &objects->l2d->p2.y); */
          /* printf("converted: %lf,%lf %lf,%lf\n", objects->l2d->p1.x, */
          /*        objects->l2d->p1.y, objects->l2d->p2.x, objects->l2d->p2.y); */
          /**/
          break;
        }
      }
      break;
    case SDL_EVENT_MOUSE_MOTION:
      /* app_state->mouse_x = SDL_lround(event.motion.x * app_state->density); */
      /* app_state->mouse_y = SDL_lround(event.motion.y * app_state->density); */
      app_state->mouse_x = SDL_lround(event.motion.x * app_state->density);
      app_state->mouse_y = SDL_lround(event.motion.y * app_state->density);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      app_state->mouse_left_down = event.button.down;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      app_state->mouse_left_down = event.button.down;
      lock = 0;
      break;
    }
  }
}





int objects_create(AppState *app_state, Objects *objects) {
  switch (app_state->mode) {
  case NORMAL:
		// TODO: clear object vars
    return 1;
    break;
  case POINT:
    if (app_state->mouse_left_down && lock == 0) {
      lock = 1;
			vector_append(&objects->point_2D_buf);
			Point2D *point = ((Point2D *)objects->point_2D_buf.data);
			uint32_t index = objects->point_2D_buf.length - 1;
			printf("point %d created\n", index);
			point[index].x = app_state->mouse_x; 
			point[index].y = app_state->mouse_y; 
    } else if (!app_state->mouse_left_down) {
      lock = 0;
    }
    break;
  case LINE:
    if (app_state->mouse_left_down && !lock) { // make point
      if (line_first_point) {
				vector_append(&objects->line_2D_buf);
				Line2D *l = ((Line2D *)objects->line_2D_buf.data);
				int index = objects->line_2D_buf.length - 1;
				l[index].p0.x = app_state->mouse_x; 
				l[index].p0.y = app_state->mouse_y; 
        line_first_point = 0;
        lock = 1;
      } else {
				Line2D *l = ((Line2D *)objects->line_2D_buf.data);
				uint32_t index = objects->line_2D_buf.length - 1;
				printf("index: %d color: %d\n", index, l[index].color);
				l[index].p1.x = app_state->mouse_x; 
				l[index].p1.y = app_state->mouse_y; 
        line_first_point = 1;
        lock = 1;
				printf("line %d created\n", index);
				printf("%f,%f,%f,%f\n", l[index].p0.x, l[index].p0.y, l[index].p1.x, l[index].p1.y);
      }
    } else if (!app_state->mouse_left_down && lock) {
      lock = 0;
    }
    break;
	case CIRCLE:
    if (app_state->mouse_left_down && !lock) { // make point
      if (line_first_point) {
				vector_append(&objects->circle_2D_buf);
				Circle2D *c = ((Circle2D *)objects->circle_2D_buf.data);
				int index = objects->circle_2D_buf.length - 1;
				c[index].center.x = app_state->mouse_x; 
				c[index].center.y = app_state->mouse_y; 
        line_first_point = 0;
        lock = 1;
      } else {
				Circle2D *c = ((Circle2D *)objects->circle_2D_buf.data);
				uint32_t index = objects->circle_2D_buf.length - 1;
				int x = app_state->mouse_x;
				int y = app_state->mouse_y;
				c[index].radius = SDL_sqrt(SDL_pow((c[index].center.x - x), 2.0) + 
						SDL_pow((c[index].center.y - y), 2.0)); 
        line_first_point = 1;
        lock = 1;
				printf("circle %d created\n", index);
				printf("%f,%f,%f\n", c[index].center.x, c[index].center.y, c[index].radius );
      }
    } else if (!app_state->mouse_left_down && lock) {
      lock = 0;
    }
		break;
  }
  return 1;
}

void draw(AppState *app_state, Objects *objects) {
  SDL_FRect dst_rect;
  dst_rect.x = 0.0;
  dst_rect.y = 0.0;
  dst_rect.w = app_state->w_pixels;
  dst_rect.h = app_state->h_pixels;
  SDL_FRect src_rect;
  src_rect.x = 0.0;
  src_rect.y = 0.0;
  src_rect.w = app_state->w_pixels;
  src_rect.h = app_state->h_pixels;

  void *pixels;
  int pitch;
	/* SDL_SetTextureScaleMode(app_state->window_texture, SDL_SCALEMODE_NEAREST); */
	/* SDL_SetRenderLogicalPresentation(app_state->renderer, app_state->w_pixels, */
	/* 		app_state->h_pixels, SDL_LOGICAL_PRESENTATION_DISABLED); */
	/* SDL_SetTextureBlendMode(app_state->window_texture, SDL_BLENDMODE_NONE); */
	/* SDL_SetRenderDrawColor(app_state->renderer, 255, 255, 255, 255); */
  /* SDL_RenderClear(app_state->renderer); */
  if (SDL_LockTexture(app_state->window_texture, NULL, &pixels, &pitch)) {
		/* printf("pitch: %d\n", pitch); */
    uint32_t *pixs = pixels;
		for (int i = 0; i < app_state->w_pixels*app_state->h_pixels; i++) {
			pixs[i] = 0xFFFFFFFF;
		}

    for (int p2d_cnt = 0; p2d_cnt < objects->point_2D_buf.length; p2d_cnt++) {
			int x = SDL_lround(((Point2D *)objects->point_2D_buf.data)[p2d_cnt].x);
			int y = SDL_lround(((Point2D *)objects->point_2D_buf.data)[p2d_cnt].y);
			pixs[x + y * app_state->w_pixels] = 0x00000000;
		}

		Line2D *l = (Line2D *)objects->line_2D_buf.data;
		for (int l2d_cnt = 0; l2d_cnt < objects->line_2D_buf.length; l2d_cnt++) {
			int x0 = SDL_lround(l[l2d_cnt].p0.x);
			int y0 = SDL_lround(l[l2d_cnt].p0.y);
			int x1 = SDL_lround(l[l2d_cnt].p1.x);
			int y1 = SDL_lround(l[l2d_cnt].p1.y);
			double dx = x1 - x0;
			double dy = y1 - y0;
			double y;
			int x;
			double m = dy/dx;
			for (x = x0; x < x1; x++) {
				y = m * (double)(x - x0) + (double)y0;
				if (x < app_state->w_pixels-1 && y < app_state->h_pixels-1) {
					pixs[x + SDL_lround(y) * app_state->w_pixels] = 0x00000000;
				}
			}
		}
		Circle2D *c = (Circle2D *)objects->circle_2D_buf.data;
		for (int c2d_cnt = 0; c2d_cnt < objects->circle_2D_buf.length; c2d_cnt++) {
			int cx = SDL_lround(c[c2d_cnt].center.x);
			int cy = SDL_lround(c[c2d_cnt].center.y);
			for (int x = SDL_lround(cx - c[c2d_cnt].radius); x < SDL_lround(cx + c[c2d_cnt].radius); x++) {
				double val = SDL_pow((c[c2d_cnt].radius), 2.0) - SDL_pow((double)(x - cx), 2.0);
				if (val < 0) {
					val = 0.0;
				}
				/* double y = SDL_sqrt(val); */
				/* printf("%f\n", y); */
        double y_offset = SDL_sqrt(val);
        int y_top = cy - SDL_lround(y_offset);
        int y_bottom = cy + SDL_lround(y_offset);

				printf("y_top:%d, r:%f, \n", y_top, c[c2d_cnt].radius);
        // Draw the top and bottom points of the circle's circumference:
        if (x >= 0 && x < app_state->w_pixels) {
            if (y_top >= 0 && y_top < app_state->h_pixels)
                pixs[x + y_top * app_state->w_pixels] = 0x00000000;
            if (y_bottom >= 0 && y_bottom < app_state->h_pixels)
                pixs[x + y_bottom * app_state->w_pixels] = 0x00000000;
        }
				/* pixs[x + SDL_lround(y_offset) * app_state->w_pixels] = 0x00000000; */
			}
		}

    SDL_UnlockTexture(app_state->window_texture);
  }

	/* SDL_SetRenderDrawColor(app_state->renderer, 255, 0, 0, 255); */
	/* SDL_RenderLine(app_state->renderer, 10, 10, 400, 400); */
  SDL_RenderTexture(app_state->renderer, app_state->window_texture, NULL, NULL);
  SDL_RenderPresent(app_state->renderer);
}

void vector_append(ObjectBuf *buf)
{
	if (buf->length >= buf->capacity) {
		buf->capacity += 32;
		buf->data = SDL_realloc(buf->data, buf->object_size * buf->capacity);
	}
	buf->length++;
}
