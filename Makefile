make:
	gcc -o tomato tomato.c -lncurses -g -W

clean:
	touch tomato.c
