all:
	gcc src/*.c -o runner -g

run:
	./runner initramfs

