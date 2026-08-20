processflow/processflow: processflow/main.o
	gcc processflow/main.o -o processflow/processflow

processflow/main.o: processflow/src/main.c 
	gcc -c processflow/src/main.c -o processflow/main.o

clean:
	rm -f processflow/*.o processflow/processflow

.PHONY: clean