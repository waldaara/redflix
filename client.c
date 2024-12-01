#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080
#define BUFFER_SIZE 1024

#define LOW_DEFINITION 1
#define MEDIUM_DEFINITION 2
#define HIGH_DEFINITION 3

typedef struct {
  int client_socket;
} client_data_t;

void *handle_user_actions(void *arg) {
  client_data_t *client_data = (client_data_t *)arg;

  if (client_data == NULL) {
    fprintf(stderr, "Error: Invalid client data\n");

    exit(EXIT_FAILURE);
  }

  int client_socket = client_data->client_socket;
  char buffer[BUFFER_SIZE];

  while (1) {
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;  // Remove the line break

    // Check for empty input
    while (strlen(buffer) == 0) {
      fgets(buffer, sizeof(buffer), stdin);
      buffer[strcspn(buffer, "\n")] = 0;  // Remove the line break
    }

    if (strcasecmp(buffer, "exit") == 0) {
      close(client_socket);
      exit(EXIT_SUCCESS);
    }

    if (send(client_socket, &buffer, sizeof(buffer), 0) == -1) {
      perror("Error sending data to server");
      close(client_socket);
      exit(EXIT_FAILURE);
    }
  }
}

void *handle_streaming(void *arg) {
  client_data_t *client_data = (client_data_t *)arg;

  if (client_data == NULL) {
    fprintf(stderr, "Error: Invalid client data\n");

    exit(EXIT_FAILURE);
  }

  int client_socket = client_data->client_socket;
  char buffer[BUFFER_SIZE];

  printf("Client socket %d\n", client_socket);

  while (1) {
    int bytes_read = recv(client_socket, &buffer, sizeof(buffer), 0);

    if (bytes_read == -1) {
      perror("Error receiving data");
      continue;
    }

    if (bytes_read == 0) {
      printf("Finished receiving frames.\n");

      close(client_socket);
      exit(EXIT_SUCCESS);
    }

    printf("\n%s\n", buffer);

    memset(buffer, 0, sizeof(buffer));
  }

  return NULL;
}

int connect_to_server(const char *server_ip, int server_port) {
  int client_socket;
  struct sockaddr_in server_addr;

  // Create the socket
  client_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (client_socket < 0) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  // Configure server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    perror("Invalid server IP address");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // Connect to the server
  if (connect(client_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  return client_socket;
}

int main(int argc, char *argv[]) {
  const char *server_ip = DEFAULT_SERVER_IP;
  int server_port = DEFAULT_SERVER_PORT;
  int bitrate = LOW_DEFINITION;  // Default bitrate (Low Definition)
  int opt;

  // Parse command-line options
  while ((opt = getopt(argc, argv, "i:p:b:")) != -1) {
    switch (opt) {
      case 'i':  // Server IP
        server_ip = optarg;
        break;

      case 'p':  // Server port
        server_port = atoi(optarg);

        if (server_port <= 0) {
          fprintf(stderr, "Error: Port must be a non-negative integer.\n");

          return EXIT_FAILURE;
        }

        break;

      case 'b':  // Bitrate
        bitrate = atoi(optarg);

        if (bitrate < LOW_DEFINITION || bitrate > HIGH_DEFINITION) {
          fprintf(stderr,
                  "Error: Invalid bitrate. Choose between 1 (LD), 2 (MD), or 3 "
                  "(HD).\n");

          return EXIT_FAILURE;
        }

        break;

      default:
        fprintf(stderr, "Usage: %s -i <server_ip> -p <port> -b <bitrate>\n",
                argv[0]);

        return EXIT_FAILURE;
    }
  }

  int client_socket = connect_to_server(server_ip, server_port);

  // Send the selected bitrate to the server
  if (send(client_socket, &bitrate, sizeof(bitrate), 0) == -1) {
    perror("Error sending bitrate to server");
    close(client_socket);

    return EXIT_FAILURE;
  }

  pthread_t user_actions_thread, streaming_thread;

  // Allocate memory for client data
  client_data_t *client_data = malloc(sizeof(client_data_t));

  if (client_data == NULL) {
    perror("Error allocating memory for client data");
    close(client_socket);

    return EXIT_FAILURE;
  }

  client_data->client_socket = client_socket;

  if (pthread_create(&user_actions_thread, NULL, handle_user_actions,
                     client_data) != 0) {
    perror("Error creating thread for user actions");
    close(client_socket);

    return EXIT_FAILURE;
  }

  if (pthread_create(&streaming_thread, NULL, handle_streaming, client_data) !=
      0) {
    perror("Error creating thread for streaming");
    close(client_socket);

    return EXIT_FAILURE;
  }

  // Wait for the threads to finish
  pthread_join(user_actions_thread, NULL);
  pthread_join(streaming_thread, NULL);

  // Clean up
  free(client_data);
  close(client_socket);

  return EXIT_SUCCESS;
}
