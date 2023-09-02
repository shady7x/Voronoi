#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <atomic>
#include <stdexcept>

class Window {
    private:
        std::atomic<uint64_t> size;
    public:
        SDL_Window* ptr;

        Window(int w = 1600, int h = 900) : size(static_cast<uint64_t>(w) << 32 | h), ptr(nullptr) {
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                throw std::runtime_error(SDL_GetError());
            }
            ptr = SDL_CreateWindow("Диплом. Комаров", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
            if (ptr == nullptr) {
                throw std::runtime_error(SDL_GetError());
            }
        }

        ~Window() {
            if (ptr != nullptr) {
                SDL_DestroyWindow(ptr);
            }
        }

        uint64_t getSize() {
            return size;
        }
};