#include <stdio.h>
#include <stdlib.h>
#include "config_reader.h"

/* Takes absolute path to config file. Returns config struct */
struct Config read_config(char *path) {
    // Open the file
    // File reading code taken from http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
    // September 21, 2015
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    struct Config config;

    // Open the file.
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error when opening config file"); 
        exit(EXIT_FAILURE);
    }

    // Read the time delay.
    read = getline(&line, &len, fp); 
    if (read == -1) {
        perror("Error reading first line of config file.");
        exit(EXIT_FAILURE);
    }

    // Parse time delay.
    int time = atoi(line);
    printf("Time delay is %d\n", time);
    config.time = time;

    // Read and parse programs to be monitored.
    int i = 0;
    for (; i < MAX_CONFIG_PROGRAMS; i++) {
        read = getline(&line, &len, fp);
        if (read == -1) {
            perror("Error while reading program list in config file or end of lines reached.");
            break;
            exit(EXIT_FAILURE);
        }
        config.application_names[i] = line;
        // config.application_names[i][1] = "a";
        printf("Character in string at position %ld is %s\n", len-1, &config.application_names[i][1]);
        printf("Read program name from file: %s\n", *config.application_names[i][1]);
    }

    fclose(fp);
    return config;
};

