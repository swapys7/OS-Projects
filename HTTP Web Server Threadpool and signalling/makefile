CC      = gcc
FILES   = csuc_http_swapniljoshi.c bounded_buffer.c print_log.c utilityfunction.c
OUT_EXE = csuc_http
LDFLAGS = -pthread -lrt
build:
	$(CC) $(FILES) -o $(OUT_EXE) $(LDFLAGS)

clean:
	rm -f *.o core

rebuild: clean build
