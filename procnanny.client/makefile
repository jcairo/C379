all:
	gcc -Wall -DMEMWATCH -DMW_STDIO -o procnanny.client main.c user_interaction.c process_manager.c memwatch.c config_reader.c logger.c

clean:
	rm -f *.o procnanny.client
