procnanny: procnanny.o memwatch.o
    gcc -DMEMWATCH -DMW_STUDIO -o procnanny procnanny.o memwatch.o

procnanny.o: procnanny.c memwatch.h
    gcc -DMEMWATCH -DMW_STUDIO -c procnanny.c 

memwatch.o: memwatch.o memwatch.h
    gcc -DMEMWATCH -DMW_STUDIO -c memwatch procnanny.o memwatch.o

clean: 
	rm -f *.o *~ *.out
