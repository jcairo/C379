#include <unistd.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
    time_t start_time, current_time;
    time(&start_time);
    while(1) {
        sleep(1);
        printf("Process name: %s, process id: %d --- ", argv[0], getpid());
        time(&current_time);
        printf("Process running for %ld seconds\n", current_time - start_time);
        fflush(stdout);
    }
}
