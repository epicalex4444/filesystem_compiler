.PHONY: all clean

all: fs_compiler

clean:
	rm -f fs_compiler

fs_compiler: main.c
	gcc -O3 $< -o $@
