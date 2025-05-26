
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

// #define SHOW_FPS

#define UNUSED(x) ((void)(x))

#define defer_exit(n) \
	do { \
		defer_ret_value = (n); \
		goto defer; \
	} while (0)

#define FPS 60

#define TPS 10
#define TICK_DELAY (1.0 / TPS)

#define GRID_WIDTH 30
#define GRID_HEIGHT 20
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
	Scene *scene;
	size_t score;
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

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	SetTargetFPS(FPS);
	SetExitKey(KEY_NULL);

	init_scene(game);

	game->running = true;
	game->gameover = false;
	game->quit = false;

	return true;
}

void update_scene(Game *game)
{
	Scene *scene = game->scene;
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
		game->score += 1;
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
		update_scene(game);
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
	Rectangle rect = {
		.x = cur->x * UNIT,
		.y = cur->y * UNIT,
		.width = SNAKE_SIZE,
		.height = SNAKE_SIZE,
	};

	Color body_color = GetColor(0x18FF18FF);
	if (cur->x == next->x) {
		rect.width -= 2;
		rect.x += 1;
	} else {
		rect.height -= 2;
		rect.y += 1;
	}
	DrawRectangleRec(rect, body_color);

	do {
		previous = cur;
		cur = next;
		next = cur->next;

		rect = (Rectangle) {
			.x = cur->x*UNIT,
			.y = cur->y*UNIT,
			.width = SNAKE_SIZE,
			.height = SNAKE_SIZE,
		};

		if (cur->x == next->x) { // same column
			if (previous->x == cur->x) { // straight line
				rect.width -= 2;
				rect.x += 1;
			} else { // bending
				rect.width -= 1;
				rect.height -= 1;
				if (cur->y < next->y) // going down
					rect.y += 1;
				if (previous->x > cur->x) // from the right
					rect.x += 1;
			}
		} else { // same row
			if (previous->y == cur->y) { // straight line
				rect.height -= 2;
				rect.y += 1;
			} else { // bending
				rect.width -= 1;
				rect.height -= 1;
				if (cur->x < next->x) // going right
					rect.x += 1;
				if (previous->y > cur->y) // from the bottom
					rect.y += 1;
			}
		}
		DrawRectangleRec(rect, body_color);
	} while (next != snake->head);

	previous = cur;
	cur = next;

	rect = (Rectangle) {
		.x = snake->head->x*UNIT,
		.y = snake->head->y*UNIT,
		.width = SNAKE_SIZE - 1,
		.height = SNAKE_SIZE - 1,
	};
	if (previous->x == cur->x) { // same column
		rect.width -= 1;
		rect.x += 1;
		if (previous->y > cur->y) rect.y += 1;
	} else { // same row
		rect.height -= 1;
		rect.y += 1;
		if (previous->x > cur->x) rect.x += 1;
	}
	Color head_color = GetColor(0x90FF90FF);
	DrawRectangleRec(rect, head_color);
}

void render_apple(Game *game)
{
	Color apple_color = GetColor(0xFF1818FF);
	Rectangle rect = {
		.x = game->scene->apple.x * UNIT,
		.y = game->scene->apple.y * UNIT,
		.width = APPLE_SIZE,
		.height = APPLE_SIZE,
	};
	DrawRectangleRec(rect, apple_color);
}

void render_ui(Game *game)
{
#ifdef SHOW_FPS
	DrawFPS(10, 10 + UNIT + 10);
#endif
	char buf[32] = {0};
	sprintf(buf, "%ld", game->score);
	DrawText(buf, 10, 10, UNIT, RED);
}

void render(Game *game)
{
	BeginDrawing();
		ClearBackground(GetColor(0x181818FF));
		render_snake(game);
		render_apple(game);
		render_ui(game);
	EndDrawing();
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
	game->score = 0;
	game->gameover = false;
	game->running = true;
}

void handle_keyboard_events(Game *game)
{
	KeyboardKey key;
	while ((key = GetKeyPressed()) != 0) {
		switch (key) {
		case KEY_UP:
		case KEY_W:
			if (game->running && !game->scene->snake->dir_changed && game->scene->snake->dy == 0) {
				game->scene->snake->dy = -1;
				game->scene->snake->dx = 0;
				game->scene->snake->dir_changed = true;
			}
			break;
		case KEY_DOWN:
		case KEY_S:
			if (game->running && !game->scene->snake->dir_changed && game->scene->snake->dy == 0) {
				game->scene->snake->dy = 1;
				game->scene->snake->dx = 0;
				game->scene->snake->dir_changed = true;
			}
			break;
		case KEY_LEFT:
		case KEY_A:
			if (game->running && !game->scene->snake->dir_changed && game->scene->snake->dx == 0) {
				game->scene->snake->dx = -1;
				game->scene->snake->dy = 0;
				game->scene->snake->dir_changed = true;
			}
			break;
		case KEY_RIGHT:
		case KEY_D:
			if (game->running && !game->scene->snake->dir_changed && game->scene->snake->dx == 0) {
				game->scene->snake->dx = 1;
				game->scene->snake->dy = 0;
				game->scene->snake->dir_changed = true;
			}
			break;
		case KEY_SPACE:
			if (game->gameover) {
				reset_game_state(game);
			} else {
				game->running = !game->running;
			}
			break;
		default: {}
		}
	}
}

void handle_events(Game *game)
{
	if (WindowShouldClose()) {
		game->quit = true;
	} else {
		handle_keyboard_events(game);
	}
}

int main(void)
{
	int defer_ret_value = 0;

	Game game = {0};
	if (!init_game(&game)) {
		fprintf(stderr, "Could not initialize game.\n");
		defer_exit(1);
	}

	double previous_time = GetTime();
	do {
		double current_time = GetTime();

		handle_events(&game);
		if (current_time - previous_time >= TICK_DELAY) {

			update_game(&game);

			previous_time = current_time;
		}

		render(&game);

	} while (!game.quit);

	CloseWindow();
 
defer:
	free_game(&game);
	return defer_ret_value;
}

