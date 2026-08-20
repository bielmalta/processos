CC = gcc
CFLAGS = -Wall -Wextra -std=c11

processflow/processflow: processflow/src/main.c
	$(CC) $(CFLAGS) processflow/src/main.c -o processflow/processflow

clean:
	rm -f processflow/processflow