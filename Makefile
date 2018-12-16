CFLAGS=-Wall -pedantic -g

mysh: mysh.o 
	gcc -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $^

.PHONY: trace 
trace: 
	strace -f -o strace.out ./mysh
.PHONY: clean
clean:
	rm -f *.o mysh
