#ifndef LOGGER_H
#define LOGGER_H

#define INFO 0
#define ACTION 1

void clear_log_file(); 
void log_message(char *message, int type);

#endif
