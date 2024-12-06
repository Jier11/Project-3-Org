#include "server.h"
#include "list.h"


int chat_serv_sock_fd; // Server socket
struct node *head = NULL; // User list
struct room *rooms = NULL; // Room list

pthread_mutex_t rw_lock = PTHREAD_MUTEX_INITIALIZER; // Reader-writer lock
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for counters
int numReaders = 0;

void sigintHandler(int sig_num) {
    printf("\nShutting down server...\n");

    pthread_mutex_lock(&rw_lock);

    // Notify users about server shutdown
    struct node *current_user = head;
    while (current_user) {
        char shutdown_message[] = "Server is shutting down...\n";
        send(current_user->socket, shutdown_message, strlen(shutdown_message), 0);
        close(current_user->socket);
        struct node *temp = current_user;
        current_user = current_user->next;
        free(temp);
    }

    // Free room memory
    struct room *current_room = rooms;
    while (current_room) {
        struct room *temp = current_room;
        current_room = current_room->next;
        free(temp);
    }

    pthread_mutex_unlock(&rw_lock);

    close(chat_serv_sock_fd);
    exit(0);
}

int main(int argc, char **argv) {
    // Handle Ctrl+C
    signal(SIGINT, sigintHandler);

    // Create the default room (Lobby)
    pthread_mutex_lock(&rw_lock);
    rooms = insertRoom(rooms, DEFAULT_ROOM);
    pthread_mutex_unlock(&rw_lock);

    // Set up server socket
    chat_serv_sock_fd = get_server_socket();
    if (start_server(chat_serv_sock_fd, BACKLOG) == -1) {
        printf("Error: Failed to start server.\n");
        exit(1);
    }

    printf("Server Launched! Listening on PORT: %d\n", PORT);

    while (1) {
        int new_client = accept_client(chat_serv_sock_fd);
        if (new_client != -1) {
            pthread_t new_client_thread;
            pthread_create(&new_client_thread, NULL, client_receive, (void *)&new_client);
        }
    }

    close(chat_serv_sock_fd);
}

int get_server_socket() {
    int opt = 1;
    int master_socket;
    struct sockaddr_in address;

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    return master_socket;
}

int start_server(int serv_socket, int backlog) {
    return listen(serv_socket, backlog);
}

int accept_client(int serv_sock) {
    struct sockaddr_storage client_addr;
    socklen_t sin_size = sizeof(client_addr);
    return accept(serv_sock, (struct sockaddr *)&client_addr, &sin_size);
}
