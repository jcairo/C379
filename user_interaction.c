#include <stdio.h>
#include "user_interaction.h"

char prompt_user_for_instructions() {
    char response;
    printf("A procnanny process is already running. To kill the existing process and rerun procnanny type y to allow the existing process to continue type n."); 
    scanf("y/n: %s\n", &response);
    return response;
}
 
