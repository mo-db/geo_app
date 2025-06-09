// draw.hpp
#pragma once
#include "core.hpp"
#include "app.hpp"
#include "graphics.hpp"
#include "shapes.hpp"

namespace draw {
constexpr uint32_t black =				0x00000000;
constexpr uint32_t white =				0x00FFFFFF;
constexpr uint32_t dark_grey =		0x005C5C5C;
constexpr uint32_t light_grey =		0x00C5C5C5;

constexpr uint32_t red =					0x00FF0000;
constexpr uint32_t green =				0x0000FF00;
constexpr uint32_t blue =					0x000000FF;

constexpr uint32_t yellow =				0x00FFFF00;
constexpr uint32_t magenta =			0x00FF00FF;
constexpr uint32_t cyan =					0x0000FFFF;

constexpr uint32_t fg_color = white;
constexpr uint32_t bg_color = black;
constexpr uint32_t conceal_color = dark_grey;
constexpr uint32_t select_color = red;

constexpr uint32_t hl_primary_color = green;
constexpr uint32_t hl_secondary_color = cyan;
constexpr uint32_t hl_tertiary_color = magenta;

constexpr uint32_t special_color = blue;

namespace detail {
void set_pixel(App &app, uint32_t *pixel_buf, int x, int y, uint32_t color);
void plot_line(App &app, uint32_t *pixel_buf, const Line2 &line, uint32_t color);
void plot_circle(App &app, uint32_t *pixel_buf, const Circle2 &circle, uint32_t color);
void plot_arc(App &app, uint32_t *pixel_buf, const Arc2 &arc, uint32_t color);
} // namespace detail
void plot_shapes(App &app, Shapes &shapes);
uint32_t get_color(const Shapes &shapes);
} // namespace draw
