makeparts: part1.c part2.c part3.c
	$(info Compiling...)
	gcc -g -o part1 part1.c
	gcc -g -o part2 part2.c
	gcc -g -o part3 part3.c
	gcc -g -o part4 part4.c

clean:
	$(info Removing executables...)
	rm -f part1
	rm -f part2
	rm -f part3
	rm -f part4
