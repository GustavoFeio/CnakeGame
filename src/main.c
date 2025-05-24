
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#define UNUSED(x) ((void)(x))

#define defer_exit(n) \
	do { \
		defer_ret_value = (n); \
		goto defer; \
	} while (0)

#define log_error(...) SDL_LogError(SDL_LOG_CATEGORY_ERROR, __VA_ARGS__)

// TODO: Use Ticks Per Second instead of FPS to update the game
#define FPS 10
#define FRAME_DELAY (1000 / FPS)

#define GRID_WIDTH 40
#define GRID_HEIGHT 30
#define UNIT 30

#define SNAKE_SIZE UNIT
#define APPLE_SIZE UNIT

// Pre: initial snake length must be >= 3
#define SNAKE_INIT_LENGTH 5

#define WINDOW_WIDTH (UNIT*GRID_WIDTH)
#define WINDOW_HEIGHT (UNIT*GRID_HEIGHT)

typedef struct {
	int x, y;
} Vec2;

typedef struct SnakeBody {
	struct SnakeBody *next;
	int x, y;
} SnakeBody;

typedef struct {
	SnakeBody *tail;
	SnakeBody *head;
	size_t size;
	int dx, dy;
	bool dir_changed;
} Snake;

typedef Vec2 Apple;

typedef struct {
	Snake *snake;
	Apple apple;
} Scene;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	Scene *scene;
	bool running;
	bool gameover;
	bool quit;
} Game;

static const char *const WINDOW_TITLE = "Cnake Game";

SnakeBody *new_snake_body(int x, int y)
{
	SnakeBody *b = calloc(1, sizeof(*b));
	b->x = x;
	b->y = y;
	return b;
}

Snake *new_snake()
{
	Snake *snake = calloc(1, sizeof(*snake));
	snake->dx = 1;
	snake->dy = 0;
	snake->dir_changed = false;
	snake->size = SNAKE_INIT_LENGTH;

	// We update the game once before rendering, so we start at -1
	SnakeBody *b = new_snake_body(-1, 0);
	snake->tail = b;
	for (size_t i = 1; i < snake->size; i++) {
		SnakeBody *next = new_snake_body(i-1, 0);
		b->next = next;
		b = next;
	}
	snake->head = b;
	return snake;
}

bool intersects_snake(Snake *snake, Vec2 rect)
{
	SnakeBody *cur = snake->tail;
	while (cur != NULL) {
		if (cur->x == rect.x && cur->y == rect.y) return true;
		cur = cur->next;
	}
	return false;
}

Apple new_apple(Scene *scene)
{
	Apple apple;
	do {
		apple.x = (rand() / (double) RAND_MAX) * (GRID_WIDTH - 1);
		apple.y = (rand() / (double) RAND_MAX) * (GRID_HEIGHT - 1);
	} while (intersects_snake(scene->snake, apple));
	return apple;
}

void init_scene(Game *game)
{
	game->scene = calloc(1, sizeof(*game->scene));
	game->scene->snake = new_snake();
	game->scene->apple = new_apple(game->scene);
}

bool init_game(Game *game)
{
	srand(time(NULL));
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
	if (SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0") == SDL_FALSE) return false;

	game->window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if (game->window == NULL) return false;

	game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
	if (game->renderer == NULL) return false;

	init_scene(game);

	game->running = true;
	game->gameover = false;
	game->quit = false;

	return true;
}

void update_scene(Scene *scene)
{
	Snake *snake = scene->snake;
	SnakeBody *tail = snake->tail;
	SnakeBody *head = snake->head;
	Vec2 old_tail = {
		.x = tail->x,
		.y = tail->y,
	};
	Vec2 old_head = {
		.x = head->x,
		.y = head->y,
	};

	snake->tail = tail->next;
	tail->next = NULL;
	head->next = tail;
	snake->head = tail;
	head = tail;

	head->x = old_head.x + snake->dx;
	head->y = old_head.y + snake->dy;

	if (head->x == scene->apple.x && head->y == scene->apple.y) {
		SnakeBody *new_tail = new_snake_body(old_tail.x, old_tail.y);
		new_tail->next = snake->tail;
		snake->tail = new_tail;
		snake->size += 1;
		scene->apple = new_apple(scene);
	}
	snake->dir_changed = false;
}

bool out_of_bounds(Snake *snake)
{
	Vec2 head_pos = {
		.x = snake->head->x,
		.y = snake->head->y,
	};
	return head_pos.x >= GRID_WIDTH || head_pos.x < 0 || head_pos.y >= GRID_HEIGHT || head_pos.y < 0;
}

bool ouroboros(Snake *snake)
{
	int head_x = snake->head->x;
	int head_y = snake->head->y;
	SnakeBody *cur = snake->tail;
	while (cur != snake->head) {
		if (cur->x == head_x && cur->y == head_y) return true;
		cur = cur->next;
	}
	return false;
}

void check_game_over(Game *game)
{
	Snake *snake = game->scene->snake;
	if (ouroboros(snake) || out_of_bounds(snake)) {
		game->gameover = true;
	}
}

void update_game(Game *game)
{
	if (game->running && !game->gameover) {
		// TODO: if the game is running we update, else we present a pause screen
		update_scene(game->scene);
		check_game_over(game);
	}
	// TODO: if the player lost the game present a game over screen
}

void render_snake(Game *game)
{
	Snake *snake = game->scene->snake;
	SnakeBody *cur = snake->tail;
	SnakeBody *next = cur->next;
	SnakeBody *previous;
	SDL_Rect rect = {
		.x = cur->x * UNIT,
		.y = cur->y * UNIT,
		.w = SNAKE_SIZE,
		.h = SNAKE_SIZE,
	};

	SDL_SetRenderDrawColor(game->renderer, 0x18, 0xFF, 0x18, 0xFF);
	if (cur->x == next->x) {
		rect.w -= 2;
		rect.x += 1;
	} else {
		rect.h -= 2;
		rect.y += 1;
	}
	SDL_RenderFillRect(game->renderer, &rect);

	do {
		previous = cur;
		cur = next;
		next = cur->next;

		rect = (SDL_Rect) {
			.x = cur->x*UNIT,
			.y = cur->y*UNIT,
			.w = SNAKE_SIZE,
			.h = SNAKE_SIZE,
		};

		SDL_SetRenderDrawColor(game->renderer, 0x18, 0xFF, 0x18, 0xFF);
		if (cur->x == next->x) { // same column
			if (previous->x == cur->x) { // straight line
				rect.w -= 2;
				rect.x += 1;
			} else { // bending
				rect.w -= 1;
				rect.h -= 1;
				if (cur->y < next->y) // going down
					rect.y += 1;
				if (previous->x > cur->x) // from the right
					rect.x += 1;
			}
		} else { // same row
			if (previous->y == cur->y) { // straight line
				rect.h -= 2;
				rect.y += 1;
			} else { // bending
				rect.w -= 1;
				rect.h -= 1;
				if (cur->x < next->x) // going right
					rect.x += 1;
				if (previous->y > cur->y) // from the bottom
					rect.y += 1;
			}
		}
		SDL_RenderFillRect(game->renderer, &rect);
	} while (next != snake->head);

	previous = cur;
	cur = next;

	rect = (SDL_Rect) {
		.x = snake->head->x*UNIT,
		.y = snake->head->y*UNIT,
		.w = SNAKE_SIZE - 1,
		.h = SNAKE_SIZE - 1,
	};
	if (previous->x == cur->x) { // same column
		rect.w -= 1;
		rect.x += 1;
		if (previous->y > cur->y) rect.y += 1;
	} else { // same row
		rect.h -= 1;
		rect.y += 1;
		if (previous->x > cur->x) rect.x += 1;
	}
	SDL_SetRenderDrawColor(game->renderer, 0x90, 0xFF, 0x90, 0xFF);
	SDL_RenderFillRect(game->renderer, &rect);
}

void render_apple(Game *game)
{
	SDL_SetRenderDrawColor(game->renderer, 0xFF, 0x18, 0x18, 0xFF);
	SDL_Rect rect = {
		.x = game->scene->apple.x * UNIT,
		.y = game->scene->apple.y * UNIT,
		.w = APPLE_SIZE,
		.h = APPLE_SIZE,
	};
	SDL_RenderFillRect(game->renderer, &rect);
}

void render(Game *game)
{
	SDL_SetRenderDrawColor(game->renderer, 0x18, 0x18, 0x18, 0xFF);
	SDL_RenderClear(game->renderer);
	render_snake(game);
	render_apple(game);
	SDL_RenderPresent(game->renderer);
}

void free_snake(Snake *snake)
{
	SnakeBody *cur = snake->tail;
	if (cur != NULL) {
		SnakeBody *next = cur->next;
		while (next != NULL) {
			free(cur);
			cur = next;
			next = cur->next;
		}
		free(cur);
	}
	free(snake);
}

void free_scene(Scene *scene)
{
	free_snake(scene->snake);
	free(scene);
}

void free_game(Game *game)
{
	free_scene(game->scene);
}

void reset_game_state(Game *game)
{
	free_snake(game->scene->snake);
	game->scene->snake = new_snake();
	game->scene->apple = new_apple(game->scene);
	game->gameover = false;
	game->running = true;
}

void handle_keyboard_events(SDL_KeyboardEvent e, Game *game)
{
	switch (e.keysym.sym) {
	case SDLK_UP:
	case SDLK_w:
		if (!game->scene->snake->dir_changed && game->scene->snake->dy == 0) {
			game->scene->snake->dy = -1;
			game->scene->snake->dx = 0;
			game->scene->snake->dir_changed = true;
		}
		break;
	case SDLK_DOWN:
	case SDLK_s:
		if (!game->scene->snake->dir_changed && game->scene->snake->dy == 0) {
			game->scene->snake->dy = 1;
			game->scene->snake->dx = 0;
			game->scene->snake->dir_changed = true;
		}
		break;
	case SDLK_LEFT:
	case SDLK_a:
		if (!game->scene->snake->dir_changed && game->scene->snake->dx == 0) {
			game->scene->snake->dx = -1;
			game->scene->snake->dy = 0;
			game->scene->snake->dir_changed = true;
		}
		break;
	case SDLK_RIGHT:
	case SDLK_d:
		if (!game->scene->snake->dir_changed && game->scene->snake->dx == 0) {
			game->scene->snake->dx = 1;
			game->scene->snake->dy = 0;
			game->scene->snake->dir_changed = true;
		}
		break;
	case SDLK_SPACE:
		if (game->gameover) {
			reset_game_state(game);
		} else {
			game->running = !game->running;
		}
	}
}

void handle_events(Game *game)
{
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			game->quit = true;
			break;
		case SDL_KEYDOWN:
			handle_keyboard_events(e.key, game);
			break;
		}
	}
}

int main(void)
{
	int defer_ret_value = 0;

	Game game = {0};
	if (!init_game(&game)) {
		log_error("Could not initialize game: %s\n", SDL_GetError());
		defer_exit(1);
	}

	uint32_t frame_start;
	do {
		frame_start = SDL_GetTicks();

		handle_events(&game);

		update_game(&game);

		render(&game);

		int elapsed = FRAME_DELAY - (SDL_GetTicks() - frame_start);
		if (elapsed > 0) {
			SDL_Delay(elapsed);
		}
	} while (!game.quit);

	SDL_DestroyRenderer(game.renderer);
	SDL_DestroyWindow(game.window);
 
defer:
	free_game(&game);
	SDL_Quit();
	return defer_ret_value;
}

