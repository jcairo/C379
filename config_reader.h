#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#define MAX_CONFIG_PROGRAMS 128
#define MAX_CONFIG_CHARS_PER_LINE 255

struct Config {
    int application_count;
    int time;
    char *application_names[MAX_CONFIG_PROGRAMS];
};

struct Config read_config(char *path);

#endif
