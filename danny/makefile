all: danny clean
danny: fdreader.o danny.o utils.o network.o
	gcc fdreader.o danny.o utils.o network.o -o danny -Wall -Wextra
fdreader.o: fdreader.c fdreader.h
	gcc fdreader.c -c -Wall -Wextra
danny.o: danny.c fdreader.h utils.h network.h
	gcc danny.c -c -Wall -Wextra
utils.o: utils.c utils.h
	gcc utils.c -c -Wall -Wextra
network.o: network.c network.h
	gcc network.c -c -Wall -Wextra
clean:
	rm *.o
runValgrind:
	valgrind --leak-check=full --track-fds=yes --show-reachable=yes --track-origins=yes --show-leak-kinds=all danny config.dat
run:
	danny config.dat