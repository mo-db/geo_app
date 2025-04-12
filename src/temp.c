
		/*   if (pix_cnt == (WINDOW_WIDTH_PIXELS * WINDOW_HEIGHT_PIXELS)) { */
		/*     pix_cnt = 0; */
		/*   } */
		/* pix_cnt+=50; */
		/* if (pix_cnt%10 == 0) { */
		/* 	pixs[pix_cnt] = 0xFF0000FF; */
		/* } */

		int ix = appstate->mouse_x;
		int iy = appstate->mouse_y;
		/* printf("mouse: %d, %d\n", ix, iy); */
		for (int x = ix; x < ix+5; x++) {
			if (x <= WINDOW_WIDTH_PIXELS) {
				pixs[x+WINDOW_WIDTH_PIXELS*iy] = 0x00FF00FF;
			}
			if (x-5 >= 0) {
				pixs[x-5+WINDOW_WIDTH_PIXELS*iy] = 0x00FF00FF;
			}
		}
		for (int y = iy; y < iy+5; y++) {
			if (y <= WINDOW_HEIGHT) {
				pixs[ix+WINDOW_WIDTH_PIXELS*y] = 0x00FF00FF;
			}
			if (y-5 >= 0) {
				pixs[ix+WINDOW_WIDTH_PIXELS*(y-5)] = 0x00FF00FF;
			}
		}

    for (int l2d_cnt = 0; l2d_cnt < objects->l2d_index; l2d_cnt++) {
      int x0 = SDL_lround(objects->l2d->p1.x);
      int y0 = SDL_lround(objects->l2d->p1.y);
      int x1 = SDL_lround(objects->l2d->p2.x);
      int y1 = SDL_lround(objects->l2d->p2.y);
      // midpoint line
      int dx = x1 - x0;
      int dy = y1 - y0;
      int d = 2 * dy - dx; // initial decision variable
      int incrE = 2 * dy;
      int incrNE = 2 * (dy - dx);
      int x = x0;
      int y = y0;
#ifdef DEBUG
      printf("l2d_index: %d\n", objects->l2d_index);
      printf("(x0,y0),(x1,y1): (%d,%d),(%d,%d)\n", x0, y0, x1, y1);
      printf("dx, dy, d: %d, %d, %d\n", dx, dy, d);
#endif
      pixs[x + y * WINDOW_WIDTH] = 0x00FF00FF;
      while (x < x1) {
        if (d <= 0) { // works if d is big number, just check sign
          d += incrE;
          x++;
        } else {
          d += incrNE;
          x++;
          y++;
        }
        pixs[x + y * WINDOW_WIDTH] = 0x00FF00FF;
      }
    }

