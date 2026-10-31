CC = gcc
CFLAGS = -Wall -Wextra -g -DENABLE_LOG
SRC = $(wildcard src/*.c src/syscall/*.c)

all:
	$(CC) $(CFLAGS) $(SRC) -o runner

run:
	./runner initramfs/init

clean:
	rm -f runner
