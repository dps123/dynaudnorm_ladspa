#export CXXFLAGS = -Wall -O3 -fPIC -DPIC -g -march=pentium4
export CXXFLAGS = -Wall -O3 -fPIC -DPIC -DNDEBUG

# This is where it will be installed
export PREFIX = /usr
export LADSPA_PATH = $(PREFIX)/lib/ladspa/

# End of user-editable options.

LIB=dynaudnorm.so
SRCS=$(wildcard src/*.cpp)
OBJS=$(SRCS:.cpp=.o)

CC = $(CXX)

.PHONY: all
all: $(LIB)

.PHONY: install
install: all
	cp -f $(LIB) $(LADSPA_PATH)/

.PHONY: clean
clean:
	-rm -f *.o $(LIB)

.PHONY: depend
depend: $(SRCS)
	$(MAKEDEP) -c "$(CCX) $(CXXFLAGS) -M" -i $@ $?

$(LIB): $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $(LIB) $(OBJS)


