#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
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
    printf("Parent process ID is: %d\n", getpid());
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
    reread_config = 1;

    // Check if procnanny process is running and prompt user to kill.
    struct Process_Group procnanny_process_group = get_process_group_by_name(main_program_name, 0, 0);
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
    // Start by setting up an empty version of the main process group.
    struct Process_Group process_group = get_empty_process_group();

    if (DEBUG) {
        int k = 0;
        for (;k < process_group.process_count; k++) {
            printf("Process found: %s, time to kill %d, process id is: %d\n", process_group.process[k].process_name, process_group.process[k].time_to_kill, process_group.process[k].process_id);
        }
        printf("Program count %d\n", process_group.process_count);
    }


    // int last_time_checked = (int)time(NULL);
    int non_busy_monitor_process_found = 0;
    // Indicates the first time through the loop.
    // Used to determine whether we are in the loop for the first time.
    int first_time_through = 1;
    // Indicates the total number of processes killed.
    int total_processes_killed = 0;
    // All programs must be checked for the initial sweep.
    int time_last_checked = ((int)time(NULL));
    // int time_since_last_check = ((int)time(NULL) - last_time_checked);

    /* MAIN PROGRAM LOOP */
    while (1) {
        fflush(stdout);
        sleep(1);
        // Read from all pipes to see if any processes have been killed if so update the
        // child processes to a non busy status.
        int i = 0;
        for (;i < process_group.process_count; i++) {
            printf("In loop checking for communication from child processes. Child process count is %d\n", process_group.process_count);
            // Setup reqs for reading from pipe
            char readbuffer[512];
            int readbytes;
            // char *pch;

            // Read from pipe 1 means killed process 0 means process was killed before child tried to kill it.
            // This read is set not to block.
            readbytes = read(process_group.process[i].pipe_to_parent[0], readbuffer, sizeof(readbuffer));
            // If we read anything from the pipe the process the message otherwise do nothing.
            if (readbytes > 0) {
                printf("Parent read %d bytes from child process\n", readbytes);
                printf("Parent read message: '%s' from pipe\n", readbuffer);
                int child_message = atoi(readbuffer);
                if (child_message == 1) {
                    // The child process succesfully killed its target, print to log
                    total_processes_killed++;
                    char message[512] = {'\0'};
                    sprintf(message, "PID %d (%s) killed after exceeding %d seconds.", process_group.process[i].process_id, process_group.process[i].process_name, process_group.process[i].time_to_kill);
                    log_message(message, ACTION, main_log_file_path, 0);
                } else if (child_message == 0) {
                    // The child did not kill its target it was already dead.
                } else {
                    // If the message is not 0 or 1 we have a problem quit.
                    printf("The child message did not evaluate to 0 or 1. Unexptected behaviour shutting down...\n");
                    exit(EXIT_FAILURE);
                }

                // Reset the process info
                process_group.process[i].time_to_kill = -1;
                process_group.process[i].time_to_kill = -1;
                process_group.process[i].busy = 0;
                process_group.process[i].process_id = -1;
                process_group.process[i].process_monitor_id = -1;
                strcpy(process_group.process[i].process_name, "");

            }
        }

        // Check if config should be reread or we should rescan processes.
        if (reread_config || ((int)time(NULL) - time_last_checked > 4)) {
            printf("In check loop\n");
            // Determine whether we are rescanning programs because of 5 second time lapse or config reread.

            // If we are rereading the config Rearead the config file and replace old version.
            if (reread_config) {
                if (!first_time_through) {
                    char message[512] = { '\0' };
                    sprintf(message, "Caught SIGHUP. Configuration file '%s' re-read.", argv[1]);
                    log_message(message, INFO, main_log_file_path, 1);
                }
                first_time_through = 0;
                config = read_config(argv[1]);
            }

            // Get a list of active processes.
            struct Process_Group current_process_group = get_all_processes(config, reread_config);
            reread_config = 0;

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
                            char string_to_pipe[20] = {'\0'};
                            snprintf(pid_string, 10, "%d", current_process_group.process[i].process_id);
                            snprintf(time_to_kill_string, 10, "%d", current_process_group.process[i].time_to_kill);
                            strcat(string_to_pipe, pid_string);
                            strcat(string_to_pipe, " ");
                            strcat(string_to_pipe, time_to_kill_string);
                            write(process_group.process[j].pipe_to_child[1], string_to_pipe, sizeof(string_to_pipe));
                            printf("Wrote %s to child\n", string_to_pipe);

                            // Update process group information
                            process_group.process[j].time_to_kill = current_process_group.process[i].time_to_kill;
                            process_group.process[j].busy = 1;
                            process_group.process[j].process_id = current_process_group.process[i].process_id;
                            strcpy(process_group.process[j].process_name, current_process_group.process[i].process_name);

                            // Output to logfile new process being monitored
                            // Record monitoring process
                            char message[512] = {'\0'};
                            sprintf(message, "Initializing monitoring of process '%s' (PID %d).", process_group.process[j].process_name, process_group.process[j].process_id);
                            log_message(message, INFO, main_log_file_path, 0);
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

                    // Create pipe to child and pipe to parent
                    pipe(process_group.process[new_process_index].pipe_to_parent);
                    pipe(process_group.process[new_process_index].pipe_to_child);

                    // Taken from http://www.albany.edu/~csi402/pdfs/handout_15.2.pdf
                    /* Set O_NONBLOCK flag for the read end (fd[0]) of the pipe. */
                    if (fcntl(process_group.process[new_process_index].pipe_to_parent[0], F_SETFL, O_NONBLOCK) == -1) {
                        fprintf(stderr, "Call to fcntl failed.\n"); exit(1);
                        exit(EXIT_FAILURE);
                    }

                    // Fork the child process.
                    pid_t child_process_pid;
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
                        if (message == 1) { write(process_group.process[new_process_index].pipe_to_parent[1], "1\0", 2);
                        } else if (message == 0) { write(process_group.process[new_process_index].pipe_to_parent[1], "0\0", 2);
                        } else { printf("The process was not killed returned unexepected result\n"); exit(EXIT_FAILURE);
                        }

                        while (1) {
                            // Setup reqs for reading from pipe
                            char readbuffer[512];
                            int readbytes;
                            char *pch;
                            // If read pipe returns value
                            sleep(1);
                            printf("In child read loop.");
                            readbytes = read(process_group.process[new_process_index].pipe_to_child[0], readbuffer, sizeof(readbuffer));
                            printf("Read %s from pipe 0\n", readbuffer);
                            // readbytes = read(process_group.process[new_process_index].fd[1], readbuffer, sizeof(readbuffer));
                            // printf("Read %s from pipe 1\n", readbuffer);
                            if (readbytes > 0) {
                                printf("String received by child: %s\n", readbuffer);
                                // Pipe should send pid of process to kill followed by time to kill
                                pch = strtok(readbuffer, " ");
                                process_to_kill = atoi(pch);
                                pch = strtok(NULL, " ");
                                time_to_kill = atoi(pch);
                                sleep(time_to_kill);
                                message = kill_process(process_to_kill);

                                // Write to pipe to tell parent child process killed
                                // 1 means killed 0 means not killed
                                if (message == 1) { write(process_group.process[new_process_index].pipe_to_parent[1], "1\0", 2);
                                } else if (message == 0) { write(process_group.process[new_process_index].pipe_to_parent[1], "0\0", 2);
                                } else { printf("The process was not killed returned unexepected result\n"); exit(EXIT_FAILURE);
                                }
                            }
                        }
                    /* /CHILD PROCESS */

                    /* PARENT PROCESS AFTER FORK */
                    }  else {
                        // Record monitoring process
                        char message[512] = {'\0'};
                        sprintf(message, "Initializing monitoring of process '%s' (PID %d).", process_group.process[new_process_index].process_name, process_group.process[new_process_index].process_id);
                        log_message(message, INFO, main_log_file_path, 0);
                        process_group.process[new_process_index].process_monitor_id = child_process_pid;
                    }
                    /* /PARENT PROCESS AFTER FORK */
                }

                // Otherwise do nothing if process is already monitored.
            }
            // Unset reread config flag.
            time_last_checked = (int)time(NULL);
        }

        // Check if program should be sut down
        if (kill_program) {
            // Close all pipes
            int i = 0;
            for (;i < process_group.process_count; i++) {
                kill_process(process_group.process[i].process_monitor_id);
                // Read from pipe to ensure no further processes killed
                char readbuffer[512];
                int readbytes;

                // Read from pipe 1 means killed process 0 means process was killed before child tried to kill it.
                // This read is set not to block.
                readbytes = read(process_group.process[i].pipe_to_parent[0], readbuffer, sizeof(readbuffer));
                // If we read anything from the pipe the process the message otherwise do nothing.
                if (readbytes > 0) {
                    int child_message = atoi(readbuffer);
                    if (child_message == 1) {
                        // The child process succesfully killed its target, print to log
                        total_processes_killed++;
                        char message[512] = {'\0'};
                        sprintf(message, "PID %d (%s) killed after exceeding %d seconds.", process_group.process[i].process_id, process_group.process[i].process_name, process_group.process[i].time_to_kill);
                        log_message(message, ACTION, main_log_file_path, 0);
                    } else if (child_message == 0) {
                        // The child did not kill its target it was already dead.
                    } else {
                        // If the message is not 0 or 1 we have a problem quit.
                        printf("The child message did not evaluate to 0 or 1. Unexptected behaviour shutting down...\n");
                        exit(EXIT_FAILURE);
                    }

                }
                close(process_group.process[i].pipe_to_parent[0]);
                close(process_group.process[i].pipe_to_parent[1]);
                close(process_group.process[i].pipe_to_child[0]);
                close(process_group.process[i].pipe_to_child[1]);
            }
            char message[512] = {'\0'};
            sprintf(message, "Caught SIGINT. Exiting cleanly. %d process(es) killed.", total_processes_killed);
            log_message(message, INFO, main_log_file_path, 1);
            exit(0);
        }
    }
    /* /MAIN PROGRAM LOOP */
}
