#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include "user_interaction.h"
#include "process_manager.h"
#include "config_reader.h"
#include "logger.h"
#include "memwatch.h"

#define DEBUG 0

int reread_config = 0;
int kill_program = 0;

char main_program_name[] = "procnanny";

void sigint_handler(int signo) {
    if (signo == SIGINT) {
        kill_program = 1;
        printf("Caught SIGINT... Leaving program\n");
    }
}

void sighup_handler(int signo) {
    if (signo == SIGHUP) {
        printf("Caught SIGHUP\n");
        reread_config = 1;
    }
}

int main(int argc, char *argv[]) {
    // Setup signal handler for SIGINT/Kill
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        printf("Can't catch SIGINT... Exiting\n");
        exit(EXIT_FAILURE);
    }

    // Setup signal handler for SIGHUP/Config reread
    if (signal(SIGHUP, sighup_handler) == SIG_ERR) {
        printf("Cant catch SIGHUP... Exiting\n");
        exit(EXIT_FAILURE);
    }

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

        // Create pipe to child
        pipe(process_group.process[i].fd);

        if ((child_process_pid = fork()) < 0) {
            printf("Error forking process. Exiting...\n");
            exit(EXIT_FAILURE);

        } else if (child_process_pid == 0) { // Child process after fork.

            // sleep(config.time);
            char process_log_file_path[256];
            pid_t child_process_pid = getpid();
            sprintf(process_log_file_path, "%s%d", ".", child_process_pid);
            kill_processes(target_group, config, process_log_file_path);

            // Write to pipe to tell parent child process killed
            // test
            char message[] = "Killed";
            write(process_group.process[i].fd[1], message, sizeof(message));

            while (1) {
                // Setup reqs for reading from pipe
                char readbuffer[512];
                int readbytes;
                int pid_to_kill;
                int time_to_kill;
                char *pch;

                // If read pipe returns value
                readbytes = read(process_group.process[i].fd[0], readbuffer, sizeof(readbuffer));
                if (readbytes) {
                    // Pipe should send pid of process to kill followed by time to kill
                    pch = strtok(readbuffer, " ");
                    printf("Child Process pid to kill %s\n", pch);
                    pid_to_kill = atoi(pch);
                    pch = strtok(readbuffer, " ");
                    printf("Child Process time limit to kill %s\n", pch);
                    time_to_kill = atoi(pch);
                    sleep(time_to_kill);
                    kill_process(pid_to_kill);
                    write(process_group.process[i].fd[1], message, sizeof(message));
                }
            }

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
    int last_time_checked = (int)time(NULL);

    // Main program loop
    while (1) {
        // Read from all pipes to see if any processes have been killed

        // Check if config should be reread
        if (reread_config) {
            // Do some stuff.
        }

        // Check if program should be sut down
        if (kill_program) {
            exit(0);
        }

        // Check whether its been 5 seconds since scanning active programs.
        if (((int)time(NULL) - last_time_checked) > 4) {
            // Do some stuff.

            last_time_checked = (int)time(NULL);
        }
    }

/* OLD LOGGING SYSTEM
    // Wait until the time specified has elapsed, then aggregate info from all
    // killer child processes and place info into the main user specified log file.
    sleep(config.time + 1);
    aggregate_log_files(process_group, main_log_file_path);
    int processes_killed = get_total_processes_killed();
    char terminal_message[512];
    sprintf(terminal_message, "Exiting. %d process(es) killed.", processes_killed);
    log_message(terminal_message, INFO, main_log_file_path);
    return 0;
*/
}

