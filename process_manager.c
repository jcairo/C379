#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process_manager.h"
#include "config_reader.h"

#define DEBUG 0
#define MAX_CHARS 1000

/* Iterates through each program to be monitored, calls get pids by name for each
 * and builds a process_group struct with all running processes and returns it.
 */
struct Process_Group get_all_processes(struct Config config) {

}


/* Checks whether a passed in process name is running. Passes back pids. */
struct Process_Group get_process_group_by_name(char *process_name) {
    // File reading variables.
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char buffer[MAX_CHARS];

    // Process Group struct to return.
    struct Process_Group process_group;

    // Construct the required command.
    char command_prefix[] = "ps aux | pgrep ";

    // Concat the command components.
    char command[512] = {'\0'};
    strcat(command, command_prefix);
    strcat(command, process_name);
    
    // Run the command.
    printf("%s\n", command);
    fp = popen(command, "r");
    printf("Main program name is %s\n", process_name);  
    if (fp == NULL) {
        printf("Error during popen of ps aux command."); 
        exit(EXIT_FAILURE);
    }

    // Read process ids from the ps aux statement.
    int i = 0;
    for (;;i++) {
        read = getline(&line, &len, fp);
        // If the first read is empty there are no matching processes.
        if (read == -1 && i == 0) {
            perror("First line when reading ps aux output is empty");
            process_group.process_count = 0;
            break;
        }
        
        // End of file reached.
        if (read == -1) {
            break;
        }

        // Try and parse the pid into an integer
        int pid = atoi(line);
        process_group.process[i].process_id = pid;
        strcpy(process_group.process[i].process_name, process_name);
        if (DEBUG) {
            printf("Parsed process id: %d ", process_group.process[i].process_id);
            printf("Process name: %s\n", process_group.process[i].process_name);
        }
        process_group.process[i].process_monitored = 0;
        process_group.process[i].process_monitor_id = -1;
        process_group.process_count = i + 1;
    }

    return process_group;
}



/* Checks whether and existing procnanny process is already running */
int proc_running(char *process_name) {
    // Get group of procnanny processes.
    struct Process_Group process_group = get_process_group_by_name(process_name);
    if (process_group.process_count) {
        // At least one procnanny process is running.
        return 1;
    } else {
        return 0; 
    }
}
