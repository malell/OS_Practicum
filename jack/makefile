all: jack clean
jack: fdreader.o lloyd.o network.o jack.o
	gcc fdreader.o lloyd.o network.o jack.o  -o jack -Wall -Wextra -lpthread
fdreader.o: fdreader.c fdreader.h
	gcc fdreader.c -c -Wall -Wextra
lloyd.o: lloyd.c lloyd.h semaphore_v2.h
	gcc lloyd.c -c -Wall -Wextra
network.o: network.c network.h lloyd.h
	gcc network.c -c -Wall -Wextra -lpthread
jack.o: jack.c fdreader.h network.h
	gcc jack.c -c -Wall -Wextra -lpthread
clean:
	rm *.o
runValgrind:
	valgrind --leak-check=full --track-fds=yes --show-reachable=yes --track-origins=yes --show-leak-kinds=all jack config.dat
run:
	jack config.dat
