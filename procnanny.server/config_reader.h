#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#define MAX_CONFIG_PROGRAMS 128
#define MAX_CONFIG_CHARS_PER_LINE 255

struct Config {
    int application_count;
    char application_names[MAX_CONFIG_PROGRAMS][MAX_CONFIG_CHARS_PER_LINE];
    int application_timeout[MAX_CONFIG_PROGRAMS];
    char raw_config[10000];
};

struct Config read_config(char *path);

#endif
