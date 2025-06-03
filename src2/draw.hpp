// draw.hpp
#pragma once
#include "core.hpp"
#include "app.hpp"
#include "graphics.hpp"
#include "shapes.hpp"

namespace draw {
constexpr uint32_t fg_color = 0x00000000;					//black
constexpr uint32_t bg_color = 0x00FFFFFF;					//white
constexpr uint32_t sel_color = 0x00FF0000;				//red
constexpr uint32_t conceal_color = 0x00CCCCCC;		//grey
constexpr uint32_t gen_color = 0x000000FF;				//blue
constexpr uint32_t edit_color = 0x0000FF00;				//green
constexpr uint32_t ref_color = 0x000000FF;				//blue
																									//
uint32_t get_color(const Shapes &shapes);
void draw_line(App &app, uint32_t *pixel_buf, const Line2 &line, uint32_t color);
void draw_circle(App &app, uint32_t *pixel_buf, const Circle2 &circle, uint32_t color);
void draw_arc(App &app, uint32_t *pixel_buf, const Arc2 &arc, uint32_t color);
void draw_shapes(App &app, Shapes &shapes);
} // namespace draw
