// pong.h
// some settings for the game

#ifndef PONG_H
#define PONG_H

#include <curses.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// General
#define BLANK			' '
#define DFL_SYMBOL		'O'
#define TICKS_PER_SEC	50	// affects speed

// Response actions
#define LOSE 0		// Indicates the opponent missed and we must serve now
#define PLAY 1		// Indicates the is still in normal play
#define BOUNCE 2	// Indicates that the ball has bounced off something
#define FINISH 3

// Reponse codes
#define ERROR -1	// Indicates something went wrong, terminate immediately
#define SERV 4
#define BALL 5
#define MISS 6
#define DONE 7
#define QUIT 8

// Court
#define NET_HEIGHT		16
#define TOP_ROW			4
#define BOT_ROW			21
#define LEFT_EDGE		9
#define RIGHT_EDGE		70
#define COURT_HORIZ_SYM	'-'
#define COURT_VERT_SYM	':'

// Ball
#define Y_INIT			(BOT_ROW - TOP_ROW) / 2
#define MIN_Y_TTM		3
#define MAX_Y_TTM		8
#define MIN_X_TTM		1
#define MAX_X_TTM		5
#define STARTING_BALLS	5

// Misc
#define GAME_VERSION 			"1.0"
#define MAX_NAMESIZE			25
#define SPPBTP_MAX_MSG_LENGTH	128
#define SPPBTP_MAX_ARG_LENGTH	40

//
// pongUtils.c
//

#define HOSTLEN 256
#define errorQuit(msg) { perror(msg); exit(1); }

int getOpponent();
void printUsage();
void serverSetup(char* portnum);
int clientSetup(char* host, char* port);
int socket_check(int readResult);
int serverIntro(int sock_fd);
int clientIntro(int sock_fd);
int getResponse(int sock_fd);
int sendResponse(int sock_fd, int responseType);
int sendNegativeStatus(int sock_fd, char* msg);

//
// cursesUtils.c
//

void printStatus();
void showServePrompt();
void draw_walls(int isClient);
void eraseByNet(int isClient);
void debugPrint(char* str);

//
// paddle.c
//

struct ppball
{
	int y_pos, x_pos,
		y_ttm, x_ttm,	// ticks-to-move (how many ticks before it will move)
		y_ttg, x_ttg,	// ticks-to-go (how many remaining ticks before it will move)
		y_dir, x_dir;
	char symbol;
};

void draw_paddle();
void paddle_init();
void paddle_up();
void paddle_down();
int bounce_or_lose(struct ppball* bp);
int paddle_contact(int y, int x);

//
// pong.c
//

void serve();
void wrap_up();

#endif // PONG_H