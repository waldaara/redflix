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

#define LOW_DEFINITION "LD"
#define MEDIUM_DEFINITION "MD"
#define HIGH_DEFINITION "HD"

typedef struct {
  int client_socket;
  char bitrate[3];  // To store the bitrate text (LD, MD, or HD)
  int stop_thread;  // Flag to indicate when to stop the threads
} client_data_t;

void show_menu(const char *bitrate) {
  printf("\nSelect an action:\n");
  printf("1. Play\n");
  printf("2. Pause\n");
  printf("3. Stop\n");
  printf("4. Set Low Definition (LD)\n");
  printf("5. Set Medium Definition (MD)\n");
  printf("6. Set High Definition (HD)\n");
  printf("Current bitrate: %s\n", bitrate);
}

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

    if (strcasecmp(buffer, "1") == 0) {
      if (send(client_socket, "play", 4, 0) == -1) {
        perror("Error sending play command to server");
      }
    } else if (strcasecmp(buffer, "2") == 0) {
      if (send(client_socket, "pause", 5, 0) == -1) {
        perror("Error sending pause command to server");
      }
    } else if (strcasecmp(buffer, "3") == 0) {
      if (send(client_socket, "stop", 4, 0) == -1) {
        perror("Error sending stop command to server");
      }
      client_data->stop_thread = 1;  // Set the flag to stop both threads
      break;                         // Exit the loop
    } else if (strcasecmp(buffer, "4") == 0) {
      strcpy(client_data->bitrate, LOW_DEFINITION);
      printf("Bitrate set to Low Definition (LD)\n");
      if (send(client_socket, "LD", 2, 0) == -1) {
        perror("Error sending LD to server");
      }
    } else if (strcasecmp(buffer, "5") == 0) {
      strcpy(client_data->bitrate, MEDIUM_DEFINITION);
      printf("Bitrate set to Medium Definition (MD)\n");
      if (send(client_socket, "MD", 2, 0) == -1) {
        perror("Error sending MD to server");
      }
    } else if (strcasecmp(buffer, "6") == 0) {
      strcpy(client_data->bitrate, HIGH_DEFINITION);
      printf("Bitrate set to High Definition (HD)\n");
      if (send(client_socket, "HD", 2, 0) == -1) {
        perror("Error sending HD to server");
      }
    } else {
      printf("Invalid choice. Please choose a valid action.\n");
    }

    memset(buffer, 0, sizeof(buffer));
  }

  close(client_socket);

  return NULL;
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
    // Check if stop flag is set
    if (client_data->stop_thread) {
      printf("Stopping streaming...\n");
      break;
    }

    int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_read == -1) {
      perror("Error receiving data");
      continue;
    }

    if (bytes_read == 0) {
      printf("Finished receiving frames.\n");
      break;  // Exit if the server closes the connection
    }

    printf("\n%s\n", buffer);
    memset(buffer, 0, sizeof(buffer));

    show_menu(client_data->bitrate);
  }

  close(client_socket);

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
  int opt;

  // Default bitrate is Low Definition
  char bitrate[3] = "LD";

  // Parse command-line options
  while ((opt = getopt(argc, argv, "i:p:")) != -1) {
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
      default:
        fprintf(stderr, "Usage: %s -i <server_ip> -p <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  int client_socket = connect_to_server(server_ip, server_port);

  // Send the initial bitrate (LD) to the server
  if (send(client_socket, "LD", 2, 0) == -1) {
    perror("Error sending LD to server");
    close(client_socket);
    return EXIT_FAILURE;
  }

  // Allocate memory for client data
  client_data_t *client_data = malloc(sizeof(client_data_t));
  if (client_data == NULL) {
    perror("Error allocating memory for client data");
    close(client_socket);
    return EXIT_FAILURE;
  }

  client_data->client_socket = client_socket;
  strcpy(client_data->bitrate, bitrate);  // Set initial bitrate to LD
  client_data->stop_thread = 0;           // Initialize stop flag

  pthread_t user_actions_thread, streaming_thread;

  // Create threads for user actions and streaming
  if (pthread_create(&user_actions_thread, NULL, handle_user_actions,
                     client_data) != 0) {
    perror("Error creating thread for user actions");
    close(client_socket);
    free(client_data);
    return EXIT_FAILURE;
  }

  if (pthread_create(&streaming_thread, NULL, handle_streaming, client_data) !=
      0) {
    perror("Error creating thread for streaming");
    close(client_socket);
    free(client_data);
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
