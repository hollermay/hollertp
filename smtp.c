#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>  // For isspace()
#include "loglib.h"
#pragma comment(lib, "Ws2_32.lib")

// Ensure Windows compatibility
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define BUFFER_SIZE 1024
#define EMAIL_DATA_SIZE 10240

void handle_client(SOCKET client_socket);
void send_response(SOCKET client_socket, const char *response);
void send_email(SOCKET client_socket, const char *from, const char *to, const char *data);
void cleanup_and_exit(SOCKET server_fd, int error_code);
void trim_string(char *str);

int main() {
    WSADATA wsaData;
    SOCKET server_fd = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        log_message("ERROR", "WSAStartup failed");
        return 1;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        log_message("ERROR", "Socket creation failed");
        WSACleanup();
        return 1;
    }

    // Set socket to reuse address to prevent "address already in use" errors
    BOOL opt = TRUE;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("setsockopt failed: %d\n", WSAGetLastError());
        log_message("ERROR", "setsockopt failed");
        cleanup_and_exit(server_fd, 1);
    }

    // Set up address structure
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(25);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        log_message("ERROR", "Bind failed");
        cleanup_and_exit(server_fd, 1);
    }

    // Listen for connections
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        log_message("ERROR", "Listen failed");
        cleanup_and_exit(server_fd, 1);
    }

    printf("SMTP Server listening on port 25...\n");
    log_message("INFO", "SMTP Server started and listening on port 25");

    // Accept and handle clients
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            log_message("ERROR", "Accept failed");
            continue;  // Continue to accept other connections
        }

        // Get client's IP address
        char *client_ip = inet_ntoa(address.sin_addr);
        printf("Connection accepted from %s\n", client_ip);
        
        char log_buffer[100];
        snprintf(log_buffer, sizeof(log_buffer), "Connection accepted from %s", client_ip);
        log_message("INFO", log_buffer);

        handle_client(client_socket);
        closesocket(client_socket);
    }

    // This point should never be reached in normal operation
    cleanup_and_exit(server_fd, 0);
    return 0;
}

// Helper function to trim whitespace from beginning and end of string
void trim_string(char *str) {
    if (!str) return;
    
    // Trim leading spaces
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    
    if (*start == 0) {  // All spaces
        *str = 0;
        return;
    }
    
    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    // Null terminate
    *(end + 1) = 0;
    
    // If there were leading spaces, shift the string
    if (start > str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Extract the email address from different formats of MAIL FROM or RCPT TO
void extract_email(const char *command, char *email_buffer, size_t buffer_size) {
    memset(email_buffer, 0, buffer_size);
    
    // Look for email between < and >
    const char *start = strchr(command, '<');
    if (start) {
        start++; // Move past '<'
        const char *end = strchr(start, '>');
        if (end) {
            size_t length = end - start;
            if (length < buffer_size) {
                strncpy(email_buffer, start, length);
                email_buffer[length] = '\0';
                return;
            }
        }
    }
    
    // If no angle brackets or invalid format, try to extract what looks like an email
    // Find first non-space after "MAIL FROM:" or "RCPT TO:"
    const char *colon = strchr(command, ':');
    if (colon) {
        const char *potential_email = colon + 1;
        while (*potential_email && isspace((unsigned char)*potential_email)) {
            potential_email++;
        }
        
        // Copy what's left (which should be the email or at least part of it)
        if (*potential_email) {
            strncpy(email_buffer, potential_email, buffer_size - 1);
            trim_string(email_buffer);
            return;
        }
    }
    
    // If all else fails, just copy the whole command as is
    strncpy(email_buffer, command, buffer_size - 1);
    trim_string(email_buffer);
}

void cleanup_and_exit(SOCKET server_fd, int error_code) {
    if (server_fd != INVALID_SOCKET) {
        closesocket(server_fd);
    }
    WSACleanup();
    exit(error_code);
}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    char command[BUFFER_SIZE] = {0};  // Buffer to accumulate the command
    int command_len = 0;
    const char *greeting = "220 Hollertp Service Ready\r\n";
    const char *ok_response = "250 OK\r\n";
    const char *data_end_response = "354 End data with <CR><LF>.<CR><LF>\r\n";
    const char *message_received = "250 Message accepted for delivery\r\n";
    const char *bye_response = "221 Bye\r\n";
    const char *syntax_help = "501 Syntax: MAIL FROM:<address>\r\n";
    const char *help_message = "214 SMTP commands supported:\r\n"
                              "    HELO/EHLO domain - Identify yourself\r\n"
                              "    MAIL FROM:<address> - Start mail transaction\r\n"
                              "    RCPT TO:<address> - Add recipient\r\n"
                              "    DATA - Start mail body\r\n"
                              "    RSET - Reset mail transaction\r\n"
                              "    NOOP - No operation\r\n"
                              "    QUIT - Close connection\r\n"
                              "    HELP - This message\r\n";
    const char *heading = 
    "********************************************************************************\r\n"
    "*                     H O L L E R T P   S M T P   1.0                          *\r\n"
    "*                     =============================                             *\r\n"
    " __    __   ______   __        __        ________  _______   ________  _______  \r\n"
    "/  |  /  | /      \\ /  |      /  |      /        |/       \\ /        |/       \\ \r\n"
    "$$ |  $$ |/$$$$$$  |$$ |      $$ |      $$$$$$$$/ $$$$$$$  |$$$$$$$$/ $$$$$$$  |\r\n"
    "$$ |__$$ |$$ |  $$ |$$ |      $$ |      $$ |__    $$ |__$$ |   $$ |   $$ |__$$ |\r\n"
    "$$    $$ |$$ |  $$ |$$ |      $$ |      $$    |   $$    $$<    $$ |   $$    $$/ \r\n"
    "$$$$$$$$ |$$ |  $$ |$$ |      $$ |      $$$$$/    $$$$$$$  |   $$ |   $$$$$$$/  \r\n"
    "$$ |  $$ |$$ \\__$$ |$$ |_____ $$ |_____ $$ |_____ $$ |  $$ |   $$ |   $$ |      \r\n"
    "$$ |  $$ |$$    $$/ $$       |$$       |$$       |$$ |  $$ |   $$ |   $$ |      \r\n"
    "$$/   $$/  $$$$$$/  $$$$$$$$/ $$$$$$$$/ $$$$$$$$/ $$/   $$/    $$/    $$/       \r\n"
    "*                                                                               *\r\n"
    "*********************************************************************************\r\n"
    "*                                                                               *\r\n"
    "*       HOLLERTP MAIL SYSTEM V0.1.0 - ALPHA RELEASE - AUGUST 2024               *\r\n"
    "*  CURRENTLY SUPPORTED ON WINDOWS(WE BLEW UP THE LINUX A FEW DAYS AGO)          *\r\n"
    "* CLASSIC MAIL DELIVERY SERVICE OVER TCP/IP - TELNET COMPATIBLE                 *\r\n"
    "* BATCH PROCESSING OF UP TO 64K MESSAGES - REQUIRES 16KB FREE MEMORY            *\r\n"
    "*                                                                               *\r\n"
    "* SYSTEM: WIN-11 RECOMMENDED (NO LINUX!)                                        *\r\n"
    "* AUTHOR: UDAYAN SHARMA (@HOLLERMAY)                                            *\r\n"
    "*                                                                               *\r\n"
    "* TYPE 'HELP' FOR COMMAND LIST                                                  *\r\n"
    "* ©2025 HOLLERMAY  - 'BUILDING MAINFRAMES TO LEARN THE NEW ONES'                *\r\n"
    "*                                                                               *\r\n"
    "*********************************************************************************\r\n\r\n";


    // Variables to store email details
    char mail_from[BUFFER_SIZE] = {0};
    char rcpt_to[BUFFER_SIZE] = {0};
    char email_data[EMAIL_DATA_SIZE] = {0};  // Increased buffer size

    // Send greeting message
    send_response(client_socket, greeting);
    send_response(client_socket, heading);

    // Start a timeout for client inactivity
    DWORD timeout = 300000;  // 5 minutes in milliseconds
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        log_message("ERROR", "Failed to set socket timeout");
    }

    // Receive and handle data from client
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data

        // Append received data to the command buffer safely
        if (command_len + bytes_received < BUFFER_SIZE) {
            strncat(command, buffer, BUFFER_SIZE - command_len - 1);
            command_len += bytes_received;
        } else {
            log_message("ERROR", "Command buffer overflow detected");
            send_response(client_socket, "500 Line too long\r\n");
            command_len = 0;
            memset(command, 0, BUFFER_SIZE);
            continue;
        }

        // Check if we have a complete command
        if (strstr(command, "\r\n") != NULL) {
            // Remove trailing CRLF
            char *crlf = strstr(command, "\r\n");
            if (crlf) *crlf = '\0';
            
            printf("Received: %s\r\n", command);
            log_message("INFO", command);

            // Check for different SMTP commands - making comparisons case-insensitive
            if (strncasecmp(command, "HELO", 4) == 0 || strncasecmp(command, "EHLO", 4) == 0) {
                send_response(client_socket, "250 Hello, pleased to meet you\r\n");
                log_message("INFO", "Client greeted");
            } else if (strncasecmp(command, "MAIL FROM", 9) == 0) {
                extract_email(command, mail_from, BUFFER_SIZE);
                
                if (strlen(mail_from) > 0) {
                    send_response(client_socket, ok_response);
                    
                    char log_buffer[BUFFER_SIZE + 20];
                    snprintf(log_buffer, sizeof(log_buffer), "Sender email set: %s", mail_from);
                    log_message("INFO", log_buffer);
                } else {
                    send_response(client_socket, syntax_help);
                    log_message("ERROR", "Invalid MAIL FROM format");
                }
            } else if (strncasecmp(command, "RCPT TO", 7) == 0) {
                extract_email(command, rcpt_to, BUFFER_SIZE);
                
                if (strlen(rcpt_to) > 0) {
                    send_response(client_socket, ok_response);
                    
                    char log_buffer[BUFFER_SIZE + 20];
                    snprintf(log_buffer, sizeof(log_buffer), "Recipient email set: %s", rcpt_to);
                    log_message("INFO", log_buffer);
                } else {
                    send_response(client_socket, "501 Syntax: RCPT TO:<address>\r\n");
                    log_message("ERROR", "Invalid RCPT TO format");
                }
            } else if (strncasecmp(command, "DATA", 4) == 0) {
                if (mail_from[0] == '\0' || rcpt_to[0] == '\0') {
                    send_response(client_socket, "503 Send MAIL FROM and RCPT TO first\r\n");
                    log_message("ERROR", "DATA command received before MAIL FROM or RCPT TO");
                } else {
                    send_response(client_socket, data_end_response);
                    log_message("INFO", "Receiving email data");
                    
                    int data_len = 0;
                    memset(email_data, 0, EMAIL_DATA_SIZE);
                    
                    // Handle message data
                    int end_of_data = 0;
                    while (!end_of_data && (bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
                        buffer[bytes_received] = '\0';
                        
                        // Check for end of data marker (a line with just a period)
                        if (strcmp(buffer, ".") == 0) {
                            end_of_data = 1;
                        } else {
                            // Check if we have space in the email_data buffer
                            if (data_len + bytes_received < EMAIL_DATA_SIZE) {
                                strncat(email_data, buffer, EMAIL_DATA_SIZE - data_len - 1);
                                data_len += bytes_received;
                            } else {
                                log_message("ERROR", "Email data buffer overflow");
                                send_response(client_socket, "552 Exceeded storage allocation\r\n");
                                end_of_data = 1;
                                continue;
                            }
                        }
                    }
                    
                    if (end_of_data) {
                        // Send the email after receiving the data
                        log_message("INFO", "Processing email data");
                        send_email(client_socket, mail_from, rcpt_to, email_data);
                        send_response(client_socket, message_received);
                    } else {
                        log_message("ERROR", "Data reception error");
                    }
                }
            } else if (strncasecmp(command, "QUIT", 4) == 0) {
                send_response(client_socket, bye_response);
                log_message("INFO", "Client disconnected");
                break;
            } else if (strncasecmp(command, "RSET", 4) == 0) {
                // Reset the mail transaction
                memset(mail_from, 0, BUFFER_SIZE);
                memset(rcpt_to, 0, BUFFER_SIZE);
                memset(email_data, 0, EMAIL_DATA_SIZE);
                send_response(client_socket, ok_response);
                log_message("INFO", "Mail transaction reset");
            } else if (strncasecmp(command, "NOOP", 4) == 0) {
                send_response(client_socket, ok_response);
                log_message("INFO", "NOOP command received");
            } else if (strncasecmp(command, "HELP", 4) == 0) {
                send_response(client_socket, help_message);
                log_message("INFO", "HELP command received");
            } else {
                send_response(client_socket, "502 Command not implemented\r\n");
                log_message("ERROR", "Unrecognized command");
            }

            // Reset command buffer
            memset(command, 0, BUFFER_SIZE);
            command_len = 0;
        }
    }

    if (bytes_received == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            log_message("INFO", "Client connection timed out");
        } else {
            printf("Receive failed: %d\n", error);
            log_message("ERROR", "Receive operation failed");
        }
    }
}

void send_response(SOCKET client_socket, const char *response) {
    int len = strlen(response);
    int sent = 0;
    while (sent < len) {
        int result = send(client_socket, response + sent, len - sent, 0);
        if (result == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
            log_message("ERROR", "Send operation failed");
            break;
        }
        sent += result;
    }
    printf("Sent: %s", response);
}

void send_email(SOCKET client_socket, const char *from, const char *to, const char *data) {
    // Log the email sending attempt
    char log_buffer[BUFFER_SIZE];
    snprintf(log_buffer, sizeof(log_buffer), "Sending email from: %s to: %s", from, to);
    log_message("INFO", log_buffer);
    
    // In a real implementation, this would connect to a mail server or save to disk
    // For now, just log that we would send the email
    log_message("INFO", "Email content processed successfully");
    
    // For demonstration, construct a response to the client
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), 
             "Email successfully processed:\r\nFrom: %s\r\nTo: %s\r\n", 
             from, to);
    
    send_response(client_socket, response);
}

// Compilation command:
// gcc smtp.c loglib.c -o smtp.exe -lws2_32