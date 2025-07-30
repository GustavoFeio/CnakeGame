
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

// #define CNAKE_DEBUG

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

#define BACKGROUND_COLOR 0x181818FF
#define APPLE_COLOR 0xFF1818FF

void *cnake_realloc(void *items, size_t bytes)
{
#ifdef CNAKE_DEBUG
	printf("DEBUG: Reallocated %zu bytes\n", bytes);
#endif
	return realloc(items, bytes);
}

#define DA_INIT_CAPACITY 16
#define DA_GROWTH_FACTOR 1.5

#define da_append(xs, x) \
	do { \
		if ((xs)->count >= (xs)->capacity) { \
			(xs)->capacity = (xs)->capacity == 0 ? DA_INIT_CAPACITY : (xs)->capacity * DA_GROWTH_FACTOR; \
			(xs)->items = cnake_realloc((xs)->items, (xs)->capacity * sizeof((xs)->items[0])); \
		} \
		(xs)->items[(xs)->count++] = (x); \
	} while (0)

typedef struct {
	int x, y;
} Vec2;

typedef struct {
	int x, y;
	size_t next;
} SnakeBody;

typedef struct {
	// Dynamic array
	SnakeBody *items;
	size_t count;
	size_t capacity;

	size_t tail; // tail index into items
	size_t head; // head index into items
	int dx, dy;
	bool dir_changed;
} Snake;

typedef Vec2 Apple;

typedef struct {
	Snake snake;
	Apple apple;
} Scene;

typedef enum {
	MENU,
	PLAYING,
	PAUSED,
	GAMEOVER,
} GameState;

typedef struct {
	Scene scene;
	size_t score;
	GameState state;
	bool quit;
} Game;

void render_snake(Game *game)
{
	Snake *snake = &game->scene.snake;
	SnakeBody cur = snake->items[snake->tail];
	SnakeBody next = snake->items[cur.next];
	SnakeBody previous;
	Rectangle rect = {
		.x = cur.x * UNIT,
		.y = cur.y * UNIT,
		.width  = SNAKE_SIZE,
		.height = SNAKE_SIZE,
	};

	Color body_color = GetColor(0x18FF18FF);
	if (cur.x == next.x) {
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
		next = snake->items[cur.next];

		rect = (Rectangle) {
			.x = cur.x*UNIT,
			.y = cur.y*UNIT,
			.width  = SNAKE_SIZE,
			.height = SNAKE_SIZE,
		};

		if (cur.x == next.x) { // same column
			if (previous.x == cur.x) { // straight line
				rect.width -= 2;
				rect.x += 1;
			} else { // bending
				rect.width -= 1;
				rect.height -= 1;
				if (cur.y < next.y) // going down
					rect.y += 1;
				if (previous.x > cur.x) // from the right
					rect.x += 1;
			}
		} else { // same row
			if (previous.y == cur.y) { // straight line
				rect.height -= 2;
				rect.y += 1;
			} else { // bending
				rect.width -= 1;
				rect.height -= 1;
				if (cur.x < next.x) // going right
					rect.x += 1;
				if (previous.y > cur.y) // from the bottom
					rect.y += 1;
			}
		}
		DrawRectangleRec(rect, body_color);
	} while (cur.next != snake->head);

	previous = cur;
	cur = next;

	rect = (Rectangle) {
		.x = snake->items[snake->head].x * UNIT,
		.y = snake->items[snake->head].y * UNIT,
		.width  = SNAKE_SIZE - 1,
		.height = SNAKE_SIZE - 1,
	};
	if (previous.x == cur.x) { // same column
		rect.width -= 1;
		rect.x += 1;
		if (previous.y > cur.y) rect.y += 1;
	} else { // same row
		rect.height -= 1;
		rect.y += 1;
		if (previous.x > cur.x) rect.x += 1;
	}
	Color head_color = GetColor(0x90FF90FF);
	DrawRectangleRec(rect, head_color);
}

void render_apple(Game *game)
{
	Apple apple = game->scene.apple;
	Color apple_color = GetColor(APPLE_COLOR);
	DrawCircle((apple.x + 0.5) * UNIT, (apple.y + 0.5) * UNIT, APPLE_SIZE / 2, apple_color);
}

void render_score(Game *game)
{
#ifdef CNAKE_DEBUG
	DrawFPS(10, 10 + UNIT + 10);
#endif
	char buf[32] = {0};
	sprintf(buf, "%ld", game->score);
	DrawText(buf, 10, 10, UNIT*1.5, RED);
}

void render_pause_screen(Game *game)
{
	UNUSED(game);

	Color darker_bg = ColorAlpha(BLACK, 0.3f);
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, darker_bg);

	int pause_size = UNIT*2;
	const char *pause = "Game Paused";
	int pause_width = MeasureText(pause, pause_size);
	DrawText(pause, (WINDOW_WIDTH - pause_width)/2, WINDOW_HEIGHT/3, pause_size, WHITE);

	int msg_size = UNIT*1.5;
	const char *msg = "Press `Esc` to unpause";
	int msg_width = MeasureText(msg, msg_size);
	DrawText(msg, (WINDOW_WIDTH - msg_width)/2, WINDOW_HEIGHT/3*2, msg_size, WHITE);
}

void render_gameover_screen(Game *game)
{
	UNUSED(game);

	Color darker_bg = ColorAlpha(BLACK, 0.15f);
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, darker_bg);

	int gameover_size = UNIT*2;
	const char *gameover = "GAME OVER";
	int gameover_width = MeasureText(gameover, gameover_size);
	DrawText(gameover, (WINDOW_WIDTH - gameover_width)/2, WINDOW_HEIGHT/3, gameover_size, RED);

	int msg_size = UNIT*1.5;
	const char *msg = "Press `Space` to restart";
	int msg_width = MeasureText(msg, msg_size);
	DrawText(msg, (WINDOW_WIDTH - msg_width)/2, WINDOW_HEIGHT/3*2, msg_size, RED);
}

typedef void (*RenderFunc)(Game *);

#define RENDER_COLUMNS 5
static const RenderFunc renderer[][RENDER_COLUMNS] = {
	[PLAYING] = {&render_snake, &render_apple, &render_score, NULL},
	[PAUSED]  = {&render_snake, &render_apple, &render_score, &render_pause_screen, NULL},
	[GAMEOVER]  = {&render_snake, &render_apple, &render_score, &render_gameover_screen, NULL},
};

static const char *const WINDOW_TITLE = "Cnake Game";

void init_snake(Snake *snake)
{
	snake->dx = 1;
	snake->dy = 0;
	snake->dir_changed = false;
	snake->count = 0;

	// We update the game once before rendering, so we start at -1
	for (size_t i = 0; i < SNAKE_INIT_LENGTH; i++) {
		da_append(snake, ((SnakeBody) { i-1, 0, i+1 }));
	}
	snake->tail = 0;
	snake->head = snake->count - 1;
}

Snake new_snake()
{
	Snake snake = {0};
	init_snake(&snake);
	return snake;
}

bool intersects_snake(Snake *snake, Vec2 rect)
{
	for (size_t i = 0; i < snake->count; i++) {
		SnakeBody cur = snake->items[i];
		if (cur.x == rect.x && cur.y == rect.y) return true;
	}
	return false;
}

Apple new_apple(Scene *scene)
{
	Apple apple;
	do {
		apple.x = (rand() / (double) RAND_MAX) * (GRID_WIDTH - 1);
		apple.y = (rand() / (double) RAND_MAX) * (GRID_HEIGHT - 1);
	} while (intersects_snake(&scene->snake, apple));
	return apple;
}

void init_scene(Game *game)
{
	game->scene.snake = new_snake();
	game->scene.apple = new_apple(&game->scene);
}

bool init_game(Game *game)
{
	srand(time(NULL));

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	SetTargetFPS(FPS);
	SetExitKey(KEY_NULL);

	init_scene(game);

	game->score = 0;
	// TODO: menu
	game->state = PLAYING;
	game->quit = false;

	return true;
}

void update_scene(Game *game)
{
	Scene *scene = &game->scene;
	Snake *snake = &scene->snake;
	size_t tail = snake->tail;
	size_t head = snake->head;
	SnakeBody old_tail = snake->items[tail];
	SnakeBody old_head = snake->items[head];

	snake->tail = snake->items[tail].next;
	snake->items[head].next = tail;
	snake->head = tail;
	head = tail;

	snake->items[head].x = old_head.x + snake->dx;
	snake->items[head].y = old_head.y + snake->dy;

	if (snake->items[head].x == scene->apple.x && snake->items[head].y == scene->apple.y) {
		da_append(snake, ((SnakeBody) { old_tail.x, old_tail.y, snake->tail }));
		snake->tail = snake->count - 1;
		scene->apple = new_apple(scene);
		game->score += 1;
	}
	snake->dir_changed = false;
}

bool out_of_bounds(Snake *snake)
{
	SnakeBody head = snake->items[snake->head];
	return head.x >= GRID_WIDTH || head.x < 0 || head.y >= GRID_HEIGHT || head.y < 0;
}

bool ouroboros(Snake *snake)
{
	SnakeBody head = snake->items[snake->head];
	size_t cur = snake->tail;
	while (cur != snake->head) {
		SnakeBody b = snake->items[cur];
		if (b.x == head.x && b.y == head.y) return true;
		cur = b.next;
	}
	return false;
}

void check_game_over(Game *game)
{
	Snake *snake = &game->scene.snake;
	if (out_of_bounds(snake) || ouroboros(snake)) {
		game->state = GAMEOVER;
	}
}

void update_game(Game *game)
{
	if (game->state == PLAYING) {
		// TODO: if the game is running update, else present a pause screen
		update_scene(game);
		check_game_over(game);
	}
	// TODO: if the player lost the game present a game over screen
}

void render(Game *game)
{
	BeginDrawing();
		ClearBackground(GetColor(BACKGROUND_COLOR));
		const RenderFunc *pipeline = renderer[game->state];
		for (int i = 0; i < RENDER_COLUMNS; i++) {
			RenderFunc f = pipeline[i];
			if (f == NULL) break;
			f(game);
		}
	EndDrawing();
}

void free_snake(Snake *snake)
{
	free(snake->items);
}

void free_scene(Scene *scene)
{
	free_snake(&scene->snake);
}

void free_game(Game *game)
{
	free_scene(&game->scene);
}

void reset_game_state(Game *game)
{
	init_snake(&game->scene.snake);
	game->scene.apple = new_apple(&game->scene);
	game->score = 0;
	game->state = PLAYING;
}

void handle_keyboard_events(Game *game)
{
	Snake *snake = &game->scene.snake;
	KeyboardKey key;
	while ((key = GetKeyPressed()) != 0) {
		switch (key) {
		case KEY_UP:
		case KEY_W:
			if (game->state == PLAYING && !snake->dir_changed && snake->dy == 0) {
				snake->dy = -1;
				snake->dx = 0;
				snake->dir_changed = true;
			}
			break;
		case KEY_DOWN:
		case KEY_S:
			if (game->state == PLAYING && !snake->dir_changed && snake->dy == 0) {
				snake->dy = 1;
				snake->dx = 0;
				snake->dir_changed = true;
			}
			break;
		case KEY_LEFT:
		case KEY_A:
			if (game->state == PLAYING && !snake->dir_changed && snake->dx == 0) {
				snake->dx = -1;
				snake->dy = 0;
				snake->dir_changed = true;
			}
			break;
		case KEY_RIGHT:
		case KEY_D:
			if (game->state == PLAYING && !snake->dir_changed && snake->dx == 0) {
				snake->dx = 1;
				snake->dy = 0;
				snake->dir_changed = true;
			}
			break;
		case KEY_SPACE:
			if (game->state == GAMEOVER) {
				reset_game_state(game);
			}
			break;
		case KEY_ESCAPE:
			if (game->state == PAUSED) {
				game->state = PLAYING;
			} else if (game->state == PLAYING) {
				game->state = PAUSED;
			}
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
		handle_events(&game);

		double current_time = GetTime();
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

