# Makefile for netpong

netpong: netpong.c paddle.c pongUtils.c cursesUtils.c
	cc -o netpong netpong.c paddle.c pongUtils.c cursesUtils.c -lcurses

clean:
	rm -f netpong