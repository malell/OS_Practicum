all: wendy clean
wendy: fdreader.o network.o wendy.o
	gcc fdreader.o network.o wendy.o  -o wendy -Wall -Wextra -lpthread
fdreader.o: fdreader.c fdreader.h
	gcc fdreader.c -c -Wall -Wextra
network.o: network.c network.h
	gcc network.c -c -Wall -Wextra -lpthread
wendy.o: wendy.c fdreader.h network.h
	gcc wendy.c -c -Wall -Wextra -lpthread
clean:
	rm *.o
runValgrind:
	valgrind --leak-check=full --track-fds=yes --show-reachable=yes --track-origins=yes --show-leak-kinds=all wendy config.dat
run:
	wendy config.dat