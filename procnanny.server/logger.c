#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"

#define DEBUG 0
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

/* Clears the existing log file */
void clear_log_file() {
    // Get log file path
    char *path = getenv("PROCNANNYLOGS");
    if (path == NULL) {
        printf("Error when reading PROCNANNYLOGS variable.\n");
        exit(EXIT_FAILURE);
    }

    // Determine whether we have a relative or absolute path.
    //if (path[0] == '/') {
    //    // We have a relative path.
    //}

    FILE *fp;
    fp = fopen(path, "w");
    fclose(fp);
}


/* Clears the existing server log file */
void clear_server_log_file() {
    // Get log file path
    char *path = getenv("PROCNANNYSERVERINFO");
    if (path == NULL) {
        printf("Error when reading PROCNANNYLOGS variable.\n");
        exit(EXIT_FAILURE);
    }

    // Determine whether we have a relative or absolute path.
    //if (path[0] == '/') {
    //    // We have a relative path.
    //}

    FILE *fp;
    fp = fopen(path, "w");
    fclose(fp);
}

/* Takes a raw message from clients and logs it to log file. */
void log_raw_message(char *message, char *log_file_path) {
    // Open the file to write to.
    FILE *fp;
    fp = fopen(log_file_path, "a");
    fprintf(fp, "%s", message);
    fclose(fp);
}

/* Takes a string and logs it to the log file with appropriate formatting */
void log_message(char *message, int type, char *log_file_path, int print_to_stdout, int print_raw) {
    // Set the message type
    char message_type[BUFFER_LENGTH] = {'\0'};
    if (type == INFO) {
        strcpy(message_type, "Info: ");
    }
    if (type == ACTION){
        strcpy(message_type, "Action: ");
    }
    if (type == NONE) {
        strcpy(message_type, "");
    }

    // Get the time.
    char formatted_time[BUFFER_LENGTH] = {'\0'};
    get_formatted_time(formatted_time);

    // Open the file to write to.
    FILE *fp;
    // This is for the server info it has no time prefixing.
    if (print_raw) {
        fp = fopen(log_file_path, "a");
        fprintf(fp, "%s\n", message);
        fclose(fp);
        return;
    }

    // If not printing raw were just logging info as normal.
    fp = fopen(log_file_path, "a");
    fflush(fp);
    fprintf(fp, "%s %s%s\n", formatted_time, message_type, message);
    fclose(fp);

    // if boolean set also print to stdout.
    if (print_to_stdout) {
        char stdout_message[512] = {'\0'};
        strcat(stdout_message, formatted_time);
        strcat(stdout_message, " ");
        strcat(stdout_message, message);
        printf("%s\n", stdout_message);
        fflush(stdout);
    }

    return;
}

/* Aggregates the logfiles from each killer child process */
void aggregate_log_files(struct Process_Group process_group, char *main_log_file_path) {
    int i = 0;
    for (; i < process_group.process_count; i++) {
        FILE *fp;
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        // Open the file.
        if (DEBUG) {
            printf("Process log file id to be openend is %s\n", process_group.process[i].process_log_file_path);
            printf("Length of the process file sting is %ld\n", strlen(process_group.process[i].process_log_file_path));

        }

        fp = fopen(process_group.process[i].process_log_file_path, "r");
        if (fp == NULL) {
            // No file exists by this name. Meaning the process was killed before
            // the killer child process had a chance to kill it so no log file was made.
            continue;
            // printf("Error when opening process log file.\n");
            // exit(EXIT_FAILURE);
        }

        // Read the log message.
        read = getline(&line, &len, fp);
        fclose(fp);

        if (read == -1) {
            // No output from process. It did not kill its target.
            remove(process_group.process[i].process_log_file_path);
            continue;
        }

        // If there was a line of info in the file print it to the main log file.
        fp = fopen(main_log_file_path, "a");
        if (DEBUG) {
            printf("The line read from process log file was %s\n", line);
        }
        fprintf(fp,"%s", line);
        fclose(fp);

        // Remove the process log file.
        remove(process_group.process[i].process_log_file_path);
    }
}
