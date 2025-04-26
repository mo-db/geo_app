# Rendering
- use SDL3 Texture Streaming
- lock texture -> writeonly pixel buffer
- fill with std::fill() and use -O2 or -O3 at release for auto simd
  - this is 10x faster than just filling the array with a loop 
- unlock texture and handle with high performant sdl3 OS backend
