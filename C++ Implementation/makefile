manager: main.o manager.o server.o database.o
	g++ main.cpp manager.cpp database.cpp server.cpp raft.cpp -o manager -pthread -g

clean:
	rm -f *.o