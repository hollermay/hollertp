#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_SIZE 1024

void handle_client(SOCKET client_socket);
void send_response(SOCKET client_socket, const char *response);
void send_email(const char *from, const char *to, const char *data);


int main() {
    WSADATA wsaData;
    SOCKET server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(25);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("SMTP Server listening on port 25...\n");

    // Accept and handle clients
    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) != INVALID_SOCKET) {
        printf("Connection accepted from client...\n");
        handle_client(client_socket);
        closesocket(client_socket);
    }

    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed: %d\n", WSAGetLastError());
    }

    // Close server socket
    closesocket(server_fd);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    char command[BUFFER_SIZE] = {0};  // Buffer to accumulate the command
    int command_len = 0;
    const char *greeting = "220 MySMTPServer Service Ready\r\n";
    const char *ok_response = "250 OK\r\n";
    const char *data_end_response = "354 End data with <CR><LF>.<CR><LF>\r\n";
    const char *message_received = "250 Message accepted for delivery\r\n";
    const char *bye_response = "221 Bye\r\n";

    // Variables to store email details
    char mail_from[BUFFER_SIZE] = {0};
    char rcpt_to[BUFFER_SIZE] = {0};
    char email_data[BUFFER_SIZE * 10] = {0};

    // Send greeting message
    send_response(client_socket, greeting);

    // Receive and handle data from client
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data

        // Append received data to the command buffer
        strncat(command, buffer, sizeof(command) - command_len - 1);
        command_len += bytes_received;

        // Check if we have a complete command
        if (strstr(command, "\r\n") != NULL) {
            printf("Received: %s", command);

            // Check for different SMTP commands
            if (strncmp(command, "HELO", 4) == 0 || strncmp(command, "EHLO", 4) == 0) {
                send_response(client_socket, "250 Hello localhost, pleased to meet you\r\n");
            } else if (strncmp(command, "MAIL FROM:", 11) == 0) {
                sscanf(command, "MAIL FROM:<%1023[^>]>%*s", mail_from);
                send_response(client_socket, ok_response);
            } else if (strncmp(command, "RCPT TO:", 9) == 0) {
                sscanf(command, "RCPT TO:<%1023[^>]>%*s", rcpt_to);
                send_response(client_socket, ok_response);
            } else if (strncmp(command, "DATA", 4) == 0) {
                send_response(client_socket, data_end_response);
                int data_len = 0;
                // Handle message data
                while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
                    buffer[bytes_received] = '\0';
                    // Check for end of data
                    if (strncmp(buffer, ".\r\n", 3) == 0) {
                        send_response(client_socket, message_received);
                        break;
                    }
                    strncat(email_data, buffer, sizeof(email_data) - data_len - 1);
                    data_len += bytes_received;
                }
                // Send the email after receiving the data
                send_email(mail_from, rcpt_to, email_data);
            } else if (strncmp(command, "QUIT", 4) == 0) {
                send_response(client_socket, bye_response);
                break;
            } else {
                send_response(client_socket, "502 Command not implemented\r\n");
            }

            // Reset command buffer
            memset(command, 0, sizeof(command));
            command_len = 0;
        }
    }

    if (bytes_received == SOCKET_ERROR) {
        printf("Receive failed: %d\n", WSAGetLastError());
    }
}

void send_response(SOCKET client_socket, const char *response) {
    int len = strlen(response);
    int sent = 0;
    while (sent < len) {
        int result = send(client_socket, response + sent, len - sent, 0);
        if (result == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
            break;
        }
        sent += result;
    }
    printf("Sent: %s", response);
}

void send_email(const char *from, const char *to, const char *data) {
    printf("Sending email...\n");
    printf("From: %s\n", from);
    printf("To: %s\n", to);
    printf("Data: \n%s\n", data);
}
//keeping this command handy for the moment for compilation
//gcc smtp.c -o smtp.exe -lws2_32
