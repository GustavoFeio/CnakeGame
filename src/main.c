
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

// TODO: add debug menu to adjust all debug related stuff in the game
// instead of defining macros and recompiling
#if 0
// #define CNAKE_DEBUG
// #define CNAKE_TRANSPARENT 0.5f
// #define CNAKE_GRID
// #define CNAKE_CALLS
#endif

#ifdef CNAKE_CALLS
size_t draws;
#endif // CNAKE_CALLS

#define UNUSED(x) ((void)(x))

#define defer_exit(n) \
	do { \
		defer_ret_value = (n); \
		goto defer; \
	} while (0)

#define FPS 60

#define TPS 13
#define TICK_DELAY (1.0f / (double)TPS)

#define GRID_WIDTH 30
#define GRID_HEIGHT 20
#define UNIT 30

#define SNAKE_SIZE UNIT
#define APPLE_SIZE UNIT

// Pre: initial snake length must be >= 3
#define SNAKE_INIT_LENGTH 5

#define WINDOW_WIDTH (UNIT*GRID_WIDTH)
#define WINDOW_HEIGHT (UNIT*GRID_HEIGHT)

#define GRID_COLOR 0x4444FFFF
#define BACKGROUND_COLOR 0x181818FF
#define SNAKE_BODY_COLOR 0x18FF18FF
#define SNAKE_HEAD_COLOR 0x90FF90FF
#define APPLE_COLOR 0xFF1818FF

static inline void *cnake_realloc(void *items, size_t bytes)
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

typedef enum {
	LEFT,
	RIGHT,
	UP,
	DOWN,
} Direction;

typedef struct {
	// Dynamic array
	SnakeBody *items;
	size_t count;
	size_t capacity;

	size_t tail; // tail index into items
	size_t head; // head index into items
	Direction dir;
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

static inline void cnake_DrawRectangleRec(Rectangle rec, Color color)
{
#ifdef CNAKE_CALLS
	draws++;
#endif // CNAKE_CALLS
	DrawRectangleRec(rec, color);
}

static inline void cnake_DrawCircle(int centerX, int centerY, float radius, Color color)
{
#ifdef CNAKE_CALLS
	draws++;
#endif // CNAKE_DEBUG
	DrawCircle(centerX, centerY, radius, color);
}

static inline void cnake_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color)
{
#ifdef CNAKE_CALLS
	draws++;
#endif // CNAKE_DEBUG
	DrawLine(startPosX, startPosY, endPosX, endPosY, color);
}

void render_snake(Game *game)
{
	Snake *snake = &game->scene.snake;
	SnakeBody cur = snake->items[snake->tail];
	SnakeBody next = cur;
	SnakeBody previous;
	Rectangle rect;
	Color body_color = GetColor(SNAKE_BODY_COLOR);
#ifdef CNAKE_TRANSPARENT
	body_color = ColorAlpha(body_color, CNAKE_TRANSPARENT);
#endif // CNAKE_DEBUG

	do {
		previous = cur;
		cur = next;
		next = snake->items[cur.next];

		if (previous.x != cur.x || previous.y != cur.y) {
			rect = (Rectangle) {
				.x = previous.x * UNIT + 1,
				.y = previous.y * UNIT + 1,
				.width  = SNAKE_SIZE - 2,
				.height = SNAKE_SIZE - 2,
			};
			if (previous.x < cur.x) {
				rect.width += SNAKE_SIZE;
			} else if (previous.x > cur.x) {
				rect.x -= SNAKE_SIZE;
				rect.width += SNAKE_SIZE;
			} else if (previous.y < cur.y) {
				rect.height += SNAKE_SIZE;
			} else {
				rect.y -= SNAKE_SIZE;
				rect.height += SNAKE_SIZE;
			}
#ifndef CNAKE_DEBUG
			cnake_DrawRectangleRec(rect, body_color);
#else
			cnake_DrawRectangleRec(rect, RED);
#endif // CNAKE_DEBUG
		}

		rect = (Rectangle) {
			.x = cur.x*UNIT + 1,
			.y = cur.y*UNIT + 1,
			.width  = SNAKE_SIZE - 2,
			.height = SNAKE_SIZE - 2,
		};
		if (cur.x == next.x) { // same column
			bool going_up = next.y < cur.y;
			size_t segment_height = 0;
			while (cur.x == next.x && cur.next != snake->head) {
				segment_height += SNAKE_SIZE;
				cur = next;
				next = snake->items[next.next];
			}
			rect.height += segment_height;
			if (going_up) rect.y -= segment_height;
		} else { // same row
			bool going_left = next.x < cur.x;
			size_t segment_width = 0;
			while (cur.y == next.y && cur.next != snake->head) {
				segment_width += SNAKE_SIZE;
				cur = next;
				next = snake->items[next.next];
			}
			rect.width += segment_width;
			if (going_left) rect.x -= segment_width;
		}
		cnake_DrawRectangleRec(rect, body_color);

	} while (cur.next != snake->head);

	previous = cur;
	cur = next;
	rect = (Rectangle) {
		.x = snake->items[snake->head].x * UNIT,
		.y = snake->items[snake->head].y * UNIT,
		.width  = SNAKE_SIZE,
		.height = SNAKE_SIZE,
	};
	if (previous.x == cur.x) { // same column
		rect.width -= 2;
		rect.x += 1;
		if (previous.y > cur.y) rect.y += 1;
		else rect.y -= 1;
	} else { // same row
		rect.height -= 2;
		rect.y += 1;
		if (previous.x > cur.x) rect.x += 1;
		else rect.x -= 1;
	}
	Color head_color = GetColor(SNAKE_HEAD_COLOR);
	cnake_DrawRectangleRec(rect, head_color);
	// float eye_white_radius = rect.width/4;
	// float eye_black_radius = rect.width/5;
	// cnake_DrawCircle(rect.x + rect.width/2 - rect.width/3, rect.y + eye_white_radius, eye_white_radius, WHITE);
	// cnake_DrawCircle(rect.x + rect.width/2 + rect.width/3, rect.y + eye_white_radius, eye_white_radius, WHITE);
	// cnake_DrawCircle(rect.x + rect.width/2 - rect.width/3, rect.y + eye_white_radius, eye_black_radius, BLACK);
	// cnake_DrawCircle(rect.x + rect.width/2 + rect.width/3, rect.y + eye_white_radius, eye_black_radius, BLACK);
}

void render_apple(Game *game)
{
	Apple apple = game->scene.apple;
	Color apple_color = GetColor(APPLE_COLOR);
	cnake_DrawCircle((apple.x + 0.5) * UNIT, (apple.y + 0.5) * UNIT, APPLE_SIZE / 2, apple_color);
}

void render_score(Game *game)
{
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
	DrawText(pause, (WINDOW_WIDTH - pause_width)/2, WINDOW_HEIGHT/3 - UNIT/2, pause_size, WHITE);

	int msg_size = UNIT*1.5;
	int msg_padding = msg_size * 0.75;
	// TODO: move all key inputs to macros and use GetKeyName to print instead of hard-coding
	// TODO: add option to go back to the menu, because the animation there is nice :)
	const char *msgs[] = {
		"Press `Space` to unpause",
		"Press `Esc` to exit",
	};
	for (size_t i = 0; i < sizeof(msgs)/sizeof(*msgs); i++) {
		int msg_width = MeasureText(msgs[i], msg_size);
		DrawText(msgs[i], (WINDOW_WIDTH - msg_width)/2, WINDOW_HEIGHT/3*2 + (UNIT + msg_padding)*i - UNIT/2, msg_size, WHITE);
	}
}

void render_gameover_screen(Game *game)
{
	UNUSED(game);

	Color darker_bg = ColorAlpha(BLACK, 0.15f);
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, darker_bg);

	int gameover_size = UNIT*2;
	const char *gameover = "GAME OVER";
	int gameover_width = MeasureText(gameover, gameover_size);
	DrawText(gameover, (WINDOW_WIDTH - gameover_width)/2, WINDOW_HEIGHT/3 - UNIT/2, gameover_size, RED);

	int msg_size = UNIT*1.5;
	int msg_padding = msg_size * 0.75;
	// TODO: move all key inputs to macros and use GetKeyName to print instead of hard-coding
	const char *msgs[] = {
		"Press `Space` to restart",
		"Press `Esc` to exit",
	};
	for (size_t i = 0; i < sizeof(msgs)/sizeof(*msgs); i++) {
		int msg_width = MeasureText(msgs[i], msg_size);
		DrawText(msgs[i], (WINDOW_WIDTH - msg_width)/2, WINDOW_HEIGHT/3*2 + (UNIT + msg_padding)*i - UNIT/2, msg_size, WHITE);
	}
}

void render_menu_screen(Game *game)
{
	UNUSED(game);

	Color darker_bg = ColorAlpha(BLACK, 0.15f);
	DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, darker_bg);

	int title_size = UNIT*2;
	const char *title = "CnakeGame";
	int title_width = MeasureText(title, title_size);
	DrawText(title, (WINDOW_WIDTH - title_width)/2, WINDOW_HEIGHT/3 - UNIT/2, title_size, WHITE);

	int msg_size = UNIT*1.5;
	int msg_padding = msg_size * 0.75;
	// TODO: move all key inputs to macros and use GetKeyName to print instead of hard-coding
	const char *msgs[] = {
		"Press `Space` to start",
		"Press `Esc` to exit",
	};
	for (size_t i = 0; i < sizeof(msgs)/sizeof(*msgs); i++) {
		int msg_width = MeasureText(msgs[i], msg_size);
		DrawText(msgs[i], (WINDOW_WIDTH - msg_width)/2, WINDOW_HEIGHT/3*2 + (UNIT + msg_padding)*i - UNIT/2, msg_size, WHITE);
	}
}

typedef void (*RenderFunc)(Game *);

#define RENDER_COLUMNS 5
static const RenderFunc renderer[][RENDER_COLUMNS] = {
	[MENU] = {&render_snake, &render_apple, &render_menu_screen, NULL},
	[PLAYING] = {&render_snake, &render_apple, &render_score, NULL},
	[PAUSED]  = {&render_snake, &render_apple, &render_score, &render_pause_screen, NULL},
	[GAMEOVER]  = {&render_snake, &render_apple, &render_score, &render_gameover_screen, NULL},
};

static const char *const WINDOW_TITLE = "Cnake Game";

void init_snake(Snake *snake)
{
	snake->dir = RIGHT;
	snake->dir_changed = false;
	snake->count = 0;

	for (size_t i = 0; i < SNAKE_INIT_LENGTH; i++) {
		da_append(snake, ((SnakeBody) { i, 0, i+1 }));
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

void prepare_menu(Game *game)
{
	Snake *snake = &game->scene.snake;
	SnakeBody *b = &snake->items[snake->tail];
	for (size_t i = 0; i < snake->count; i++) {
		b->x = (GRID_WIDTH/2) - snake->count + i;
		b->y = (GRID_HEIGHT/2) - snake->count;
		b = &snake->items[b->next];
	}

	Apple *apple = &game->scene.apple;
	apple->x = (GRID_WIDTH / 2);
	apple->y = (GRID_HEIGHT / 2);
}

void init_game(Game *game)
{
	srand(time(NULL));

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	SetTargetFPS(FPS);
	SetExitKey(KEY_NULL);

	init_scene(game);
	prepare_menu(game);

	game->score = 0;
	game->state = MENU;
	game->quit = false;
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

	int dx = snake->dir == LEFT ? -1 : snake->dir == RIGHT ? 1 : 0;
	int dy = snake->dir == UP ? -1 : snake->dir == DOWN ? 1 : 0;
	snake->items[head].x = old_head.x + dx;
	snake->items[head].y = old_head.y + dy;

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
	while (snake->items[cur].next != snake->head) { // Will break if the snake's size is < 2
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

void update_menu(Game *game)
{
	Snake *snake = &game->scene.snake;
	SnakeBody head = snake->items[snake->head];
	if (head.x == (GRID_WIDTH / 2) + (int)snake->count) {
		if (head.y == (GRID_HEIGHT / 2) - (int)snake->count)
			snake->dir = DOWN;
		else if (head.y == (GRID_HEIGHT / 2) + (int)snake->count)
			snake->dir = LEFT;
	} else if (head.x == (GRID_WIDTH / 2) - (int)snake->count) {
		if (head.y == (GRID_HEIGHT / 2) - (int)snake->count)
			snake->dir = RIGHT;
		else if (head.y == (GRID_HEIGHT / 2) + (int)snake->count)
			snake->dir = UP;
	}
}

void update_game(Game *game)
{
	if (game->state == PLAYING) {
		update_scene(game);
		check_game_over(game);
	} else if (game->state == MENU) {
		update_menu(game);
		update_scene(game);
	}
}

void render(Game *game)
{
#ifdef CNAKE_CALLS
	draws = 0;
#endif // CNAKE_DEBUG
	BeginDrawing();
		ClearBackground(GetColor(BACKGROUND_COLOR));
#ifdef CNAKE_GRID
		Color color = GetColor(GRID_COLOR);
		for (int i = 0; i < GRID_HEIGHT; i++) {
			DrawLine(0, i*UNIT, WINDOW_WIDTH, i*UNIT, color);
		}
		for (int i = 0; i < GRID_WIDTH; i++) {
			DrawLine(i*UNIT, 0, i*UNIT, WINDOW_HEIGHT, color);
		}
#endif // CNAKE_GRID
		const RenderFunc *pipeline = renderer[game->state];
		for (int i = 0; i < RENDER_COLUMNS; i++) {
			RenderFunc f = pipeline[i];
			if (f == NULL) break;
			f(game);
		}
#ifdef CNAKE_DEBUG
		DrawFPS(10, 10 + UNIT + 10);
#endif // CNAKE_DEBUG
	EndDrawing();
#ifdef CNAKE_CALLS
	printf("[DEBUG]: issued %ld draw calls\n", draws);
#endif // CNAKE_CALLS
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
	if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
		if (game->state == PLAYING && !snake->dir_changed && snake->dir != DOWN) {
			snake->dir = UP;
			snake->dir_changed = true;
		}
	}
	if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
		if (game->state == PLAYING && !snake->dir_changed && snake->dir != UP) {
			snake->dir = DOWN;
			snake->dir_changed = true;
		}
	}
	if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
		if (game->state == PLAYING && !snake->dir_changed && snake->dir != RIGHT) {
			snake->dir = LEFT;
			snake->dir_changed = true;
		}
	}
	if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
		if (game->state == PLAYING && !snake->dir_changed && snake->dir != LEFT) {
			snake->dir = RIGHT;
			snake->dir_changed = true;
		}
	}
	if (IsKeyPressed(KEY_SPACE)) {
		if (game->state == GAMEOVER) {
			reset_game_state(game);
		} else if (game->state == PLAYING) {
			game->state = PAUSED;
		} else if (game->state == PAUSED || game->state == MENU) {
			game->state = PLAYING;
		}
	}
	if (IsKeyPressed(KEY_ESCAPE)) {
		// TODO: add confirmation screen
		if (game->state == MENU
				|| game->state == PAUSED
				|| game->state == GAMEOVER) {
			game->quit = true;
		}
	}
}

void _handle_keyboard_events(Game *game)
{
	Snake *snake = &game->scene.snake;
	KeyboardKey key;
	while ((key = GetKeyPressed()) != 0) {
		switch (key) {
		case KEY_UP:
		case KEY_W:
			if (game->state == PLAYING && !snake->dir_changed && snake->dir != DOWN) {
				snake->dir = UP;
				snake->dir_changed = true;
			}
			break;
		case KEY_DOWN:
		case KEY_S:
			if (game->state == PLAYING && !snake->dir_changed && snake->dir != UP) {
				snake->dir = DOWN;
				snake->dir_changed = true;
			}
			break;
		case KEY_LEFT:
		case KEY_A:
			if (game->state == PLAYING && !snake->dir_changed && snake->dir != RIGHT) {
				snake->dir = LEFT;
				snake->dir_changed = true;
			}
			break;
		case KEY_RIGHT:
		case KEY_D:
			if (game->state == PLAYING && !snake->dir_changed && snake->dir != LEFT) {
				snake->dir = RIGHT;
				snake->dir_changed = true;
			}
			break;
		case KEY_SPACE:
			if (game->state == GAMEOVER) {
				reset_game_state(game);
			} else if (game->state == PLAYING) {
				game->state = PAUSED;
			} else if (game->state == PAUSED || game->state == MENU) {
				game->state = PLAYING;
			}
			break;
		case KEY_ESCAPE:
			// TODO: add confirmation screen
			if (game->state == MENU
					|| game->state == PAUSED
					|| game->state == GAMEOVER) {
				game->quit = true;
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
	Game game = {0};
	init_game(&game);

	double previous_time = GetTime();
	do {
		handle_events(&game);

		double current_time = GetTime();
		if (current_time - previous_time >= TICK_DELAY) {
#ifdef CNAKE_DEBUG
			printf("DEBUG: time elapsed: %.3lfms\n", (current_time - previous_time) * 1000.0f);
#endif
			update_game(&game);
			previous_time = current_time;
		}
		render(&game);
	} while (!game.quit);

	CloseWindow();
	return 0;
}

