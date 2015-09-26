#include <stdio.h>
#include <unistd.h>
#include "user_interaction.h"
#include "process_manager.h"
#include "config_reader.h"
#include "logger.h"
#include "memwatch.h"

char main_program_name[] = "procnanny";


int main(int argc, char *argv[]) {
    // Parse config file
    struct Config config = read_config(argv[1]);

    // Check if procnanny process is running and prompt user to kill.
    struct Process_Group procnanny_process_group = get_process_group_by_name(main_program_name);
    if (procnanny_process_group.process_count > 1) {
        // Prompt user to quit existing process.
        int kill = prompt_user_for_instructions();        
        if (kill) {
            // Kill the existing procnanny processes.
            kill_processes(procnanny_process_group);
        } else {
            // Exit this instance of procnanny
            exit(EXIT_FAILURE);
        }
    }
    
    while(1) {
        sleep(1);
        printf("Tick");
        fflush(stdout);
    }

    // struct Process_Group process_group = get_all_processes(config);


}

