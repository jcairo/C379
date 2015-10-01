#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H
#include <sys/types.h>
#include "config_reader.h"

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 1024
// Represents a process
struct Process {
    pid_t process_id;       // Pid of the proceess being monitored
    pid_t process_monitor_id;  // Process monitor pid.
    char process_name[MAX_LINE_LENGTH];  // Name of process being monitored
    int process_monitored;  // whether process was started to monitor.
};

// Represents a collection of processes
struct Process_Group {
    int process_count;
    struct Process process[MAX_PROCESSES];
};


int get_total_processes_killed();
void kill_processes(struct Process_Group process_group, struct Config config); 
struct Process_Group get_process_group_by_name(char *process_name);
struct Process_Group get_all_processes(struct Config config);
int proc_nanny_running();

#endif
