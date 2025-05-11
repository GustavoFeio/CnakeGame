
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define defer_exit(n) \
	do { \
		defer_ret_value = (n); \
		goto defer; \
	} while (0)

#define log_error(...) SDL_LogError(SDL_LOG_CATEGORY_ERROR, __VA_ARGS__)

#define FPS 30
#define FRAME_DELAY (1000 / FPS)

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define RECT_SPEED 10

typedef struct {
	SDL_Rect sdl_rect;
	int dx, dy;
} Rect;

void update_rect(Rect *rect)
{
	int nx = rect->sdl_rect.x + RECT_SPEED*rect->dx;
	if (nx < 0 || nx + rect->sdl_rect.w >= WINDOW_WIDTH) {
		rect->dx *= -1;
		nx = rect->sdl_rect.x + RECT_SPEED*rect->dx;
	}
	rect->sdl_rect.x = nx;

	int ny = rect->sdl_rect.y + RECT_SPEED*rect->dy;
	if (ny < 0 || ny + rect->sdl_rect.h >= WINDOW_HEIGHT) {
		rect->dy *= -1;
		ny = rect->sdl_rect.y + RECT_SPEED*rect->dy;
	}
	rect->sdl_rect.y = ny;
}

int main(void)
{
	int defer_ret_value = 0;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		log_error("Could not initialize SDL: %s", SDL_GetError());
		defer_exit(1);
	}

	if (SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0") == SDL_FALSE) {
		log_error("Could not set hint: %s\n", SDL_GetError());
		defer_exit(1);
	}

	const char *title = "Hello, from SDL!";
	SDL_Window *window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		log_error("Could not create window: %s\n", SDL_GetError());
		defer_exit(1);
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		log_error("Could not create renderer: %s", SDL_GetError());
		defer_exit(1);
	}

	Rect rect = {
		(SDL_Rect) {
			.x = 0,
			.y = 0,
			.w = 50,
			.h = 50,
		},
		.dx = 1,
		.dy = 1,
	};

	uint32_t frame_start;
	bool quit = false;
	do {
		frame_start = SDL_GetTicks();

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				quit = true;
				break;
			default:
			}
		}

		update_rect(&rect);

		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 0xFF, 0x0, 0x0, 0xFF);
		SDL_RenderFillRect(renderer, &rect.sdl_rect);
		SDL_RenderPresent(renderer);

		int elapsed = FRAME_DELAY - (SDL_GetTicks() - frame_start);
		if (elapsed > 0) {
			SDL_Delay(elapsed);
		}
	} while (!quit);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
 
defer:
	SDL_Quit();
	return defer_ret_value;
}

