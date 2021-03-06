#ifndef LOGGER_H
#define LOGGER_H

#define INFO 0
#define ACTION 1
#include "process_manager.h"

void clear_log_file();
void log_message(char *message, int type, char *file_path, int print_to_stdout, int middle_node_formatting);
void aggregate_log_files(struct Process_Group process_group, char *main_log_file_path);

#endif
