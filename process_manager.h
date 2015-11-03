#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H
#include <sys/types.h>
#include "config_reader.h"

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 1024
// Represents a process
struct Process {
    int time_to_kill; // Time the process should be killed in
    int pipe_to_child[2];  // Pipe to child process.
    int pipe_to_parent[2];  // Pipe to parent process.
    int busy;   // Whether the child process is waiting to kill a process or not.
    pid_t process_id;       // Pid of the proceess being monitored
    pid_t process_monitor_id;  // Child Process monitoring the desired process pid.
    char process_name[MAX_LINE_LENGTH];  // Name of process being monitored
    char process_log_file_path[256]; // The path to the monitoring child processes log file.
};

// Represents a collection of processes
struct Process_Group {
    int process_count;
    struct Process process[MAX_PROCESSES];
};

struct Process_Group get_empty_process_group();
int is_monitored(int pid, struct Process_Group process_group);
int get_total_processes_killed();
int kill_process(int pid);
void kill_processes(struct Process_Group process_group, struct Config config, char *log_file_path);
struct Process_Group get_process_group_by_name(char *process_name, int time_to_kill);
struct Process_Group get_all_processes(struct Config config);
int proc_nanny_running();

#endif
