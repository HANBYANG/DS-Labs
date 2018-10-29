CC=g++

SRCS=router.cpp\
	rtbl.cpp\
	udp.cpp

OBJS=$(SRCS:.cpp=.o)

EXEC=dsdv

start:$(OBJS)
	$(CC) -pthread -o $(EXEC) $(OBJS)

.cpp.o:
	$(CC) -std=c++11 -o $@ -c $<

clean: 
	rm -rf $(OBJS)
