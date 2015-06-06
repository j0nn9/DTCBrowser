VERSION   = 0.0.1
SRC       = ./src
BIN       = ./bin
CC        = g++
DBFLAGS   = -g
CXXFLAGS  = -Wall -Wextra -c -std=c++11 -Wformat -Wformat-security \
            -pthread --param max-inline-insns-single=10000 -lm \
						-Wno-write-strings 
LDFLAGS   = -lsqlite3 -lcurl -ljansson -lmagic -lmicrohttpd -lprotobuf -lbz2 -pthread -lssl -lcrypto
OTFLAGS   = -march=native -O2

.PHONY: clean test all install

# default target
all: link
	$(CC) $(ALL_OBJ) $(LDFLAGS) -o $(BIN)/datacoin-http-server

install: all
	cp $(BIN)/datacoin-http-server /usr/bin/


# development
CXXFLAGS += $(DBFLAGS) 

# optimization
CXXFLAGS  += $(OTFLAGS)
LDFLAGS   += $(OTFLAGS)

ALL_SRC = $(shell find $(SRC) -type f -name '*.cpp')
ALL_OBJ = $(ALL_SRC:%.cpp=%.o)

%.o: %.cpp
	$(CC) $(CXXFLAGS) $^ -o $@

compile: $(ALL_OBJ) $(ALL_OBJ)


prepare:
	@mkdir -p bin
	protoc --cpp_out=./src/ envelope.proto
	mv ./src/envelope.pb.cc ./src/envelope.pb.cpp

link: prepare compile

clean:
	rm -rf $(BIN)
	rm -f $(ALL_OBJ) $(ALL_OBJ)

