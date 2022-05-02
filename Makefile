# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -Wall -Wextra

# The build target 
TARGET = p4exp1

all: $(TARGET)

$(TARGET): main.cpp
			$(CC) $(CFLAGS) -o $(TARGET) main.cpp

clean:
			$(RM) $(TARGET)