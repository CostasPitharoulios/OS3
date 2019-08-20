functions: functions.h
	gcc -c functions.c

main: main.c functions.h functions.o
	gcc -o main main.c functions.o

clean:
	rm functions.o
