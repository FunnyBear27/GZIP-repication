COMP=clang++
CPPFLAGS= -g -O0 -std=c++17 -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer
BIN=bin
CFILES=$(foreach D, ./, $(wildcard ./*.cpp))
OBJFILES=$(patsubst %.cpp, %.o, $(CFILES))

.DEFAULT_GOAL: all

all: $(BIN)

$(BIN): $(OBJFILES)
	$(COMP) $(CPPFLAGS) $^ -o $@

%.o: %.cpp
	$(COMP) $(CPPFLAGS) -c $^ -o $@

clean:
	rm -rf $(BIN) $(OBJFILES)
	
