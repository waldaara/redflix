#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

#define LOW_DEFINITION "LD"
#define MEDIUM_DEFINITION "MD"
#define HIGH_DEFINITION "HD"

typedef struct {
  int client_socket;
  char bitrate[3];  // "LD", "MD", or "HD"
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int pause_streaming;  // Flag to control the pause state
  int current_frame;    // Frame counter
} client_data_t;

// Function to handle client communication (streaming)
void *handle_streaming(void *arg) {
  client_data_t *client_data = (client_data_t *)arg;
  if (client_data == NULL) {
    fprintf(stderr, "Error: Invalid client data\n");
    exit(EXIT_FAILURE);
  }

  int client_socket = client_data->client_socket;
  int frames_to_skip = 0;
  FILE *file = fopen("video.txt", "r");
  if (!file) {
    perror("Error opening video file");
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE] = "";
  char temp[32];
  int frames_in_buffer = 0;
  int number;

  while (fscanf(file, "%d", &number) == 1) {
    // Adjust frame skipping based on bitrate
    if (strcmp(client_data->bitrate, LOW_DEFINITION) == 0) {
      frames_to_skip = 100;
    } else if (strcmp(client_data->bitrate, MEDIUM_DEFINITION) == 0) {
      frames_to_skip = 10;
    } else if (strcmp(client_data->bitrate, HIGH_DEFINITION) == 0) {
      frames_to_skip = 1;
    }

    // Skip frames based on bitrate setting
    if (client_data->current_frame % frames_to_skip == 0) {
      snprintf(temp, sizeof(temp), "%d ", number);
      strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
      frames_in_buffer++;
    }

    client_data->current_frame++;  // Increment frame count

    // Send frames when buffer is full
    if (frames_in_buffer == 30) {
      buffer[strlen(buffer)] = '\0';  // Null-terminate the string

      // Check if the stream is paused
      pthread_mutex_lock(&client_data->mutex);
      while (client_data->pause_streaming) {
        pthread_cond_wait(&client_data->cond,
                          &client_data->mutex);  // Wait if paused
      }
      pthread_mutex_unlock(&client_data->mutex);

      // Send the buffer to the client
      if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
        perror("Error sending frames to client");
        break;
      }

      memset(buffer, 0, sizeof(buffer));  // Clear the buffer
      frames_in_buffer = 0;

      usleep(1 * 1000 * 1000);  // Pause for 1 second between sending frames
    }
  }

  // Send any remaining frames in the buffer
  if (frames_in_buffer > 0) {
    buffer[strlen(buffer)] = '\0';
    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
      perror("Error sending last group of frames");
    }
  }

  fclose(file);
  printf("Finished streaming to client %d.\n", client_socket);
  close(client_socket);

  return NULL;
}

// Function to listen for client commands (pause/play)
void *listen_commands(void *arg) {
  client_data_t *client_data = (client_data_t *)arg;
  if (client_data == NULL) {
    fprintf(stderr, "Error: Invalid client data\n");
    exit(EXIT_FAILURE);
  }

  char command[BUFFER_SIZE];

  while (1) {
    int bytes_received =
        recv(client_data->client_socket, command, sizeof(command), 0);
    if (bytes_received <= 0) {
      perror("Error receiving command from client");
      break;
    }

    command[bytes_received] = '\0';  // Null-terminate the command string

    // Handle "pause" and "play" commands
    if (strcasecmp(command, "pause") == 0) {
      pthread_mutex_lock(&client_data->mutex);
      client_data->pause_streaming = 1;  // Pause streaming
      pthread_cond_signal(
          &client_data->cond);  // Wake up the streaming thread if needed
      pthread_mutex_unlock(&client_data->mutex);
      printf("Received 'pause' command.\n");
    } else if (strcasecmp(command, "play") == 0) {
      pthread_mutex_lock(&client_data->mutex);
      client_data->pause_streaming = 0;         // Resume streaming
      pthread_cond_signal(&client_data->cond);  // Wake up the streaming thread
      pthread_mutex_unlock(&client_data->mutex);
      printf("Received 'play' command.\n");
    }
  }

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

    char bitrate[3];
    int bytes_read = recv(new_client_socket, bitrate, sizeof(bitrate), 0);

    if (bytes_read == -1) {
      perror("Error receiving bitrate from client");
      close(new_client_socket);
      continue;
    }

    if (bytes_read == 0) {
      printf("Client %d disconnected", new_client_socket);
      close(new_client_socket);
      continue;
    }

    // Allocate memory for client data
    client_data_t *client_data = malloc(sizeof(client_data_t));
    if (client_data == NULL) {
      perror("Error allocating memory for client data");
      close(new_client_socket);
      continue;  // Continue listening for other clients
    }

    client_data->client_socket = new_client_socket;
    strcpy(client_data->bitrate, bitrate);  // Set the received bitrate
    client_data->current_frame = 0;         // Reset frame counter
    client_data->pause_streaming = 0;       // Initially not paused
    pthread_mutex_init(&client_data->mutex, NULL);
    pthread_cond_init(&client_data->cond, NULL);

    // Create threads for streaming and command listening
    pthread_t streaming_thread, command_thread;

    if (pthread_create(&streaming_thread, NULL, handle_streaming,
                       client_data) != 0) {
      perror("Error creating streaming thread");
      free(client_data);
      close(new_client_socket);
      continue;  // Continue listening for other clients
    }

    if (pthread_create(&command_thread, NULL, listen_commands, client_data) !=
        0) {
      perror("Error creating command listening thread");
      free(client_data);
      close(new_client_socket);
      continue;  // Continue listening for other clients
    }

    // Detach threads to allow automatic resource cleanup after they finish
    pthread_detach(streaming_thread);
    pthread_detach(command_thread);
  }

  close(server_socket);
}

int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;
  int opt;

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

  start_server(port);

  return EXIT_SUCCESS;
}
