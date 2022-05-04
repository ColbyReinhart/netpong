// cursesUtils.c
// Utility functions pertaining to working with curses

#include "pong.h"

// Global variables
extern int balls_left;
extern int netHeight;
extern char playerName[MAX_NAMESIZE];
extern char opponentName[MAX_NAMESIZE];
extern int playerPoints;
extern int opponentPoints;

// Print the current status of the game
void printStatus()
{
	char indicator[SPPBTP_MAX_MSG_LENGTH];

	// Print player score
	bzero(indicator, SPPBTP_MAX_MSG_LENGTH);
	if (sprintf(indicator, "%s: %d points", playerName, playerPoints) == -1)
	{
		wrap_up();
		perror("Problem reporting number of balls left: ");
		exit(1);
	}
	move(TOP_ROW - 4, (LEFT_EDGE + RIGHT_EDGE) / 2 - (strlen(indicator) / 2));
	addstr(indicator);

	// Print opponent score
	bzero(indicator, SPPBTP_MAX_MSG_LENGTH);
	if (sprintf(indicator, "%s: %d points", opponentName, opponentPoints) == -1)
	{
		wrap_up();
		perror("Problem reporting number of balls left: ");
		exit(1);
	}
	move(TOP_ROW - 3, (LEFT_EDGE + RIGHT_EDGE) / 2 - (strlen(indicator) / 2));
	addstr(indicator);

	// Print balls left
	bzero(indicator, SPPBTP_MAX_MSG_LENGTH);
	if (sprintf(indicator, "Balls remaining: %d", balls_left) == -1)
	{
		wrap_up();
		perror("Problem reporting number of balls left: ");
		exit(1);
	}
	move(TOP_ROW - 2, (LEFT_EDGE + RIGHT_EDGE) / 2 - (strlen(indicator) / 2));
	addstr(indicator);

	// Park cursor and refresh
	move(LINES - 1, COLS - 1);
	refresh();
}

// Show the prompt to serve the ball
void showServePrompt()
{
	// Show prompt
	const int msgSize = 20;
	const char str[] = "Get ready to serve!";
	const char eraser[] = "                   ";
	move(TOP_ROW + (netHeight / 2), (LEFT_EDGE + RIGHT_EDGE) / 2 - (msgSize / 2));
	addstr(str);

	// Leave it on screen for a bit
	move(LINES - 1, COLS - 1);
	refresh();
	sleep(2);

	// Erase it
	move(TOP_ROW + (netHeight / 2), (LEFT_EDGE + RIGHT_EDGE) / 2 - (msgSize / 2));
	addstr(eraser);
}

void draw_walls(int isClient)
{
	// Flip the court depending on client/server
	int sideWallPos = isClient ? RIGHT_EDGE + 1 : LEFT_EDGE - 1;

	// Top wall
	move(TOP_ROW - 1, LEFT_EDGE - 1);
	for (int i = LEFT_EDGE - 1; i <= RIGHT_EDGE + 1; ++i)
	{
		addch(COURT_HORIZ_SYM);
	}

	// Net
	for (int i = TOP_ROW; i < TOP_ROW + netHeight + 2; ++i)
	{
		mvaddch(i, sideWallPos, COURT_VERT_SYM);
	}

	// Bottom wall
	move(BOT_ROW + 1, LEFT_EDGE - 1);
	for (int i = LEFT_EDGE - 1; i <= RIGHT_EDGE + 1; ++i)
	{
		addch(COURT_HORIZ_SYM);
	}
}

// Erase everything in the column to the right of the net
void eraseByNet(int isClient)
{
	for (int i = TOP_ROW; i < TOP_ROW + netHeight + 1; ++i)
	{
		mvaddch(i, isClient ? RIGHT_EDGE : LEFT_EDGE, BLANK);
	}
	move(LINES - 1, COLS - 1);
}

void debugPrint(char* str)
{
	mvaddstr(BOT_ROW + 3, LEFT_EDGE, str);
}