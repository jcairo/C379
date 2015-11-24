all:
	gcc -Wall -DMEMWATCH -DMW_STDIO -o procnanny.client/procnanny.client procnanny.client/main.c procnanny.client/user_interaction.c procnanny.client/process_manager.c procnanny.client/memwatch.c procnanny.client/config_reader.c procnanny.client/logger.c
	gcc -Wall -DMEMWATCH -DMW_STDIO -o procnanny.server/procnanny.server procnanny.server/main.c procnanny.server/user_interaction.c procnanny.server/process_manager.c procnanny.server/memwatch.c procnanny.server/config_reader.c procnanny.server/logger.c

clean:
	rm -f *.o procnanny.client/procnanny.client
	rm -f *.o procnanny.server/procnanny.server
