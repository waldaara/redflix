#include <arpa/inet.h>
#include <bits/getopt_core.h>
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

// Function to handle video reception and display
void receive_video(int server_socket, int bitrate) {
  char video_frame;
  int frames_to_receive = 0;

  // Determine the number of frames to receive based on the bitrate
  switch (bitrate) {
    case LOW_DEFINITION:
      frames_to_receive = 10;
      break;
    case MEDIUM_DEFINITION:
      frames_to_receive = 100;
      break;
    case HIGH_DEFINITION:
      frames_to_receive = 1000;
      break;
    default:
      frames_to_receive = 10;
      break;
  }

  // Receive and display video frames
  for (int i = 0; i < frames_to_receive; ++i) {
    if (recv(server_socket, &video_frame, 1, 0) <= 0) {
      perror("Error receiving data");
      break;
    }
    printf("Received frame: %c\n",
           video_frame);  // Display the frame (could be processed differently)
    sleep(0.5);           // Simulate video playback delay
  }

  printf("Finished receiving frames.\n");
}

// Function to initialize the client and connect to the server
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

// Main entry point for the client
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
          fprintf(stderr, "Error: Port must be a positive integer.\n");
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

  // Connect to the server
  int client_socket = connect_to_server(server_ip, server_port);

  // Send the selected bitrate to the server
  if (send(client_socket, &bitrate, sizeof(bitrate), 0) == -1) {
    perror("Error sending bitrate to server");
    close(client_socket);
    return EXIT_FAILURE;
  }

  // Receive video frames from the server
  receive_video(client_socket, bitrate);

  // Close the connection
  close(client_socket);

  return EXIT_SUCCESS;
}
