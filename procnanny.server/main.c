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
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define MY_PORT 2223
#define DEBUG 0
#define BUFFER_SIZE 10000

// Stores the program name on the command line.
char main_program_name[] = "procnanny.server";

// Globals set by signals handlers.
int reread_config = 0;
int kill_program = 0;
char *main_log_file_path;
char *server_log_file_path;
char *config_path;
struct Config new_config;
int kill_count = 0;

void sigint_handler(int signo) {
    if (signo == SIGINT) {
        kill_program = 1;
    }
    return;
}

void sighup_handler(int signo) {
    if (signo == SIGHUP) {
        reread_config = 1;
        new_config = read_config(config_path);
        char message[512] = { '\0' };
        sprintf(message, "Caught SIGHUP. Configuration file '%s' re-read.", config_path);
        log_message(message, INFO, main_log_file_path, 1, 0);;
    }
    return;
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

    config_path = argv[1];
    // Parse config file
    struct Config config = read_config(argv[1]);

    // Get the server log file path.
    server_log_file_path = getenv("PROCNANNYSERVERINFO");
    if (server_log_file_path == NULL) {
        printf("Error when reading PROCNANNYSERVERINFO variable.\n");
        exit(EXIT_FAILURE);
    }

    // Print the server info the the server file.
    char server_info[512] = {'\0'};
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    printf("Hostname: %s\n", hostname);
    struct hostent* h;
    h = gethostbyname(hostname);
    sprintf(server_info, "NODE %s PID %d PORT %d", h->h_name, getpid(), MY_PORT);
    log_message(server_info, INFO, server_log_file_path, 0, 1);

    // Log required server info message to main logfile.
    // Check if procnanny process is running and prompt user to kill.
    struct Process_Group procnanny_process_group = get_process_group_by_name(main_program_name, 0, 0);
    main_log_file_path = getenv("PROCNANNYLOGS");
    if (main_log_file_path == NULL) {
        printf("Error when reading PROCNANNYLOGS variable.\n");
        exit(EXIT_FAILURE);
    }

    // Print server info to main log file as well.
    char server_info_main_log_file[512] = {'\0'};
    sprintf(server_info_main_log_file, "procnanny server: PID %d on node %s, port %d", getpid(), h->h_name, MY_PORT);
    log_message(server_info_main_log_file, NONE, main_log_file_path, 0, 0);


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
    // Start by creating a place to store sockets to clients.
    int client_socket_group[32];
    int client_socket_group_count = 0;

    // Setup the server socket.
    int sock, snew;
    socklen_t fromlength;
    struct  sockaddr_in master, from;
    sock = socket (AF_INET, SOCK_STREAM, 0);
    // Make the socket non blocking
    fcntl(sock, F_SETFL, O_NONBLOCK);
    if (sock < 0) {
        perror ("Server: cannot open master socket");
        exit (1);
    }

    // Choose a different port number than 2222
    // The way we are setup here we would accept connections from any ip on the machine at port 2222
    // This struct is then passed into bind.
    master.sin_family = AF_INET;
    master.sin_addr.s_addr = INADDR_ANY;
    master.sin_port = htons (MY_PORT);

    if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
        perror ("Server: cannot bind master socket");
        exit (1);
    }

    listen (sock, 32);


    /* MAIN PROGRAM LOOP */
    while (1) {
        printf("In Loop\n");
        // Always read from the connection socket to see if we have new connections.
        while (1) {
            fflush(stdout);
            fromlength = sizeof (from);
            snew = accept(sock, (struct sockaddr*) &from, &fromlength);
            if (snew < 0) {
                // An error occured with the socket.
                perror ("Server: Nothing to read.\n");
                break;
            } else {
                // Make the client socket non blocking.
                fcntl(snew, F_SETFL, O_NONBLOCK);
                // If we accepted a new connection write the config file.
                write (snew, &config.raw_config, sizeof (config.raw_config));
                // Store the socket in the connections array.
                client_socket_group[client_socket_group_count] = snew;
                client_socket_group_count++;
            }
        }

        if (reread_config) {
            // Reset reread_config flag.
            reread_config = 0;

            // See if the new config is the same as the old config
            struct Config new_config = read_config(argv[1]);
            if (new_config.raw_config != config.raw_config){
                config = new_config;
                // The configs are different write to all clients the new config
                int i = 0;
                for (;i < client_socket_group_count; i++) {
                    write(client_socket_group[i], &config.raw_config, sizeof(config.raw_config));
                }
            }
        }

        // Check if we need to shutdown.
        if (kill_program) {
            // Send message to each client notifying to kill
            int i = 0;
            for (;i < client_socket_group_count; i++) {
                char message[BUFFER_SIZE] = {'\0'};
                strcpy(message, "EGGZIT");
                write(client_socket_group[i], message, sizeof(message));
                close(client_socket_group[i]);
            }


            // Do a final read from the sockets to ensure no more processes were killed.
            // Log the total number of processes killed.
            char message[512] = {'\0'};
            sprintf(message, "Caught SIGINT. Exiting cleanly. %d process(es) killed.", kill_count);
            log_message(message, INFO, main_log_file_path, 1, 0);
            close(sock);
            exit(0);
        }

        // Read from each open socket to see if we have info from clients we need to log.
        int k = 0;
        for(;k < client_socket_group_count; k++) {
            int bytes_read = 0;
            char buffer[BUFFER_SIZE] = {'\0'};
            bytes_read = read(client_socket_group[k], &buffer, sizeof(buffer));
            if (bytes_read > 0) {
                // Received logging info, forward to log file.
                printf("Received message from client %s", buffer);
                // Check to see if buffer contains killed message first.
                if (strstr(buffer, "killed") != NULL) {
                    kill_count++;
                }
                log_raw_message(buffer, main_log_file_path);
            }
            // Otherwise nothing to read do nothing.
        }
        /* /MAIN PROGRAM LOOP */
    }
}