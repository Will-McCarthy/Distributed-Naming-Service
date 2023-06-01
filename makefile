# CSCI 4780 Spring 2021 Project 4
# Jisoo Kim, Will McCarthy, Tyler Scalzo

GCC=g++
FLAGS=-pthread
CLTPROG=nameserver
SRVPROG=bootstrap

$(CLTPROG): nameserver.o
	$(GCC) -o $(CLTPROG) nameserver.o ${FLAGS}

$(SRVPROG): bootstrap.o
	$(GCC) -o $(SRVPROG) bootstrap.o ${FLAGS}

nameserver.o: nameserver.cpp
	$(GCC) -c nameserver.cpp ${FLAGS}

bootstrap.o: bootstrap.cpp
	$(GCC) -c bootstrap.cpp ${FLAGS}

all: $(CLTPROG) $(SRVPROG)

clean:
	rm -rf $(CLTPROG) $(SRVPROG) *.o
