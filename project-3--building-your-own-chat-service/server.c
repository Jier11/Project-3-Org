#include "server.h"

int chat_serv_sock_fd; // server socket

/////////////////////////////////////////////
// USE THESE LOCKS AND COUNTER TO SYNCHRONIZE

int numReaders = 0; // keep count of the number of readers

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // mutex lock
pthread_mutex_t rw_lock = PTHREAD_MUTEX_INITIALIZER;  // read/write lock

/////////////////////////////////////////////

char const *server_MOTD = "Thanks for connecting to the BisonChat Server.\n\nchat>";

struct node *head = NULL;

int main(int argc, char **argv) {
   struct sigaction sa;
   sa.sa_handler = sigintHandler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGINT, &sa, NULL) == -1) {
      perror("sigaction");
      exit(EXIT_FAILURE);
   }


    ////////////////////////////////////////////////////////
    // Create the default room for all clients to join when 
    // initially connecting
    ////////////////////////////////////////////////////////
    pthread_mutex_lock(&rw_lock); // Lock to ensure thread safety

    // Initialize the default room
    head = (struct node *)malloc(sizeof(struct node));
    if (head == NULL) {
        perror("Error allocating memory for default room");
        exit(EXIT_FAILURE);
    }

    // Set up the default room
    strcpy(head->username, "Default Room"); // Using username as room identifier
    head->socket = -1; // Indicates this is a room, not a user
    head->next = NULL;

    pthread_mutex_unlock(&rw_lock); // Unlock after setting up the room
    ////////////////////////////////////////////////////////

    // Open server socket
    chat_serv_sock_fd = get_server_socket();

    // Step 3: Get ready to accept connections
    if (start_server(chat_serv_sock_fd, BACKLOG) == -1) {
        printf("start server error\n");
        exit(1);
    }
    
    printf("Server Launched! Listening on PORT: %d\n", PORT);
    
    // Main execution loop
    while (1) {
        // Accept a connection and start a thread
        int new_client = accept_client(chat_serv_sock_fd);
        if (new_client != -1) {
            pthread_t new_client_thread;
            pthread_create(&new_client_thread, NULL, client_receive, (void *)&new_client);
        }
    }

    close(chat_serv_sock_fd);
}

int get_server_socket() {
    int opt = TRUE;   
    int master_socket;
    struct sockaddr_in address; 
    
    // Create a master socket  
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
    
    // Set master socket to allow multiple connections
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
    
    // Type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons(PORT);   
         
    // Bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   

    return master_socket;
}

int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}

int accept_client(int serv_sock) {
    int reply_sock_fd = -1;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct sockaddr_storage client_addr;

    // Accept a connection request from a client
    if ((reply_sock_fd = accept(serv_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
        printf("socket accept error\n");
    }
    return reply_sock_fd;
}

/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
    printf("Server shutting down. Sending notifications to connected users...\n");

    // Lock for thread-safe access to resources
    pthread_mutex_lock(&rw_lock);

    // Notify all connected clients
    struct node *temp = head;
    while (temp != NULL) {
        if (temp->socket != -1) { // Check if it's a valid client socket
            char shutdown_message[] = "Server is shutting down. Goodbye!\n";
            send(temp->socket, shutdown_message, strlen(shutdown_message), 0); // Notify client
            close(temp->socket); // Close client socket
        }
        temp = temp->next;
    }

    // Free all nodes in the linked list
    temp = head;
    while (temp != NULL) {
        struct node *next_node = temp->next;
        free(temp); // Free the memory
        temp = next_node;
    }
    head = NULL;

    pthread_mutex_unlock(&rw_lock);

    // Close the server socket
    if (chat_serv_sock_fd != -1) {
        close(chat_serv_sock_fd);
        printf("Server socket closed.\n");
    }

    printf("Server shut down gracefully.\n");
    exit(0);
}
