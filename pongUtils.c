// pongUtils.c
// Utility functions for netpong

#include "pong.h"

// Setup variables
int ticksPerSecond;				// Ticks per second
int netHeight;					// Height of the net
char playerName[MAX_NAMESIZE];	// Player's name
char opponentName[MAX_NAMESIZE];// Opponent's name
char errorMessage[SPPBTP_MAX_MSG_LENGTH];
int listen_socket;

// Game variables
int balls_left;					// Number of balls left in play
int netPosition;				// Offset of ball from top of net
int xttm, yttm;					// Ball x and y time to move
int ydir;						// Y direction of ball (-1 up, 1 down)

// Specify netpong commands
void printUsage()
{
	printf("USAGE:\n");
	printf("Server: ./netpong -s PORTNUM_TO_USE\n");
	printf("Client: ./netpong -c HOSTNAME:PORT\n");
	exit(0);
}

// Check a read return value to see if:
// It returned an error (-1)
// The other side is closed (0)
int socket_check(int readResult)
{
	if (readResult == -1)
	{
		perror("Could not read from socket\n");
		return -1;
	}
	if (readResult == 0)
	{
		printf("Lost connection to socket. Closing...\n");
		return -1;
	}

	return 0;
}

// Parse the resonse from a SPPBTP communication
// Input: reference to the received message
// Output: dynamic array of strings containing the output
// Array has max size of 10 and is null terminated
// Each string has a max length of SPPBTP_MAX_ARG_LENGTH and is nul terminated
// Assumes that the response follows SPPBTP
char** parseResponse(char* response)
{
	const int maxNumArgs = 10;
	char** result = (char**)malloc((maxNumArgs + 1) * sizeof(char*));
	result[maxNumArgs] = 0;

	// Fill result
	int pos = 0;
	int resultIndex = 0;
	// For each element in the response
	while (response[pos] != '\n')
	{
		// Allocate memory for a new argument
		result[resultIndex] = (char*)malloc(SPPBTP_MAX_ARG_LENGTH + 1);
		// For each element in the first word, fill in the argument
		int argPos = 0;
		while (response[pos] != ' ')
		{
			if (response[pos] == '\r') break;
			result[resultIndex][argPos] = response[pos];
			++pos;
			++argPos;
		}
		result[resultIndex][argPos] = 0; // Null terminate
		++resultIndex;
		++pos;

		if (resultIndex >= maxNumArgs)
		{
			printf("Response parse error: max number of args reached\n");
			printf("Tried to parse: %s\n", response);
			printf("Failed at pos %d resultIndex %d\n", pos, resultIndex);
			exit(1);
		}
	}

	result[resultIndex] = 0;
	return result;
}

// Deallocate a response list generated by parseResponse()
void freeResponseList(char** responseList)
{
	int pos = 0;
	while(responseList[pos] != 0)
	{
		free(responseList[pos]);
		++pos;
	}
	free(responseList);
}

// Send a negative indicator with accompanying error message
// Pass null for the second argument if no message is desired
// Returns -1 on error, 0 on success
int sendNegativeStatus(int sock_fd, char* msg)
{
	char comm_buffer[SPPBTP_MAX_MSG_LENGTH];
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer
	if (strlen(msg) > SPPBTP_MAX_ARG_LENGTH)
	{
		printf("Error: negative status argument exceeds max length\n");
		return -1;
	}
	char message_to_send[SPPBTP_MAX_ARG_LENGTH] = "";
	if (msg != NULL)
	{
		strcpy(message_to_send, msg);
	}
	sprintf(comm_buffer, "?ERR %s\r\n", message_to_send);
	if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
	{
		perror("Could not write to client\n");
		return -1;
	}

	return 0;
}

// Accept a connection with a client and return the file descriptor
int getOpponent()
{
	// Accept an incoming call from a client
	printf("Waiting for new connection . . .\n");
	int sock_fd = accept(listen_socket, NULL, NULL);
	if (sock_fd == -1) errorQuit("Couldn't accept client socket call");
	printf("Connected to client.\n");
	return sock_fd;
}

// Create a socket and listen for client
void serverSetup(char* portnum)
{
	struct sockaddr_in server_addr;	// Server address information
	struct hostent* host_info;		// Host information
	char hostname[HOSTLEN];			// Buffer to store host name

	// Ask kernel for a socket from which to listen
	listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket == -1) errorQuit("Couldn't get listening socket");

	// Get information about the server host
	bzero((void*)&server_addr, sizeof(server_addr));
	gethostname(hostname, HOSTLEN);
	host_info = gethostbyname(hostname);

	// Alert user of host credentials
	printf("Hosting at %s:%s\n", hostname, portnum);

	// Fill host information into hostent* struct
	bcopy(
		(void*)host_info->h_addr,
		(void*)&server_addr.sin_addr,
		host_info->h_length
	);
	server_addr.sin_port = htons(atoi(portnum));
	server_addr.sin_family = AF_INET;

	// Bind server address to listening socket
	if
	(
		bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))
		!= 0
	)
	{
		errorQuit("Could not bing server credentials to listening socket");
	}

	// Allow incoming calls to Qsize = 1 on listening socket
	if (listen(listen_socket, 1) != 0) errorQuit("Couldn't listen for calls");
}

// Create a socket and call the server provided
int clientSetup(char* host, char* port)
{
	struct sockaddr_in server_addr;	// Server address information
	struct hostent* host_info;		// Host information

	// Get a socket to talk to the server
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) errorQuit("Client couldn't get a socket");

	// Build the address of the server specified
	bzero(&server_addr, sizeof(server_addr));
	host_info = gethostbyname(host);
	if( host_info == NULL) errorQuit("Couldn't get server host");
	bcopy(
		host_info->h_addr,
		(struct sockaddr*)&server_addr.sin_addr,
		host_info->h_length
	);
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_family = AF_INET;

	// Dial the server
	if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
	{
		errorQuit("Client couldn't dial server");
	}
	printf("Connected to server.\n");

	return sock_fd;
}

// Server introduction procedure
int serverIntro(int sock_fd)
{
	char comm_buffer[SPPBTP_MAX_MSG_LENGTH];

	// Initialize server-decided variables
	ticksPerSecond = TICKS_PER_SEC;
	netHeight = NET_HEIGHT;
	balls_left = STARTING_BALLS;

	// Issue welcome command
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer
	sprintf
	(
		comm_buffer,
		"HELO %s %d %d %s\r\n",
		GAME_VERSION,
		TICKS_PER_SEC,
		NET_HEIGHT,
		playerName
	);
	if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
	{
		perror("Could not write to client\n");
		return -1;
	}

	// Wait for response
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer
	int messageSize = read(sock_fd, comm_buffer, SPPBTP_MAX_MSG_LENGTH);
	if (socket_check(messageSize) == -1) return -1;

	// Interpret response
	char** responseList = parseResponse(comm_buffer);
	// Check response header
	if (strcmp(responseList[0], "?ERR") == 0)
	{
		printf("Error: client returned negative status indicator\n");
		return -1;
	}
	if (strcmp(responseList[0], "NAME") != 0)
	{
		sprintf(errorMessage, "Expected NAME response, got %s", responseList[0]);
		printf("Error: %s\n", errorMessage);
		// Send negative status indicator
		sendNegativeStatus(sock_fd, errorMessage);
		return -1;
	}
	// Check client version
	if (strcmp(responseList[1], GAME_VERSION) != 0)
	{
		sprintf
		(
			errorMessage,
			"Error: Server is using ver%s but client is using ver%s",
			GAME_VERSION,
			responseList[1]
		);
		printf("Error: %s\n", errorMessage);
		sendNegativeStatus(sock_fd, errorMessage);
		return -1;
	}
	// Read client name
	strcpy(opponentName, responseList[2]);

	// Deallocate responseList and return success
	freeResponseList(responseList);
	return 0;
}

// Client introduction procedure
int clientIntro(int sock_fd)
{
	// Wait for welcome command
	char comm_buffer[SPPBTP_MAX_MSG_LENGTH];
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer
	int messageSize = read(sock_fd, comm_buffer, SPPBTP_MAX_MSG_LENGTH);
	if (socket_check(messageSize) == -1) return -1;

	// Interpret welcome command
	char** responseList = parseResponse(comm_buffer);
	// Check response header
	if (strcmp(responseList[0], "?ERR") == 0)
	{
		printf("Error: server returned negative status indicator\n");
		return -1;
	}
	if (strcmp(responseList[0], "HELO") != 0)
	{
		sprintf(errorMessage, "Expected HELO response, got %s", responseList[0]);
		printf("Error: %s\n", errorMessage);
		// Send negative status indicator
		sendNegativeStatus(sock_fd, errorMessage);
		return -1;
	}
	// Check server version
	if (strcmp(responseList[1], GAME_VERSION) != 0)
	{
		printf
		(
			"Error: Server is using ver%s but client is using ver%s\n",
			responseList[1],
			GAME_VERSION[1]
		);
		return -1;
	}
	// Check ticks_per_sec
	ticksPerSecond = atoi(responseList[2]);
	// Check net height
	netHeight = atoi(responseList[3]);
	// Read client name
	strcpy(opponentName, responseList[4]);

	// Deallocate responseList
	freeResponseList(responseList);
	
	// Issue welcome response
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer
	sprintf
	(
		comm_buffer,
		"NAME %s %s\r\n",
		GAME_VERSION,
		playerName
	);
	if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
	{
		perror("Could not write to server\n");
		return -1;
	}

	// Return success
	return 0;
}

// Wait for a response, and then interpret it
// This is intended to be used only in the playing phase
// Possible response headers: SERV, BALL, MISS, QUIT, DONE
// Input: socket file descriptor to read from
// Returns response type
int getResponse(int sock_fd)
{
	// Wait for response
	char comm_buffer[SPPBTP_MAX_MSG_LENGTH];
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer
	int messageSize = read(sock_fd, comm_buffer, SPPBTP_MAX_MSG_LENGTH);
	if (socket_check(messageSize) == -1) return -1;

	// Interpret response
	char** responseList = parseResponse(comm_buffer);
	
	// Was there an error?
	if (strcmp(responseList[0], "?ERR") == 0)
	{
		sprintf(errorMessage, "Received negative status indicator: %s", responseList[1]);
		return ERROR;
	}

	// Should we serve the first ball?
	else if (strcmp(responseList[0], "SERV") == 0)
	{
		balls_left = atoi(responseList[1]);
		return SERV;
	}

	// Is the ball in our court now?
	else if (strcmp(responseList[0], "BALL") == 0)
	{
		netPosition = atoi(responseList[1]);
		xttm = atoi(responseList[2]);
		yttm = atoi(responseList[3]);
		ydir = atoi(responseList[4]);
		return BALL;
	}

	// Did the opponent miss their ball?
	else if (strcmp(responseList[0], "MISS") == 0)
	{
		return MISS;
	}

	// Did the opponent quit?
	else if (strcmp(responseList[0], "QUIT") == 0)
	{
		return QUIT;
	}

	// Did the opponent say the game is done?
	else if(strcmp(responseList[0], "DONE") == 0)
	{
		return DONE;
	}

	// If the message was something else, return error
	else
	{
		sprintf(errorMessage, "Bad response: %s", responseList[0]);
		sendNegativeStatus(sock_fd, errorMessage);
		return ERROR;
	}
}

// Send the given response
// This is intended to be used only in the playing phase
// Possible response headers: SERV, BALL, MISS, QUIT
// Input: socket file descriptor to write to, response
// Returns 0 on success, -1 on error
int sendResponse(int sock_fd, int responseType)
{
	char comm_buffer[SPPBTP_MAX_MSG_LENGTH];
	bzero(comm_buffer, SPPBTP_MAX_MSG_LENGTH);	// Clear the buffer

	switch (responseType)
	{
		case SERV:
			sprintf(comm_buffer, "SERV %d\r\n", STARTING_BALLS);
			if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
			{
				perror("Could not write to opponent\n");
				return -1;
			}
			break;

		case BALL:
			sprintf
			(
				comm_buffer,
				"BALL %d %d %d %d 0\r\n",
				netPosition,
				xttm,
				yttm,
				ydir
			);
			if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
			{
				perror("Could not write to opponent\n");
				return -1;
			}
			break;

		case MISS:
			sprintf(comm_buffer, "MISS [NULL]\r\n");
			if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
			{
				perror("Could not write to opponent\n");
				return -1;
			}
			break;

		case QUIT:
			sprintf(comm_buffer, "QUIT [NULL]\r\n");
			if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
			{
				perror("Could not write to opponent\n");
				return -1;
			}
			break;

		case DONE:
			sprintf(comm_buffer, "DONE [NULL]\r\n");
			if (write(sock_fd, comm_buffer, strlen(comm_buffer)) == -1)
			{
				perror("Could not write to opponent\n");
				return -1;
			}
			break;

		default:
			printf("Error: invalid response type given to sendResponse()\n");
			return -1;
	}

	return 0;
}