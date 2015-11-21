#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config_reader.h"
#include <unistd.h>
#include <string.h>

#define DEBUG 0
#define CONFIG_PATH "nanny.config"

/* Takes absolute path to config file. Returns config struct */
struct Config read_config(char *path) {
    // Write the string to a file
    FILE *f;
    f = fopen(CONFIG_PATH, "w+");
    if (f == NULL) {
        perror("Could not open file to write config to.\n");
        exit(0);
    }
    fputs(path, f);
    fclose(f);

    // Open the file
    // File reading code taken from http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
    // September 21, 2015
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    struct Config config;

    // Open the file.
    fp = fopen(CONFIG_PATH, "r");
    if (fp == NULL) {
        perror("Error when opening config file");
        exit(EXIT_FAILURE);
    }
    // Read and parse programs to be monitored.
    int i = 0;
    for (; i < MAX_CONFIG_PROGRAMS + 1; i++) {
        read = getline(&line, &len, fp);
        if (0) {
            printf("Read line %s\n", line);
        }
        // Ensure config file has at least one program to read.
        if (read == -1 && i == 0) {
            perror("No programs to monitor found in the config file. Procnanny will not run\n");
            exit(EXIT_FAILURE);
        }

        //  Ensure we aren't at the end of the config file.
        if (read == -1) {
            break;
        }

        // Split string on whitespace.
        char *pch;
        // Get application name.
        pch = strtok(line, " ");
        strcpy(config.application_names[i], pch);
        if (DEBUG) {
            printf("Program name read in config %s\n", config.application_names[i]);
        }

        // Get timeout time.
        pch = strtok(NULL, " ");
        config.application_timeout[i] = atoi(pch);
        if (DEBUG) {
            printf("Program timeout read in config %d\n", config.application_timeout[i]);
        }
    }

    // Set total application count.
    config.application_count = i;
    if (DEBUG) {
        printf("Read a total of %d programs\n", i);
    }
    fclose(fp);
    return config;
};

