// Pong
// By Colby Reinhart
// 3-1-2022
// A simple pong game whcih uses the curses library
// Now supporting multiplayer functionality

#include "pong.h"

// Function prototypes
void set_up();
int set_ticker();
void ball_move(int signum);
int interpretResponse(int response);

// Setup variables
int sock_fd;					// Will hold the socket fd
int isClient;					// Are we running in client mode?
extern int ticksPerSecond;
extern int netHeight;
extern char playerName[MAX_NAMESIZE];
extern char opponentName[MAX_NAMESIZE];
extern char errorMessage[SPPBTP_MAX_MSG_LENGTH];

// Game variables
struct ppball the_ball;
extern int balls_left;			// Number of balls left in play
extern int netPosition;			// Offset of ball from top of net
extern int xttm, yttm;			// Ball x and y time to move
extern int ydir;				// Y direction of ball (-1 up, 1 down)
int playerPoints = 0;
int opponentPoints = 0;

// USAGE:
// Server: ./netpong -s PORT
// Client: ./netpong -c HOSTNAME:PORT
int main(int argc, char* argv[])
{
	// Get player's name
	///printf("Enter your name: ");
	///fgets(playerName, MAX_NAMESIZE, stdin);
	///playerName[strcspn(playerName, "\n")] = 0; // Remove newline
	
	/////
	if (strcmp(argv[1], "-s") == 0)
	{
		strcpy(playerName, "Server");
	}
	else
	{
		strcpy(playerName, "Client");
	}
	/////

	//
	// Setup
	//

	if (argc != 3) printUsage();
	strcpy(errorMessage, "");

	// Are we running in server mode?
	if (strcmp(argv[1], "-s") == 0)
	{
		isClient = 0;
		sock_fd = serverSetup(argv[2]);
	}
	// Are we running in client mode?
	else if (strcmp(argv[1], "-c") == 0)
	{
		isClient = 1;
		sock_fd = clientSetup(argv[2]);
	}
	else
	{
		printUsage();
	}

	//
	// Game initialization
	//

	// Run INTRODUCTION STATE

	if (isClient)
	{
		if (clientIntro(sock_fd) == -1)
		{
			close(sock_fd);
			exit(1);
		}
	}
	else
	{
		if (serverIntro(sock_fd) == -1)
		{
			close(sock_fd);
			exit(1);
		}
	}

	//
	// Start the game
	//

	set_up();

	// Client setup
	if (isClient)
	{
		if (getResponse(sock_fd) == SERV)
		{
			printStatus();
			serve();
		}
		else
		{
			wrap_up();
			printf("Error: expected SERV from server\n");
			sendResponse(sock_fd, QUIT);
			exit(1);
		}
	}

	// Server setup
	else
	{
		printStatus();

		// Show serving prompt
		const int msgSize = 26;
		const char str[] = "Opponent is serving . . .";
		const char eraser[] = "                         ";
		move(TOP_ROW + (netHeight / 2), (LEFT_EDGE + RIGHT_EDGE) / 2 - (msgSize / 2));
		addstr(str);
		move(LINES - 1, COLS - 1);
		refresh();

		// Listen for the ball
		if (sendResponse(sock_fd, SERV) == -1)
		{
			wrap_up();
			exit(1);
		}

		// Erase prompt
		move(TOP_ROW + (netHeight / 2), (LEFT_EDGE + RIGHT_EDGE) / 2 - (msgSize / 2));
		addstr(eraser);
		move(LINES - 1, COLS - 1);
		refresh();

		interpretResponse(getResponse(sock_fd));
	}

	int c;
	while ((c = getchar()) != 'Q')
	{
		if (c == 'k') paddle_up();
		if (c == 'j') paddle_down();
	}

	// If we got here, then the player has quit
	// Send a done response to opponent and wait for their response
	if (sendResponse(sock_fd, DONE) == -1)
	{
		wrap_up();
		exit(1);
	}
	interpretResponse(getResponse(sock_fd));
}

int set_ticker(int n_msecs)
{
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec = n_msecs / 1000;						// int part
	n_usecs = (n_msecs % 1000) * 1000L;			// remainder

	new_timeset.it_interval.tv_sec = n_sec;		// set reload
	new_timeset.it_interval.tv_usec = n_usecs;	// new ticker value
	new_timeset.it_value.tv_sec = n_sec;		// store this
	new_timeset.it_value.tv_usec = n_usecs;		// and this

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

void serve()
{
	set_ticker(0);
	// Let the player know they're serving	
	showServePrompt();

	// Get initial velocities
	int initXVel = (rand() % (MAX_X_TTM - MIN_X_TTM)) + MIN_X_TTM;
	int initYVel = (rand() % (MAX_Y_TTM - MIN_Y_TTM)) + MIN_Y_TTM;

	// Serve the ball
	the_ball.y_pos = Y_INIT;
	the_ball.x_pos = isClient ? RIGHT_EDGE - 5 : LEFT_EDGE + 5;
	the_ball.y_ttg = the_ball.y_ttm = initYVel;
	the_ball.x_ttg = the_ball.x_ttm = initXVel;
	the_ball.y_dir = (rand() % 2) ? 1 : -1;
	the_ball.x_dir = isClient ? -1 : 1;

	set_ticker(1000 / ticksPerSecond);
}

void set_up()
{
	// Init random
	srand(getpid());

	// curses init
	initscr();
	noecho();
	crmode();
	mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol); // draw the ball

	// Draw the field
	paddle_init();
	draw_walls(isClient);
	refresh();

	// ball
	the_ball.symbol = DFL_SYMBOL;

	// signals
	///signal(SIGINT, SIG_IGN);
	signal(SIGALRM, ball_move);
	set_ticker(1000 / ticksPerSecond);	// send millisecs per tick
}

void wrap_up()
{
	// Run closing stuff
	set_ticker(0);
	close(sock_fd);
	endwin();
	
	// Print error messages, if any
	if (strlen(errorMessage) != 0)
	{
		printf("Error: %s\n", errorMessage);
	}

	// Announce winner
	printf("Game over!\nThe winner is: ");
	char temp[MAX_NAMESIZE];
	if (playerPoints > opponentPoints)
	{
		strcpy(temp, playerName);
	}
	else if (opponentPoints > playerPoints)
	{
		strcpy(temp, opponentName);
	}
	else
	{
		strcpy(temp, "A tie");
	}
	printf("%s!\n", temp);

	printf
	(
		"%s: %d points --- %s: %d points\n",
		playerName,
		playerPoints,
		opponentName,
		opponentPoints
	);
}

// Interpret the resonse received from the opponent
// Input: the received response code
// Output: the response action to take
int interpretResponse(int response)
{
	// Did the opponent send back a negative status indicator?
	if (response == ERROR)
	{
		wrap_up();
		exit(1);
	}

	// Did the opponet send back the ball?
	else if (response == BALL)
	{
		// Reinitialize ball
		the_ball.x_pos = isClient ? RIGHT_EDGE -1 : LEFT_EDGE + 1;
		the_ball.y_pos = netPosition;
		the_ball.x_ttg = the_ball.x_ttm = xttm;
		the_ball.y_ttg = the_ball.y_ttm = yttm;
		the_ball.x_dir = isClient ? -1 : 1;
		the_ball.y_dir = ydir;

		// Return PLAY
		return PLAY;
	}

	// Did the opponent miss?
	else if (response == MISS)
	{
		// Give the player a point
		++playerPoints;

		// Take one ball out of play
		--balls_left;

		// Update the scoreboard
		printStatus();

		// Is the game done?
		if (!balls_left)
		{
			// Send a done response to opponent and wait for their response
			if (sendResponse(sock_fd, DONE) == -1)
			{
				wrap_up();
				exit(1);
			}

			// Did the opponent quit?
			interpretResponse(getResponse(sock_fd));
		}

		// Tell bounce_or_lose() to serve
		return LOSE;
	}

	//Did the opponent say the game is over?
	else if (response == DONE)
	{
		// Send back a quit response
		if (sendResponse(sock_fd, QUIT) == -1)
		{
			wrap_up();
			exit(1);
		}

		wrap_up();
		exit(0);
	}

	// Did the opponent say it's time to quit?
	else if (response == QUIT)
	{
		wrap_up();
		exit(0);
	}

	// If we didn't get a valid resonse
	else
	{
		wrap_up();
		sendNegativeStatus(sock_fd, "Got bad response from opponent");
		printf("Error: got bad response from opponent\n");
		return ERROR;
	}
}

// Send the ball to the other player
int passToOpponent()
{
	// Init variables
	netPosition = the_ball.y_pos;
	xttm = the_ball.x_ttm;
	yttm = the_ball.y_ttm;
	ydir = the_ball.y_dir;

	// Send BALL response
	if (sendResponse(sock_fd, BALL) == -1)
	{
		wrap_up();
		exit(1);
	}

	// Erase the ball
	eraseByNet(isClient);

	// Wait for opponent response
	return interpretResponse(getResponse(sock_fd));
}

// Tell the other player to serve, since we missed
int playerMissed()
{
	// Take away one ball
	--balls_left;

	// Give a point to the opponent
	++opponentPoints;

	// Update the scoreboard
	printStatus();

	// Tell the other player we missed and wait
	if (sendResponse(sock_fd, MISS) == -1)
	{
		wrap_up();
		exit(1);
	}

	// Wait for opponent response
	return interpretResponse(getResponse(sock_fd));
}

// Check collisions and take the appropriate action
int bounce_or_lose(struct ppball* bp)
{
	// If we hit the paddle
	if (paddle_contact(bp->y_pos, bp->x_pos))
	{
		bp->x_dir = isClient ? 1 : -1;

		// Randomize speed of ball
		the_ball.y_ttg = the_ball.y_ttm = (rand() % (MAX_Y_TTM - MIN_Y_TTM) + MIN_Y_TTM);
		the_ball.x_ttg = the_ball.x_ttm = (rand() % (MAX_X_TTM - MIN_X_TTM) + MIN_X_TTM);

		return BOUNCE;
	}

	// If we hit the top wall
	if (bp->y_pos <= TOP_ROW)
	{
		bp->y_dir = 1;
		return BOUNCE;
	}

	// If we hit the bottom wall
	else if (bp->y_pos >= TOP_ROW + netHeight + 1)
	{
		bp->y_dir = -1;
		return BOUNCE;
	}

	// If the ball crosses into the opponent's court
	if (bp->x_pos <= LEFT_EDGE)
	{
		if (isClient)
		{
			return playerMissed();
		}
		else
		{
			return passToOpponent();
		}
	}

	// If the ball got past the paddle
	else if (bp->x_pos >= RIGHT_EDGE)
	{
		if (isClient)
		{
			return passToOpponent();
		}
		else
		{
			return playerMissed();
		}
	}
}

void ball_move(int signum)
{
	int y_cur, x_cur, moved;

	signal(SIGALRM, SIG_IGN);	// don't get caught now
	y_cur = the_ball.y_pos;		// old spot
	x_cur = the_ball.x_pos;
	moved = 0;

	// If it's time to move up or down
	if (the_ball.y_ttm > 0 && the_ball.y_ttg-- == 1)
	{
		the_ball.y_pos += the_ball.y_dir;	// move
		the_ball.y_ttg = the_ball.y_ttm;	// reset
		moved = 1;
	}

	// If it's time to move left or right
	if (the_ball.x_ttm > 0 && the_ball.x_ttg-- == 1)
	{
		the_ball.x_pos += the_ball.x_dir;	// move
		the_ball.x_ttg = the_ball.x_ttm;	// reset
		moved = 1;
	}

	// If we moved
	if (moved)
	{
		// Move the ball
		mvaddch(y_cur, x_cur, BLANK);
		mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);

		// If it's our turn to serve, do so
		int result = bounce_or_lose(&the_ball);
		if (result == LOSE) serve();
		else if (result == BOUNCE) draw_walls(isClient);
		
		// Draw the paddle and ball
		draw_paddle();

		// Park cursor and refresh
		move(LINES - 1, COLS - 1);
		refresh();
	}

	signal(SIGALRM, ball_move); // for unreliable systems
}