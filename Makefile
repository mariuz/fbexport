###############################################################################
.SUFFIXES: .o .cpp

OBJECTS_FBE=fbexport/ParseArgs.o fbexport/FBExport.o fbexport/cli-main.o
OBJECTS_FBC=fbcopy/args.o fbcopy/fbcopy.o fbcopy/TableDependency.o fbcopy/main.o 

# Compiler & linker flags
COMPILE_FLAGS=-O2 -DIBPP_LINUX -DIBPP_GCC -Iibpp -W -Wall -fPIC
LINK_FLAGS=-pthread -lfbclient 

#COMPILE_FLAGS=-O1 -DIBPP_WINDOWS -DIBPP_GCC -Iibpp
#LINK_FLAGS=

all:	exe/fbcopy exe/fbexport

exe/fbexport: $(OBJECTS_FBE) ibpp/all_in_one.o
	g++ ibpp/all_in_one.o $(OBJECTS_FBE) -oexe/fbexport $(LINK_FLAGS) 

exe/fbcopy: $(OBJECTS_FBC) ibpp/all_in_one.o
	g++ ibpp/all_in_one.o $(OBJECTS_FBC) -oexe/fbcopy $(LINK_FLAGS) 

# Linux only
#	FB2.x: g++ -pthread -lfbclient $(OBJECTS) -o$(EXENAME)

install:
	install exe/fbcopy /usr/bin/fbcopy
	install exe/fbexport /usr/bin/fbexport

.cpp.o:
	g++ -c $(COMPILE_FLAGS) -o $@ $<

clean:
	rm -f fbcopy/*.o
	rm -f ibpp/all_in_one.o
	rm -f exe/fbcopy*
	rm -f fbexport/*.o
	rm -f exe/fbexport*

#EOF
