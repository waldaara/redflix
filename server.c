#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

#define LOW_DEFINITION 1
#define MEDIUM_DEFINITION 2
#define HIGH_DEFINITION 3

typedef struct {
  int client_socket;
  int bitrate;  // 1: LD, 2: MD, 3: HD
} client_data_t;

const char *video_data = "1234567890123456789012345678901234567890";

// Function to handle client communication
void *handle_client(void *arg) {
  client_data_t *client_data = (client_data_t *)arg;
  if (client_data == NULL) {
    fprintf(stderr, "Error: Invalid client data\n");
    return NULL;
  }

  int client_socket = client_data->client_socket;
  int bitrate = client_data->bitrate;
  free(client_data);

  printf("Connection established with client, bitrate: %d\n", bitrate);

  int frames_to_send = 0;
  switch (bitrate) {
    case LOW_DEFINITION:
      frames_to_send = 10;
      break;
    case MEDIUM_DEFINITION:
      frames_to_send = 100;
      break;
    case HIGH_DEFINITION:
      frames_to_send = 1000;
      break;
    default:
      frames_to_send = 10;
      break;
  }

  // Send video frames to client
  for (int i = 0; i < frames_to_send; ++i) {
    if (send(client_socket, &video_data[i % strlen(video_data)], 1, 0) == -1) {
      perror("Error sending data to client");
      break;
    }
    sleep(0.5);  // Simulate video frame delay
  }

  printf("Finished sending frames to client.\n");
  close(client_socket);
  return NULL;
}

// Function to start the server
void start_server(int port) {
  int server_socket, new_client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Create the server socket
  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error opening socket");
    exit(EXIT_FAILURE);
  }

  // Setup server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  // Bind the socket to the address
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Error binding socket");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Start listening for connections
  if (listen(server_socket, 5) < 0) {
    perror("Error listening on socket");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d...\n", port);

  // Accept and handle incoming client connections
  while (1) {
    new_client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (new_client_socket < 0) {
      perror("Error accepting connection");
      continue;  // Continue listening for other clients
    }

    int bitrate;
    if (recv(new_client_socket, &bitrate, sizeof(bitrate), 0) <= 0) {
      perror("Error receiving bitrate from client");
      close(new_client_socket);
      continue;  // Continue listening for other clients
    }

    // Allocate memory for client data
    client_data_t *client_data = malloc(sizeof(client_data_t));
    if (client_data == NULL) {
      perror("Error allocating memory for client data");
      close(new_client_socket);
      continue;  // Continue listening for other clients
    }

    client_data->client_socket = new_client_socket;
    client_data->bitrate = bitrate;

    // Create a new thread to handle the client
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, handle_client, client_data) != 0) {
      perror("Error creating thread for client");
      free(client_data);  // Free memory if thread creation fails
      close(new_client_socket);
      continue;  // Continue listening for other clients
    }

    // Detach the thread to allow automatic resource cleanup after it finishes
    pthread_detach(client_thread);
  }

  close(server_socket);
}

// Main entry point of the program
int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;
  int opt;

  // Parse command-line options
  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
      case 'p':
        port = atoi(optarg);
        if (port <= 0) {
          fprintf(stderr, "Error: 'p' must be a positive integer.\n");
          return EXIT_FAILURE;
        }
        break;
      default:
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  // Start the server with the chosen port
  start_server(port);
  return EXIT_SUCCESS;
}
