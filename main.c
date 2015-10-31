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

// Stores the program name on the command line.
char main_program_name[] = "procnanny";

// Globals set my signals handlers.
int reread_config = 0;
int kill_program = 0;


void sigint_handler(int signo) {
    if (signo == SIGINT) {
        kill_program = 1;
    }
}

void sighup_handler(int signo) {
    if (signo == SIGHUP) {
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
    struct Process_Group procnanny_process_group = get_process_group_by_name(main_program_name, 0);
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

    if (DEBUG) {
        int k = 0;
        for (;k < process_group.process_count; k++) {
            printf("Process found: %s, time to kill %d, process id is: %d\n", process_group.process[k].process_name, process_group.process[k].time_to_kill, process_group.process[k].process_id);
        }
        printf("Program count %d\n", process_group.process_count);
    }

    // Fork all initial child processes.
    int i = 0;
    pid_t child_process_pid;

    for (;i < process_group.process_count; i++) {

        // Set up the target process struct and process group struct to be passed to child.
        int time_to_kill = process_group.process[i].time_to_kill;
        int process_to_kill = process_group.process[i].process_id;

        // Create pipe to child
        pipe(process_group.process[i].fd);
        if ((child_process_pid = fork()) < 0) {
            printf("Error forking process. Exiting...\n");
            exit(EXIT_FAILURE);

/* CHILD PROCESS */
        } else if (child_process_pid == 0) { // Child process after fork.

            // Perform the initial kill.
            sleep(time_to_kill);
            int message = kill_process(process_to_kill);

            // Write to pipe to tell parent child process killed
            // 1 means killed 0 means not killed
            if (message == 1) { write(process_group.process[i].fd[1], "1", 1);
            } else if (message == 0) { write(process_group.process[i].fd[1], "0", 1);
            } else { printf("The process was not killed returned unexepected result\n"); exit(EXIT_FAILURE);
            }

            while (1) {
                // Setup reqs for reading from pipe
                char readbuffer[512];
                int readbytes;
                char *pch;

                // If read pipe returns value
                readbytes = read(process_group.process[i].fd[0], readbuffer, sizeof(readbuffer));
                if (readbytes) {
                    // Pipe should send pid of process to kill followed by time to kill
                    pch = strtok(readbuffer, " ");
                    printf("Child Process pid to kill %s\n", pch);
                    process_to_kill = atoi(pch);
                    pch = strtok(NULL, " ");
                    printf("Child Process time limit to kill %s\n", pch);
                    time_to_kill = atoi(pch);
                    sleep(time_to_kill);
                    message = kill_process(process_to_kill);

                    // Write to pipe to tell parent child process killed
                    // 1 means killed 0 means not killed
                    if (message == 1) { write(process_group.process[i].fd[1], "1", 1);
                    } else if (message == 0) { write(process_group.process[i].fd[1], "0", 1);
                    } else { printf("The process was not killed returned unexepected result\n"); exit(EXIT_FAILURE);
                    }
                }
            }
/* /CHILD PROCESS */

/* PARENT PROCESS AFTER FORK */
        }  else {
            // Record monitoring process
            char message[512];
            sprintf(message, "Initializing monitoring of process '%s' (PID %d).", process_group.process[i].process_name, process_group.process[i].process_id);
            log_message(message, INFO, main_log_file_path);
            process_group.process[i].process_monitor_id = child_process_pid;
            process_group.process[i].busy = 1;
        }
/* /PARENT PROCESS AFTER FORK */
    }

    fflush(stdout);
    // int last_time_checked = (int)time(NULL);
    int non_busy_monitor_process_found = 0;
    // int time_since_last_check = ((int)time(NULL) - last_time_checked);

    // Main program loop
    while (1) {
        fflush(stdout);
        // Read from all pipes to see if any processes have been killed
        // Make sure to reset all the process_group info for each process.

        // Check if config should be reread or we should rescan processes.
        if (reread_config) {
            // Determine whether we are rescanning programs because of 5 second time lapse or config reread.

            // If we are rereading the config Rearead the config file and replace old version.
            if (reread_config) {
                config = read_config(argv[1]);
            }

            // Get a list of active processes.
            struct Process_Group current_process_group = get_all_processes(config);

            // Iterate through all the current snapshot of processes to see which need monitoring.
            int i = 0;
            for (;i < current_process_group.process_count; i++) {
                non_busy_monitor_process_found = 0;

                // If the process is not currently being monitored, monitor it.
                if (!is_monitored(current_process_group.process[i].process_id, process_group)) {

                    // First check to see if any existing child processes aren't busy
                    int j = 0;
                    for (;j < process_group.process_count; j++) {

                        // If a process is not busy send it a message to monitor this process.
                        if (!process_group.process[j].busy) {
                            non_busy_monitor_process_found = 1;

                            // Convert and concat program pid and time to monitor before sending in pipe.
                            char pid_string[10];
                            char time_to_kill_string[10];
                            char string_to_pipe[20];
                            snprintf(pid_string, 10, "%d", current_process_group.process[i].process_id);
                            snprintf(time_to_kill_string, 10, "%d", current_process_group.process[i].time_to_kill);
                            strcpy(string_to_pipe, pid_string);
                            strcpy(string_to_pipe, " ");
                            strcpy(string_to_pipe, time_to_kill_string);
                            write(process_group.process[j].fd[1], string_to_pipe, sizeof(string_to_pipe));

                            // Update process group information
                            process_group.process[j].time_to_kill = current_process_group.process[i].time_to_kill;
                            process_group.process[j].busy = 1;
                            process_group.process[j].process_id = current_process_group.process[i].process_id;
                            strcpy(process_group.process[j].process_name, current_process_group.process[i].process_name);
                            break;
                        }
                    }

                    // If a process was found to do the monitoring move onto the next process.
                    if (non_busy_monitor_process_found) {
                        continue;
                    }

                    // If we get here we know no free process exists we need to fork a new process.
                    // Set up the target process struct and process group struct to be passed to child.
                    int time_to_kill = current_process_group.process[i].time_to_kill;
                    int process_to_kill = current_process_group.process[i].process_id;
                    int new_process_index = process_group.process_count;
                    process_group.process_count++;

                    // Update main process group information
                    process_group.process[new_process_index].time_to_kill = time_to_kill;
                    process_group.process[new_process_index].busy = 1;
                    process_group.process[new_process_index].process_id = process_to_kill;
                    strcpy(process_group.process[new_process_index].process_name, current_process_group.process[i].process_name);

                    // Create pipe to child
                    pipe(process_group.process[new_process_index].fd);
                    if ((child_process_pid = fork()) < 0) {
                        printf("Error forking process. Exiting...\n");
                        exit(EXIT_FAILURE);

                    /* CHILD PROCESS */
                    } else if (child_process_pid == 0) { // Child process after fork.

                        // Perform the initial kill.
                        sleep(time_to_kill);
                        int message = kill_process(process_to_kill);

                        // Write to pipe to tell parent child process killed
                        // 1 means killed 0 means not killed
                        if (message == 1) { write(process_group.process[new_process_index].fd[1], "1", 1);
                        } else if (message == 0) { write(process_group.process[new_process_index].fd[1], "0", 1);
                        } else { printf("The process was not killed returned unexepected result\n"); exit(EXIT_FAILURE);
                        }

                        while (1) {
                            // Setup reqs for reading from pipe
                            char readbuffer[512];
                            int readbytes;
                            char *pch;

                            // If read pipe returns value
                            readbytes = read(process_group.process[new_process_index].fd[0], readbuffer, sizeof(readbuffer));
                            if (readbytes) {
                                // Pipe should send pid of process to kill followed by time to kill
                                pch = strtok(readbuffer, " ");
                                printf("Child Process pid to kill %s\n", pch);
                                process_to_kill = atoi(pch);
                                pch = strtok(NULL, " ");
                                printf("Child Process time limit to kill %s\n", pch);
                                time_to_kill = atoi(pch);
                                sleep(time_to_kill);
                                message = kill_process(process_to_kill);

                                // Write to pipe to tell parent child process killed
                                // 1 means killed 0 means not killed
                                if (message == 1) { write(process_group.process[new_process_index].fd[1], "1", 1);
                                } else if (message == 0) { write(process_group.process[new_process_index].fd[1], "0", 1);
                                } else { printf("The process was not killed returned unexepected result\n"); exit(EXIT_FAILURE);
                                }
                            }
                        }
                    /* /CHILD PROCESS */

                    /* PARENT PROCESS AFTER FORK */
                    }  else { // Parent process after the fork.
                        // Record monitoring process
                        char message[512];
                        sprintf(message, "Initializing monitoring of process '%s' (PID %d).", process_group.process[new_process_index].process_name, process_group.process[new_process_index].process_id);
                        log_message(message, INFO, main_log_file_path);
                        process_group.process[new_process_index].process_monitor_id = child_process_pid;
                    }
                    /* /PARENT PROCESS AFTER FORK */
                }

                // Otherwise do nothing if process is already monitored.
            }
            // Unset reread config flag.
            reread_config = 0;
        }

        // Check if program should be sut down
        if (kill_program) {
            printf("Caught SIGINT... Leaving program\n");
            exit(0);
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
