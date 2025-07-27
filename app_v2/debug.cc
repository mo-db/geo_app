#include "debug.hpp"

namespace debug {
void info_print(App &app, Shapes &shapes) {
  std::cout << "Mouse at (" << app.input.mouse.x << "," << app.input.mouse.y
            << ")\n";
  std::cout << "node snap: " << shapes.snap.enabled_for_node_shapes
            << std::endl;

  if (shapes.lines.size() > 0) {
    auto &line = shapes.lines[0];
    Vec2 pp = line2::project_point(line.geom, app.input.mouse);
    std::cout << "projected point at: " << pp.x << "," << pp.y << std::endl;
    double seg_dist =
        line2::get_distance_point_to_seg(line.geom, app.input.mouse);
    double ray_dist =
        line2::get_distance_point_to_ray(line.geom, app.input.mouse);
    std::cout << "seg_dist: " << seg_dist << std::endl;
    std::cout << "ray_dist: " << ray_dist << std::endl;
  }
  std::cout << "Snap ID: " << shapes.snap.id << std::endl;
}
} // namespace debug
