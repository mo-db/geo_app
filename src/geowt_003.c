#include <SDL3/SDL.h>
#include <stdio.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480 

#define OBJECTS_MAX 1000
#define TEXT_BUF_MAX 1024

#define DEFAULT_FG_COLOR 0x00000000
#define DEFAULT_BG_COLOR 0xFFFFFFFF

/* #define DEBUG */

const bool *g_key_states = NULL;
int *g_num_keys = NULL;
int keep_running = true;
bool lock = 0;
int pix_cnt = 0;
bool line_first_point = 1;
bool first_run = 1;
int scale_var = 3;
bool OBJ_CREATED = false;

typedef enum {
	SELECTED,
	HIGHLITED,
	STATUS_COUNT,
} ObjectStatus;

typedef struct {
	void *data;
	uint32_t length;
	uint32_t capacity;
	size_t object_size;
} ObjectBuf;

typedef struct {
  double x;
  double y;
} Point_2D;

typedef struct {
	double x;
	double y;
} Vector_2D;



typedef struct {
  double x;
  double y;
	double angle;
} Angle_Point_2D;

typedef struct {
	Angle_Point_2D *data;
	uint32_t length;
	uint32_t capacity;
} Angle_Point_2D_Array;

typedef struct {
  Point_2D p0;
  Point_2D p1;
  uint32_t color;
	ObjectBuf isp_buf;
	ObjectStatus status_buf[STATUS_COUNT];
} Line_2D;

typedef struct {
	double x;
	double y;
	double radius;
  uint32_t color;
	ObjectBuf isp_buf;
	ObjectStatus status_buf[STATUS_COUNT];
} Circle_2D;



typedef struct {
	ObjectBuf point_2D_buf;
	ObjectBuf line_2D_buf;
	ObjectBuf circle_2D_buf;

	ObjectBuf line_segment_buf;
	ObjectBuf global_isp_buf;

	Angle_Point_2D_Array isp_ary;
} Objects;

typedef enum {
  NORMAL,
  POINT,
  LINE,
	CIRCLE,
} AppMode;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *window_texture;
	double density;
	uint32_t w_pixels;
	uint32_t h_pixels;
	bool keep_running;
	AppMode mode;

	Point_2D mouse;
  /* double mouse_x; */
  /* double mouse_y; */
  bool mouse_left_down;
  bool mouse_right_down;
	bool mouse_click;

  bool text_input;
  char text[TEXT_BUF_MAX];
} AppState;

void vector_append(ObjectBuf *ctx);
void vector_pop(ObjectBuf *ctx, uint32_t index);
void vector_destroy(ObjectBuf *ctx);

void calc_intersection(ObjectBuf *buf);

int app_init(AppState *appstate);
int objects_init(Objects *objects);
void process_event(AppState *appstate, Objects *objects);
void draw(AppState *appstate, Objects *objects);
int objects_create(AppState *appstate, Objects *objects);
int graphics(AppState *appstate, Objects *objects);

/* int create_line( Objects *objects, Point_2D *p0, Point_2D *p1, uint32_t color); */
/* int create_circle( Objects *objects, Point_2D *center, double radius, uint32_t color); */

int main() {
  AppState appstate;
  Objects objects;
  app_init(&appstate);
  objects_init(&objects);

  while (keep_running) {
    g_key_states = SDL_GetKeyboardState(g_num_keys);
		appstate.mouse_click = false;
    process_event(&appstate, &objects);
    objects_create(&appstate, &objects);

		// probably should lock and unlock the texture in main
		// to be able to draw from many places
		graphics(&appstate, &objects);
#ifndef DEBUG
    draw(&appstate, &objects);
		/* float x,y; */
		/* SDL_GetGlobalMouseState(&x, &y); */
		/* printf("glob mouse pos: %f, %f\n", x,y); */
#endif
    SDL_DelayNS(1000);
  }
  SDL_Quit();
  return 0;
}

int app_init(AppState *appstate) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	appstate->window = NULL;
	appstate->renderer = NULL;
  if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures",
				WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY |
				SDL_WINDOW_MOUSE_CAPTURE, &appstate->window, &appstate->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
	appstate->w_pixels = WINDOW_WIDTH * SDL_GetWindowPixelDensity(appstate->window);
	appstate->h_pixels = WINDOW_HEIGHT * SDL_GetWindowPixelDensity(appstate->window);

	// texture create with pixels and not window size -> retina display scaling
  appstate->window_texture = SDL_CreateTexture(
			appstate->renderer, SDL_PIXELFORMAT_XRGB8888,
			SDL_TEXTUREACCESS_STREAMING, 
			appstate->w_pixels, appstate->h_pixels);

	if (!appstate->window_texture) {
    SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
	}

	appstate->keep_running = true;
  appstate->mouse.x = 0;
  appstate->mouse.y = 0;
  appstate->mouse.
  appstate->mouse_left_down = 0;
  appstate->mouse_right_down = 0;
	appstate->mouse_click = false;
  appstate->density = SDL_GetWindowPixelDensity(appstate->window);
  appstate->text_input = false;
  for (int i = 0; i < TEXT_BUF_MAX; i++) {
    appstate->text[i] = 0;
  }
  return 1;
}

// compound literal use (C99)
int objects_init(Objects *objects) {
	objects->point_2D_buf = (ObjectBuf){ .object_size = sizeof(Point_2D) };
	objects->line_2D_buf = (ObjectBuf){ .object_size = sizeof(Line_2D) };
	objects->circle_2D_buf = (ObjectBuf){ .object_size = sizeof(Circle_2D) };

	objects->line_segment_buf = (ObjectBuf){ .object_size = sizeof(Line_2D) };
	objects->global_isp_buf = (ObjectBuf){ .object_size = sizeof(Point_2D) };
	objects->isp_ary = (Angle_Point_2D_Array){}; // init to 0
  return 1;
}

void process_event(AppState *appstate, Objects *objects) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      keep_running = false;
      break;
    case SDL_EVENT_TEXT_INPUT:
      SDL_strlcat(appstate->text, event.text.text, sizeof(appstate->text));
      printf("%s", event.text.text);
      fflush(stdout); // This will flush any pending printf output
      break;
    case SDL_EVENT_KEY_DOWN:
	  if (appstate->text_input) {
        if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN) {
					/* if (appstate->mode == LINE) { */
					/* 	double* p0x = &objects->l2d->p1.x; */
					/* 	double* p0y = &objects->l2d->p1.y; */
					/* 	double* p1x = &objects->l2d->p2.x; */
					/* 	double* p1y = &objects->l2d->p2.y; */
					/* 	int index = 0; */
					/* 	sscanf(appstate->text, "%d:(%lf,%lf)(%lf,%lf)", &index, */
					/* 			&objects->l2d[index].p1.x, &objects->l2d[index].p1.y, */
					/* 			&objects->l2d[index].p2.x, &objects->l2d[index].p2.y); */
					/* 	char* c = appstate->text; */
					/* 	for (int i = 0; *c != '\0' && i < TEXT_BUF_MAX; i++) { */
					/**/
					/* 	} */
					/* } */
					/*      printf("\ntext input stop\n"); */
					/*      appstate->text_input = false; */
					/*      SDL_StopTextInput(window); */
        }
	  } else {
        switch (event.key.key) {
        case SDLK_T:
					appstate->text_input = true;
					for (int i = 0; i < TEXT_BUF_MAX; i++) {
						appstate->text[i] = 0;
					}
					if (appstate->mode == LINE) {
						printf("line:(p0x,p0y)(p1x,p1y):\n");
					} else {
						printf("text input:\n");
					}
          SDL_StartTextInput(appstate->window);
          break;
        case SDLK_Y:
          printf("text:\n %s\n", appstate->text);
          break;
        case SDLK_ESCAPE:
          if (!event.key.repeat) {
            appstate->mode = NORMAL;
          }
          break;
				case SDLK_C:
          if (!event.key.repeat) {
            appstate->mode = CIRCLE;
          }
					break;
        case SDLK_P:
          if (!event.key.repeat) {
            appstate->mode = POINT;
          }
          break;
        case SDLK_L:
          if (!event.key.repeat) {
            appstate->mode = LINE;
          }
          break;
        case SDLK_D:
          if (!event.key.repeat) {
            printf("appstate->mode: %d\n", appstate->mode);
            printf("mouse x,y: %f,%f\n", appstate->mouse.x, appstate->mouse.y);
            printf("mouse down: %d\n", appstate->mouse_left_down);
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
          draw(appstate, objects);
          break;
				case SDLK_H:
					SDL_SetWindowMouseGrab(appstate->window, 1);
					SDL_HideCursor();
					break;
				case SDLK_J:
					SDL_SetWindowMouseGrab(appstate->window, 0);
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
				case SDLK_O:
					for (int i = 0; i < objects->circle_2D_buf.length; i++) {
						Circle_2D circle = ((Circle_2D*)objects->circle_2D_buf.data)[i];
						if (circle.status_buf[HIGHLITED] == true) {
						}
					}
					break;
        }
      }
      break;
    case SDL_EVENT_MOUSE_MOTION:
      /* appstate->mouse_x = SDL_lround(event.motion.x * appstate->density); */
      /* appstate->mouse_y = SDL_lround(event.motion.y * appstate->density); */
      appstate->mouse.x = SDL_lround(event.motion.x * appstate->density);
      appstate->mouse.y = SDL_lround(event.motion.y * appstate->density);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      appstate->mouse_left_down = event.button.down;
			appstate->mouse_click = true;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      appstate->mouse_left_down = event.button.down;
      lock = 0;
      break;
    }
  }
}

// create functions for all defined objects TODO: destroy functions
int create_point(ObjectBuf *object_buf, Point_2D *p)
{
	vector_append(object_buf);
	Point_2D *point = ((Point_2D *)object_buf->data);
	int index = object_buf->length - 1;
	if (!p) {
		point[index].x = 0.0;
		point[index].y = 0.0;
	} else {
		point[index].x = p->x;
		point[index].y = p->y;
	}
	return index;
}

int create_line(ObjectBuf *object_buf, Point_2D *p0, Point_2D *p1, uint32_t color)
{
	vector_append(object_buf);
	Line_2D *line = ((Line_2D *)object_buf->data);
	int index = object_buf->length - 1;
	if (!p0) {
		line[index].p0.x = 0.0; 
		line[index].p0.y = 0.0; 
	} else {
		line[index].p0.x = p0->x; 
		line[index].p0.y = p0->y; 
	}
	if (!p1) {
		line[index].p1.x = 0.0; 
		line[index].p1.y = 0.0; 
	} else {
		line[index].p1.x = p1->x; 
		line[index].p1.y = p1->y; 
	}
	if (!color) {
		line[index].color = DEFAULT_FG_COLOR;
	} else {
		line[index].color = color;
	}
	for (int i = 0; i < STATUS_COUNT; i++) {
		line[index].status_buf[i] = false;
	}
	printf("New line %d created!\n", index);
	return index;
}

int create_circle(ObjectBuf *object_buf, Point_2D *center, double radius, uint32_t color)
{
	// if no radius then 0, if no color then default fg (define it)
	vector_append(object_buf);
	Circle_2D *circle = ((Circle_2D *)object_buf->data);
	int index = object_buf->length - 1;

	circle[index].x = center->x; 
	circle[index].y = center->y;

	if (!radius) {
		circle[index].radius = 0.0;
	} else {
		circle[index].radius = radius;
	}
	if (!color) {
		circle[index].color = DEFAULT_FG_COLOR;
	} else {
		circle[index].color = color;
	}
	for (int i = 0; i < STATUS_COUNT; i++) {
		circle[index].status_buf[i] = false;
	}
	circle[index].isp_buf = (ObjectBuf){ .object_size = sizeof(Point_2D) };
	printf("New circle %d created!\n", index);
	return index;
}

int objects_create(AppState *appstate, Objects *objects) {
  switch (appstate->mode) {
  case NORMAL:
    return 1;
    break;
  case POINT:
    if (appstate->mouse_left_down && lock == 0) {
      lock = 1;
			create_point(&objects->point_2D_buf, &appstate->mouse);
			OBJ_CREATED = true;
    } else if (!appstate->mouse_left_down) {
      lock = 0;
    }
    break;
  case LINE:
    if (appstate->mouse_left_down && !lock) {
      if (line_first_point) {
				int index = create_line(&objects->line_2D_buf, &appstate->mouse, NULL, 0);
        line_first_point = 0;
        lock = 1;
      } else {
				Line_2D *line = ((Line_2D *)objects->line_2D_buf.data);
				int index = objects->line_2D_buf.length - 1;
				line[index].p1.x = appstate->mouse.x; 
				line[index].p1.y = appstate->mouse.y; 
        line_first_point = 1;
        lock = 1;
				OBJ_CREATED = true;
      }
    } else if (!appstate->mouse_left_down && lock) {
      lock = 0;
    }
    break;
	case CIRCLE:
    if (appstate->mouse_left_down && !lock) {
      if (line_first_point) {
				create_circle(&objects->circle_2D_buf, &appstate->mouse, 0.0, 0);
        line_first_point = 0;
        lock = 1;
      } else {
				Circle_2D *circle = ((Circle_2D *)objects->circle_2D_buf.data);
				uint32_t index = objects->circle_2D_buf.length - 1;
				int x = appstate->mouse.x;
				int y = appstate->mouse.y;
				circle[index].radius = SDL_sqrt(SDL_pow((circle[index].x - appstate->mouse.x), 2.0) + 
						SDL_pow((circle[index].y - appstate->mouse.y), 2.0)); 
        line_first_point = 1;
        lock = 1;
				OBJ_CREATED = true;
      }
    } else if (!appstate->mouse_left_down && lock) {
      lock = 0;
    }
		break;
  }
  return 1;
}

// for SDL_qsort
int compare( const void* a, const void* b)
{
    const Point_2D *pA = (const Point_2D *)a;
    const Point_2D *pB = (const Point_2D *)b;
    if (pA->x < pB->x) return -1;
    else if (pA->x > pB->x) return 1;
    else {
        if (pA->y < pB->y) return -1;
        else if (pA->y > pB->y) return 1;
        else return 0;
    }
}

int graphics(AppState *appstate, Objects *objects)
{
	// if objects count changed, run once and update objectscount watcher
	// status of all line segments, dono about this here
	for (int i = 0; i < objects->line_segment_buf.length; i++) {
		// direct indexing every iteration because realloc
		Line_2D segment = ((Line_2D *)objects->line_segment_buf.data)[i];
		Point_2D p0 = { segment.p0.x, segment.p0.y };
		Point_2D p1 = { segment.p1.x, segment.p1.y };
		Vector_2D a = { -(p1.y - p0.y), (p1.x - p0.x) }; // senkrecht zu (p1 - p0)
		Point_2D mouse = { appstate->mouse.x, appstate->mouse.y };

		// relative to pixel desnsity
		double distance = SDL_abs((a.x * mouse.x + a.y * mouse.y + (-a.x * p0.x - a.y * p0.y)) / SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
		/* printf("%f,%f,%f,%f,%f\n", distance, mouse.x, mouse.y, a.x, a.y); */
		SDL_assert(distance <= SDL_max(appstate->w_pixels, appstate->h_pixels));
		if (distance < 20.0 && mouse.x >= p0.x && mouse.x <= p1.x) {
			segment.status_buf[HIGHLITED] = true;
		} else {
			segment.status_buf[HIGHLITED] = false;
		}
		if (segment.status_buf[HIGHLITED] && appstate->mouse_click) {
			if (segment.status_buf[SELECTED]) {
				segment.status_buf[SELECTED] = false;
			}
			segment.status_buf[SELECTED] = true;
		}
		if (segment.status_buf[SELECTED]) {
			segment.color = 0xFF00FF00;
		} else if (segment.status_buf[HIGHLITED]) {
			segment.color = 0xFFFF0000;
		} else {
			segment.color = DEFAULT_FG_COLOR;
		}
	}

	// line-line intersections TODO: not using linear system, refactor later?
	if (OBJ_CREATED == true) {
		objects->line_segment_buf.length = 0;

		Point_2D end_points[1000];
		int end_point_counter;

		// for each line, compare with all others for intersection points
		for (int i = 0; i < objects->line_2D_buf.length; i++) {
			Line_2D base_line = ((Line_2D *)objects->line_2D_buf.data)[i]; // realloc!
			end_point_counter = 0;
			// if this line[i] intersects with any other afterwards, gen circle around
			// end_points first two are p0, p1
			Point_2D p0 = { base_line.p0.x, base_line.p0.y };
			Point_2D p1 = { base_line.p1.x, base_line.p1.y };
			Vector_2D a = { -(p1.y - p0.y), (p1.x - p0.x) }; // senkrecht zu (p1 - p0)

			// get outer endpoints
			end_points[end_point_counter++] = p0;
			end_points[end_point_counter++] = p1;
			for ( int j = 0 ; j < objects->line_2D_buf.length; j++) {
				if (j == i) { continue; }
				Line_2D compare_line = ((Line_2D *)objects->line_2D_buf.data)[j]; // realloc!
				Point_2D p2 = { compare_line.p0.x, compare_line.p0.y };
				Point_2D p3 = { compare_line.p1.x, compare_line.p1.y };
				Vector_2D v = { (p3.x - p2.x), (p3.y - p2.y)};
				double t = 0.0;
				Point_2D is_point = { 0.0, 0.0 };
				// check denominator first to not divide trough 0
				t = (-(-a.x * p0.x - a.y * p0.y) - a.x * p2.x - a.y * p2.y) 
					/ ( a.x * v.x + a.y * v.y);
				is_point.x = p2.x + t * v.x;
				is_point.y = p2.y + t * v.y;

				// test if inside the line endpoints
				if (is_point.x >= p2.x && is_point.x <= p3.x
						&& is_point.x >= p0.x && is_point.x <= p1.x) {
					// test for duplicate intersections
					bool duplicate = false;
					// need epsilon here
					for (int k = 0; k < end_point_counter; k++) {
						if (end_points[k].x == is_point.x && end_points[k].y == is_point.y) {
							duplicate = true;
						}
					}
					if (!duplicate) {
						end_points[end_point_counter++] = is_point;
					}
				}
				for (int k = 0; k < end_point_counter; k++) {
						printf("%f,%f\n", end_points[k].x, end_points[k].y);
				}
				printf("---now sort!\n");
				SDL_qsort(&end_points, end_point_counter, sizeof(Point_2D), compare );
				for (int k = 0; k < end_point_counter; k++) {
						printf("%f,%f\n", end_points[k].x, end_points[k].y);
				}
				for (int k = 0; k < end_point_counter-1; k++) {
						create_line(&objects->line_segment_buf, &end_points[k], &end_points[k+1], 0);
				}
				for (int k = 0; k < end_point_counter; k++) {
						create_point(&objects->global_isp_buf, &end_points[k]);
				}
			}
		} 
	}

	// circle-line intersections
	if (OBJ_CREATED == true) {
		for (int i = 0; i < objects->circle_2D_buf.length; i++) {
			Circle_2D circle = ((Circle_2D *)objects->circle_2D_buf.data)[i]; // realloc
			// reset intersections for each circle to 0
			circle.isp_buf.length = 0;
			Point_2D m = { circle.x, circle.y };
			double radius = circle.radius;


			// test every line and save intersections if not duplicate
			for (int j = 0; j < objects->line_2D_buf.length; j++) {
				Line_2D line = ((Line_2D *)objects->line_2D_buf.data)[j]; // realloc

				Point_2D p0 = { line.p0.x, line.p0.y };
				Point_2D p1 = { line.p1.x, line.p1.y };
				Point_2D v = {p1.x - p0.x, p1.y - p0.y };

				double a = SDL_pow(v.x, 2.0) + SDL_pow(v.y, 2.0);
				double b = 2.0 * v.x * (p0.x - m.x) + 2 * v.y * (p0.y - m.y);
				double c = SDL_pow(p0.x - m.x, 2.0) + SDL_pow(p0.y - m.y, 2.0) - SDL_pow(radius, 2.0);

				double det = SDL_pow(b, 2.0) - 4 * a * c;
				double t0 = 0.0;
				double t1 = 0.0;

				printf("det: %f\n", det);

				// check if 0 and 1 are the same with epsilon
				Point_2D isp_0 = { 0.0, 0.0 };
				Point_2D isp_1= { 0.0, 0.0 };
				bool duplicate = false;
				bool has_isp = false;
				bool double_isp = false;

				if (det > 0.0) {
					t0 = (-b +	SDL_sqrt(det)) / (2 * a);
					t1 = (-b -	SDL_sqrt(det)) / (2 * a);
					isp_0.x = p0.x + t0 * v.x;
					isp_0.y = p0.y + t0 * v.y;

					isp_1.x = p0.x + t1 * v.x;
					isp_1.y = p0.y + t1 * v.y;

					double_isp = true;
					has_isp = true;
				} else if (det == 0.0) { // need epsilon
					t0 = -b / (2 * a);

					isp_0.x = p0.x + t0 * v.x;
					isp_0.y = p0.y + t0 * v.y;

					double_isp = false;
					has_isp = true;
				} else {
					has_isp = false;
				}

				// check duplicate intersection need epsilon here
				if (has_isp) {
					if (double_isp) {
						for (int k = 0; k < circle.isp_buf.length; k++) {
							Point_2D isp = ((Point_2D *)circle.isp_buf.data)[k]; // realloc
							if (isp.x == isp_0.x && isp.y == isp_0.y) {
								duplicate = true;
							}
							if (isp.x == isp_1.x && isp.y == isp_1.y) {
								duplicate = true;
							}
						}
					} else {
						for (int k = 0; k < circle.isp_buf.length; k++) {
							Point_2D isp = ((Point_2D *)circle.isp_buf.data)[k]; // realloc
							if (isp.x == isp_0.x && isp.y == isp_0.y) {
								duplicate = true;
							}
						}
					}
					if (!duplicate) {
						if (double_isp) {
							create_point(&circle.isp_buf, &isp_0);
							create_point(&circle.isp_buf, &isp_1);
						} else {
							create_point(&circle.isp_buf, &isp_0);
						}
					}
				} else {
					continue;
				}
			}
			for (int k = 0; k < circle.isp_buf.length; k++) {
				Point_2D isp = ((Point_2D *)circle.isp_buf.data)[k]; // realloc
				create_point(&objects->global_isp_buf, &isp);
			}
		}
	}
	OBJ_CREATED = false; // this should be somwhere global
	

	// sort cicrle isp buffer
	for (int i = 0; i < objects->circle_2D_buf.length; i++) {
		Circle_2D circle = ((Circle_2D *)objects->circle_2D_buf.data)[i]; // realloc
		double isp_angles[circle.isp_buf.length];

	}
	return 0;
}

void draw(AppState *appstate, Objects *objects) {
  SDL_FRect dst_rect;
  dst_rect.x = 0.0;
  dst_rect.y = 0.0;
  dst_rect.w = appstate->w_pixels;
  dst_rect.h = appstate->h_pixels;
  SDL_FRect src_rect;
  src_rect.x = 0.0;
  src_rect.y = 0.0;
  src_rect.w = appstate->w_pixels;
  src_rect.h = appstate->h_pixels;

  void *pixels;
  int pitch;
	/* SDL_SetTextureScaleMode(appstate->window_texture, SDL_SCALEMODE_NEAREST); */
	/* SDL_SetRenderLogicalPresentation(appstate->renderer, appstate->w_pixels, */
	/* 		appstate->h_pixels, SDL_LOGICAL_PRESENTATION_DISABLED); */
	/* SDL_SetTextureBlendMode(appstate->window_texture, SDL_BLENDMODE_NONE); */
	/* SDL_SetRenderDrawColor(appstate->renderer, 255, 255, 255, 255); */
  /* SDL_RenderClear(appstate->renderer); */
  if (SDL_LockTexture(appstate->window_texture, NULL, &pixels, &pitch)) {
		/* printf("pitch: %d\n", pitch); */
    uint32_t *pixs = pixels;
		for (int i = 0; i < appstate->w_pixels*appstate->h_pixels; i++) {
			pixs[i] = 0xFFFFFFFF;
		}

    for (int p2d_cnt = 0; p2d_cnt < objects->point_2D_buf.length; p2d_cnt++) {
			int x = SDL_lround(((Point_2D *)objects->point_2D_buf.data)[p2d_cnt].x);
			int y = SDL_lround(((Point_2D *)objects->point_2D_buf.data)[p2d_cnt].y);
			pixs[x + y * appstate->w_pixels] = 0x00000000;
		}

		// draw all segments
		Line_2D *l_seg = (Line_2D *)objects->line_segment_buf.data;
		for (int l2d_cnt = 0; l2d_cnt < objects->line_segment_buf.length; l2d_cnt++) {
			int x0 = SDL_lround(l_seg[l2d_cnt].p0.x);
			int y0 = SDL_lround(l_seg[l2d_cnt].p0.y);
			int x1 = SDL_lround(l_seg[l2d_cnt].p1.x);
			int y1 = SDL_lround(l_seg[l2d_cnt].p1.y);
			double dx = x1 - x0;
			double dy = y1 - y0;
			double y;
			int x;
			double m = dy/dx;
			for (x = x0; x < x1; x++) {
				y = m * (double)(x - x0) + (double)y0;
				if (x < appstate->w_pixels-1 && y < appstate->h_pixels-1) {

					pixs[x + SDL_lround(y) * appstate->w_pixels] = l_seg[l2d_cnt].color;
				}
			}
		}

		// draw all primitive lines
		Line_2D *l = (Line_2D *)objects->line_2D_buf.data;
		// not draw atm
		/* for (int l2d_cnt = 0; l2d_cnt < objects->line_2D_buf.length; l2d_cnt++) { */
		/* 	int x0 = SDL_lround(l[l2d_cnt].p0.x); */
		/* 	int y0 = SDL_lround(l[l2d_cnt].p0.y); */
		/* 	int x1 = SDL_lround(l[l2d_cnt].p1.x); */
		/* 	int y1 = SDL_lround(l[l2d_cnt].p1.y); */
		/* 	double dx = x1 - x0; */
		/* 	double dy = y1 - y0; */
		/* 	double y; */
		/* 	int x; */
		/* 	double m = dy/dx; */
		/* 	for (x = x0; x < x1; x++) { */
		/* 		y = m * (double)(x - x0) + (double)y0; */
		/* 		if (x < appstate->w_pixels-1 && y < appstate->h_pixels-1) { */
		/* 			pixs[x + SDL_lround(y) * appstate->w_pixels] = l[l2d_cnt].color; */
		/* 		} */
		/* 	} */
		/* } */


		Circle_2D *c = (Circle_2D *)objects->circle_2D_buf.data;
		for (int c2d_cnt = 0; c2d_cnt < objects->circle_2D_buf.length; c2d_cnt++) {
			int cx = SDL_lround(c[c2d_cnt].x);
			int cy = SDL_lround(c[c2d_cnt].y);
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

				/* printf("y_top:%d, r:%f, \n", y_top, c[c2d_cnt].radius); */
        // Draw the top and bottom points of the circle's circumference:
        if (x >= 0 && x < appstate->w_pixels) {
            if (y_top >= 0 && y_top < appstate->h_pixels)
                pixs[x + y_top * appstate->w_pixels] = 0x00000000;
            if (y_bottom >= 0 && y_bottom < appstate->h_pixels)
                pixs[x + y_bottom * appstate->w_pixels] = 0x00000000;
        }
				/* pixs[x + SDL_lround(y_offset) * appstate->w_pixels] = 0x00000000; */
			}
		}


		// draw circles around global intersection points
		Point_2D *global_isp = (Point_2D*)objects->global_isp_buf.data;
		for (int c2d_cnt = 0; c2d_cnt < objects->global_isp_buf.length; c2d_cnt++) {
			double radius = 20.0; // default radius for highlight points, make global
			int cx = SDL_lround(global_isp[c2d_cnt].x);
			int cy = SDL_lround(global_isp[c2d_cnt].y);
			for (int x = SDL_lround(cx - radius); x < SDL_lround(cx + radius); x++) {
				double val = SDL_pow((radius), 2.0) - SDL_pow((double)(x - cx), 2.0);
				if (val < 0) {
					val = 0.0;
				}
				/* double y = SDL_sqrt(val); */
				/* printf("%f\n", y); */
        double y_offset = SDL_sqrt(val);
        int y_top = cy - SDL_lround(y_offset);
        int y_bottom = cy + SDL_lround(y_offset);

				/* printf("y_top:%d, r:%f, \n", y_top, c[c2d_cnt].radius); */
        // Draw the top and bottom points of the circle's circumference:
        if (x >= 0 && x < appstate->w_pixels) {
            if (y_top >= 0 && y_top < appstate->h_pixels)
                pixs[x + y_top * appstate->w_pixels] = 0x00000000;
            if (y_bottom >= 0 && y_bottom < appstate->h_pixels)
                pixs[x + y_bottom * appstate->w_pixels] = 0x00000000;
        }
				/* pixs[x + SDL_lround(y_offset) * appstate->w_pixels] = 0x00000000; */
			}
		}

    SDL_UnlockTexture(appstate->window_texture);
  }

	/* SDL_SetRenderDrawColor(appstate->renderer, 255, 0, 0, 255); */
	/* SDL_RenderLine(appstate->renderer, 10, 10, 400, 400); */
  SDL_RenderTexture(appstate->renderer, appstate->window_texture, NULL, NULL);
  SDL_RenderPresent(appstate->renderer);
}

void vector_append(ObjectBuf *buf)
{
	if (buf->length >= buf->capacity) {
		buf->capacity += 32;
		buf->data = SDL_realloc(buf->data, buf->object_size * buf->capacity);
	}
	buf->length++;
}

void vector_destroy(ObjectBuf *buf)
{
	SDL_free(buf);
}
