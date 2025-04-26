# Rendering
- use SDL3 Texture Streaming
- lock texture -> writeonly pixel buffer
- fill with std::fill() and use -O2 or -O3 at release for auto simd
  - this is 10x faster than just filling the array with a loop 
- unlock texture and handle with high performant sdl3 OS backend
- for 4k resolution
  - CPU pixel-fill + SDL_LockTexture	~3.5ms
  - SDL_RenderTexture (GPU upload)	~0.5
  - SDL_RenderPresent() (no V-Sync)	0.1 – 0.5 ms
