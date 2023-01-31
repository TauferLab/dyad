DYAD_LIB_PATH = /usr/lib
DYAD_INCLUDE_PATH = /usr/include

CFLAGS = -std=c99
CXXFLAGS = -std=c++11 -I $(DYAD_INCLUDE_PATH) -L $(DYAD_LIB_PATH) -l dyad_fstream
CPPFLAGS = -O3 -I .

all: c_prod c_cons cpp_prod cpp_cons

c_prod: prod.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $? -o $@

c_cons: cons.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $? -o $@

cpp_prod: prod.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $? -o $@

cpp_cons: cons.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $? -o $@

.PHONY: clean

clean:
	rm c_prod c_cons cpp_prod cpp_cons
