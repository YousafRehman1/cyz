# CS311 Mini Shell
# Default make uses getline(), so the project runs without external downloads.
# To satisfy the linenoise requirement, copy official linenoise.c/.h here, then:
#     make clean && make USE_LINENOISE=1

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
OBJS = main.o parser.o exec.o builtins.o

ifeq ($(USE_LINENOISE),1)
CFLAGS += -DUSE_LINENOISE
OBJS += linenoise.o
endif

mysh: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean test

clean:
	rm -f mysh *.o

test: mysh
	bash ../tests/run_tests.sh
