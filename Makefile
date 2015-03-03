PROJECT=x68emu

SRCS=$(wildcard ./*.cc)
OBJS=$(SRCS:%.cc=%.o)

#CXXFLAGS += -Wall -Wextra -std=c++0x -DNDEBUG -O2
CXXFLAGS += -Wall -Wextra -std=c++0x -DDEBUG -O0

.PHONY: all clean test

all:	$(PROJECT)

clean:
	rm -rf $(OBJS)
	rm -f $(PROJECT)

$(PROJECT):	$(OBJS)
	g++ -o $(PROJECT) $(OBJS)
