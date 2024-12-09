/* System Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>

/* Local Header Files */
#include "list.h"

#define MAX_READERS 25
#define TRUE   1  
#define FALSE  0  
#define PORT 8888  
#define max_clients  30
#define DEFAULT_ROOM "Lobby"
#define MAXBUFF   2096
#define BACKLOG 2 

// Global Variables
extern pthread_mutex_t rw_lock;       // Reader-writer lock for thread safety
extern pthread_mutex_t mutex;         // Mutex for synchronization
extern int numReaders;                // Number of readers (for synchronization)
extern char *server_MOTD;             // Message of the Day
extern struct node *head;             // Head of the user linked list

// Room structure
struct room {
    char name[50];
    struct node *userList;          // Linked list of users in the room
    struct room *next;              // Pointer to the next room
};

// Function Prototypes
int get_server_socket();
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);
void sigintHandler(int sig_num);
void *client_receive(void *ptr);

// Room Management Function Prototypes
struct room *findRoom(const char *roomName);
struct room *createRoom(const char *roomName);
void addUserToRoomList(struct room *room, struct node *user);

// Utility Function Prototypes
char *trimwhitespace(char *str);
