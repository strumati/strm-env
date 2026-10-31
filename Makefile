all:
	gcc src/*.c -o runner -g -DENABLE_LOG

run:
	./runner initramfs

