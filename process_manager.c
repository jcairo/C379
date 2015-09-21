#include <stdio.h>
#include <stdlib.h>
#include "process_manager.h"

#define MAX_CHARS 1000

/* Checks whether and existing procnanny process is already running */
int proc_nanny_running() {
    return 1;

    int is_running = 1;
    system("ps aux");
    return is_running;
}

// Takes string from ps aux command returns process id integer
int get_process_id(char *ps_aux_line) {
    return 0;     
}

// Takes Process_Group struct and array of programs
// Modifies Process_Group struct
void get_current_processes(struct Process_Group *process_group) {
    FILE *input;
    char buffer[MAX_CHARS];
    input = popen("ps aux", "r");

    // Ensure pipe opens properly.
    if (input == NULL) {
        printf("Error when opening pipe to read ps aux command\n"); 
        return;
    }
    
    // Read command output line by line.
    // Guidance from http://www.sw-at.com/blog/2011/03/23/popen-execute-shell-command-from-cc/
    while(fgets(buffer, sizeof(MAX_CHARS), input) != NULL) {
         int process_id = get_process_id(buffer);
         printf("%s", buffer); 
    }

    pclose(input);
}
