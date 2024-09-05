#include "loglib.h"
#include<stdio.h>
#include<time.h>
#include<string.h>

void log_message(const char *log_level, const char *message) {
    FILE *log_file = fopen("server.log", "a");
    if (log_file == NULL) {
        printf("Error opening log file!\n");
        return;
    }

    time_t now;
    time(&now);
    char time_str[26];
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline character

    fprintf(log_file, "[%s] %s: %s\n", time_str, log_level, message);
    fclose(log_file);
}