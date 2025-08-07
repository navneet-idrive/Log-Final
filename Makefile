CXX = g++
CXXFLAGS = -std=c++17 -I/opt/boost_1_88_0/include
LDFLAGS = -L/opt/boost_1_88_0/lib -lboost_system -lssl -lcrypto -lcurl -lpthread

SRC = main_server.cpp
OUT = main_server

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

