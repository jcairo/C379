#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "user_interaction.h"
#include "process_manager.h"
#include "config_reader.h"
#include "logger.h"
#include "memwatch.h"


#define DEBUG 0

char main_program_name[] = "procnanny";


int main(int argc, char *argv[]) {
    // Parse config file
    struct Config config = read_config(argv[1]);
    // Check if procnanny process is running and prompt user to kill.
    struct Process_Group procnanny_process_group = get_process_group_by_name(main_program_name);
    char *main_log_file_path = getenv("PROCNANNYLOGS");
    if (main_log_file_path == NULL) {
        printf("Error when reading PROCNANNYLOGS variable.\n");
        exit(EXIT_FAILURE);
    }

    if (procnanny_process_group.process_count > 1) {
        // Prompt user to quit existing process.
        int kill = prompt_user_for_instructions();        
        if (kill) {
            // Kill the existing procnanny processes.
            kill_processes(procnanny_process_group, config, main_log_file_path);
        } else {
            // Exit this instance of procnanny and let existing instance continue.
            exit(0);
        }
    }
    clear_log_file();

    // If we got here we are starting the monitoring process with procnanny.
    // Start by getting all the processes requested to be monitored.
    struct Process_Group process_group = get_all_processes(config);
    
    // Fork all child processes.
    int i = 0;
    pid_t child_process_pid;
    struct Process target;
    struct Process_Group target_group;
    for (;i < process_group.process_count; i++) {

        // Set up the target process struct and process group struct to be passed to child.
        target = process_group.process[i];
        target_group.process[0] = target;
        target_group.process_count = 1;
        sleep(1);

        if ((child_process_pid = fork()) < 0) {
            printf("Error forking process. Exiting...\n");
            exit(EXIT_FAILURE);

        } else if (child_process_pid == 0) { // Child process after fork.
            
            sleep(config.time);
            char process_log_file_path[256];
            pid_t child_process_pid = getpid();    
            sprintf(process_log_file_path, "%s%d", ".", child_process_pid); 
            kill_processes(target_group, config, process_log_file_path);
            exit(0); 

        }  else { // Parent process after the fork. 
            // Record monitoring process
            char message[512];
            sprintf(message, "Initializing monitoring of process '%s' (PID %d).", process_group.process[i].process_name, process_group.process[i].process_id);

            log_message(message, INFO, main_log_file_path);
            process_group.process[i].process_monitor_id = child_process_pid;
            process_group.process[i].process_monitored = 1;
            char process_log_file_path[256];
            sprintf(process_log_file_path, "%s%d", ".", child_process_pid);
            strcpy(process_group.process[i].process_log_file_path, process_log_file_path);
        }
    }

    fflush(stdout);

    // Wait until the time specified has elapsed, then aggregate info from all
    // killer child processes and place info into the main user specified log file.
    sleep(config.time + 1);
    aggregate_log_files(process_group, main_log_file_path);
    int processes_killed = get_total_processes_killed();
    char terminal_message[512];
    sprintf(terminal_message, "Exiting. %d process(es) killed.", processes_killed);
    log_message(terminal_message, INFO, main_log_file_path);

    return 0;
}

