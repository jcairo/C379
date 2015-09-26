#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"

#define DEBUG 1
#define BUFFER_LENGTH 256

/* Returns a formatted string */
void get_formatted_time(char *buffer) {
    // Get the time
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, BUFFER_LENGTH, "[%a %b %d %X %Z %Y] ", info);
}

/* Takes a string and logs it to the log file with appropriate formatting */
void log_message(char *message) {
    // Get log file path
    const char *path = getenv("PROCNANNYLOGS");
    if (path == NULL) {
        printf("Error when reading path to procnanny log file.");
        exit(EXIT_FAILURE);
    }

    // Determine whether we have a relative or absolute path.
    if (path[0] == '/') {
        // We have a relative path. 
    }

    char formatted_time[BUFFER_LENGTH];
    get_formatted_time(formatted_time);
    
    // Open the file to write to.
    FILE *fp;
    fp = fopen(path, "a");
    fprintf(fp, "%s%s\n", formatted_time, message);
    fclose(fp);
}
