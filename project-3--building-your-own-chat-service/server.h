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

/* Macro Definitions */
#define MAX_READERS 25
#define TRUE   1  
#define FALSE  0  
#define PORT 8888  
#define delimiters " "
#define max_clients  30
#define DEFAULT_ROOM "Lobby"
#define MAXBUFF   2096
#define BACKLOG 2 

/* Global Variables */
extern pthread_mutex_t rw_lock;  // Reader-writer lock for shared data
extern pthread_mutex_t mutex;    // Mutex for managing reader count
extern int numReaders;           // Number of readers currently accessing data

extern struct user *users_head;  // Head of the linked list of users
extern struct room *rooms_head;  // Head of the linked list of rooms

/* Data Structures */

// Structure representing a user
typedef struct user {
    int socket;                      // User's socket descriptor
    char username[50];               // User's name
    struct room_node *rooms;         // List of rooms the user is part of
    struct connection_node *connections; // List of direct connections (DMs)
    struct user *next;               // Pointer to the next user
} user_t;

// Structure for a direct message (DM) connection
typedef struct connection_node {
    user_t *connected_user;          // Pointer to the connected user
    struct connection_node *next;    // Next connection in the list
} connection_node_t;

// Structure representing a chat room
typedef struct room {
    char room_name[50];              // Name of the room
    struct user_node *users_in_room; // List of users in the room
    struct room *next;               // Pointer to the next room
} room_t;

// Node in the list of users in a room
typedef struct user_node {
    user_t *user;                    // Pointer to the user
    struct user_node *next;          // Pointer to the next user in the room
} user_node_t;

// Node in the list of rooms a user is part of
typedef struct room_node {
    room_t *room;                    // Pointer to the room
    struct room_node *next;          // Pointer to the next room
} room_node_t;

/* Function Prototypes */

// Server setup and client management
int get_server_socket();
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);
void sigintHandler(int sig_num);
void *client_receive(void *ptr);

// User management
user_t* find_user_by_socket(int socket);
user_t* find_user_by_name(const char *username);
user_t* insert_user(int socket, const char *username);
void remove_user(user_t *user);
void remove_connection(user_t *u, user_t *v);

// Room management
room_t* create_room(const char *room_name);
room_t* find_room(const char *room_name);
int join_room(user_t *user, const char *room_name);
int leave_room(user_t *user, const char *room_name);

// Direct connection (DM) management
int connect_users(user_t *u1, user_t *u2);
int disconnect_users(user_t *u1, user_t *u2);

// Message sending
void send_message(user_t *sender, const char *message);

// Synchronization (Reader-Writer Locks)
void start_read();
void end_read();
void start_write();
void end_write();

// Utility functions
char *trimwhitespace(char *str);



