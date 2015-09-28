#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "process_manager.h"
#include "config_reader.h"
#include "logger.h"

#define DEBUG 0
#define MAX_CHARS 1000

/* Takes a process group and kills all processes in the group */
void kill_processes(struct Process_Group process_group) {
    // Find the pid of the current process so we don't kill it.
    int current_pid = getpid();
    int i = 0;
    for (; i < process_group.process_count-1; i++) {
        // Make sure we don't kill the current process.
        if (process_group.process[i].process_id == current_pid) {
            continue; 
        }
       
        // Taken from http://stackoverflow.com/questions/5460702/check-running-processes-in-c
        // September 26, 2015
        // Make sure the processes are still running and kill them.
        if (kill(process_group.process[i].process_id, 0) == 0) {
            // Process is running. Kill it. 
            kill(process_group.process[i].process_id, SIGKILL);
            printf("Killed process number %d\n", process_group.process[i].process_id);
        } else if (errno == ESRCH) {
            // No process is running
            printf("No process with pid %d is running", process_group.process[i].process_id);
        } else {
            // Some other erro
            printf("An error occured when trying to kill pid: %d, %s", process_group.process[i].process_id, strerror(errno));
        }
    } 
}

/* Iterates through each program to be monitored, calls get pids by name for each
 * and builds a process_group struct with all running processes and returns it.
 */
struct Process_Group get_all_processes(struct Config config) {
    struct Process_Group aggregated_process_group;
    aggregated_process_group.process_count = 0;
    
    int i = 0;
    int total_processes = 0;
    // Iterate through each application name to be monitored.
    // For each applictation name get all prcesses of that name
    // place them all into the aggregated process group.
    for (;i < config.application_count; i++) {
        printf("Getting Processes named %s\n", config.application_names[i]);
        struct Process_Group process_group;
        process_group = get_process_group_by_name(config.application_names[i]);

        // Put processes into the aggregated process group.
        int j = 0;
        for (;j < process_group.process_count; j++) {
            aggregated_process_group.process[total_processes] = process_group.process[j];  
            printf("Process added to aggregate group, name: %s, pid %d\n", aggregated_process_group.process[total_processes].process_name, aggregated_process_group.process[total_processes].process_id);
            total_processes++;
        }
    }
    aggregated_process_group.process_count = total_processes;        
    return aggregated_process_group;
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
    fp = popen(command, "r");
    printf("Program name to monitor is %s\n", process_name);  
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
            printf("No programs called %s found\n", process_name);
            process_group.process_count = 0;
            // Output to log file that no process exsists of this name.
            break;
        }
        
        // End of file reached.
        if (read == -1) {
            break;
        }

        // Try and parse the pid into an integer
        pid_t pid = atoi(line);
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
