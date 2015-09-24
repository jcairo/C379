#include <stdio.h>
#include <stdlib.h>
#include "config_reader.h"

void read_config(struct Config *config, char *path) {
    // Open the file
    // File reading code taken from http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
    // September 21, 2015
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    printf("The file path is: %s\n", path);    
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error when opening config file"); 
        exit(EXIT_FAILURE);
    }

    // Read the time delay
    read = getline(&line, &len, fp); 
    if (read == -1) {
        perror("Error reading first line of config file.");
        exit(EXIT_FAILURE);
    }
    int time = atoi(line);
    printf("Time delay is %d", time);
    //config->time = time;
    

    fclose(fp);
};

