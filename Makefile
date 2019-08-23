myprog:	main.o functions.o semaphores.o threads.o
	gcc -pthread -o myprog main.o functions.o semaphores.o threads.o -lcrypto -lssl
threads.o:	threads.c threads.h
	gcc -c threads.c
functions.o:	functions.c functions.h
	gcc -c functions.c
semaphores.o:	semaphores.c semaphores.h
	gcc -c semaphores.c

main.o:	main.c functions.h semaphores.h
	gcc -c main.c


clean:
	rm functions.o semaphores.o main.o threads.o
