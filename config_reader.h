#ifndef CONFIG_READER_H
#define CONFIG_READER_H

struct Config {
    int application_number;
    int time;
    char *application_names[];
};

void read_config(struct Config *config, char *path);

#endif
