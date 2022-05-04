// paddle.c
// By Colby Reinhart
// 3-1-2022
// Implementation for the paddle in pong

#include "pong.h"

#define PAD_CHAR '#'
#define PAD_HEIGHT 6
#define PAD_START (BOT_ROW + TOP_ROW) / 2 - (PAD_HEIGHT / 2)

struct pppaddle
{
	int pad_top, pad_bot, pad_col;
	char pad_char;
};

static struct pppaddle paddle;
extern struct ppball the_ball;
extern int isClient;

void draw_paddle()
{
	// Draw the paddle
	for (int i = paddle.pad_top; i <= paddle.pad_bot; ++i)
	{
		mvaddch(i, paddle.pad_col, paddle.pad_char);
	}

	/// Clear the area around the paddle (just in case)

	// // Above
	// for (int i = TOP_ROW; i < paddle.pad_top; ++i)
	// {
	// 	mvaddch(i, paddle.pad_col, BLANK);
	// }

	// // Below
	// for (int i = paddle.pad_bot + 1; i <= BOT_ROW; ++i)
	// {
	// 	mvaddch(i, paddle.pad_col, BLANK);
	// }
}

void paddle_init()
{
	// Initialize structure
	paddle.pad_char = PAD_CHAR;
	paddle.pad_top = PAD_START;
	paddle.pad_bot = PAD_START + PAD_HEIGHT;
	paddle.pad_col = isClient ? LEFT_EDGE : RIGHT_EDGE;

	// draw paddle on screen
	draw_paddle();
}

void paddle_up()
{
	if (paddle.pad_top > TOP_ROW)
	{
		// Erase bottom
		mvaddch(paddle.pad_bot, paddle.pad_col, BLANK);

		// Draw top
		paddle.pad_bot--;
		paddle.pad_top--;
		mvaddch(paddle.pad_top, paddle.pad_col, paddle.pad_char);
	}

	if (bounce_or_lose(&the_ball) == LOSE)
	{
		serve();
	}
}

void paddle_down()
{
	if (paddle.pad_bot < BOT_ROW)
	{
		// Erase top
		mvaddch(paddle.pad_top, paddle.pad_col, BLANK);

		// Draw bottom
		paddle.pad_bot++;
		paddle.pad_top++;
		mvaddch(paddle.pad_bot, paddle.pad_col, paddle.pad_char);
	}

	if (bounce_or_lose(&the_ball) == LOSE)
	{
		serve();
	}
}

int paddle_contact(int y, int x)
{
	int offset = isClient ?  1 : -1;

	return y >= paddle.pad_top &&		// Is the ball below the top of the paddle?
		y <= paddle.pad_bot &&			// Is the ball above the bot of the paddle?
		(x == paddle.pad_col + offset ||// Is the ball right in front of the paddle?
		x == paddle.pad_col);			// Is the ball colliding with the paddle?
}