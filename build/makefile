CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = HttpServer
OBJS =  ../code/timer/*.cpp \
       ../code/database/*.cpp ../code/epoller/*.cpp \
       ../code/http/*.cpp ../code/webserver/*.cpp \
       ../code/buffer/*.cpp ../code/main.cpp ../code/log/*.cpp \

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient -ljsoncpp

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)
