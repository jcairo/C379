#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

// Represents a process
struct Process {
    int process_id;       // Pid of the proceess being monitored
    int process_monitor;  // Process monitor pid.
    char user[50];        // User who created the process
    char start_time[50];  // Time the process was started
    char ps_aux_time[50]; // Time ps aux was run
};

// Represents a collection of processes
struct Process_Group {
    int process_count;
    struct Process process[100];
};

void get_current_processes(struct Process_Group *process_group);
int proc_nanny_running();

#endif
