#include <stdio.h>
#include <stdlib.h>
#include "user_interaction.h"

/* prompts the user if they want to kill previous proess */
int prompt_user_for_instructions() {
    char response;
    printf("A procnanny process is already running.\n");
    printf("To kill the existing process and rerun procnanny type y.\n");
    printf("To allow the existing process to continue type n.\n");
    fflush(stdin);
    scanf(" %c", &response);
    printf("The response was %c\n", response);
    if (response == 'y' || response == 'Y') {
        printf("Responded yes");
        fflush(stdout);
        return 1; 
    }

    if (response == 'n' || response == 'N') {
        printf("Responded no");
        fflush(stdout);
        return 0; 
    }

    // If response does not match expected close program.
    printf("You have not provided an appropriate response, this process will be shut down.\n");
    exit(EXIT_FAILURE);
}
 
