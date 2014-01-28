SRCS=AutoDiff.cpp Main.cpp
CPP=g++
INCLUDES=-I.
CFLAGS=-c $(INCLUDES)
OBJDIR=build
OBJS=$(SRCS:%.cpp=$(OBJDIR)/%.cpp.o)
TARGET=autodiff

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CPP) -o $@ $(CLAGS) $^

$(OBJDIR)/%.cpp.o: %.cpp
	$(CPP) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.phony: clean
