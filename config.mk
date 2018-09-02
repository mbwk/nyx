#LANGUAGE	= C
LANGUAGE	= C++

COMMONFLAGS	= -Werror -Wall -pedantic \
		  -O2 -g

CC		= cc
CSTD		= iso9899:1999
CFLAGS		= -std=$(CSTD) $(COMMONFLAGS) \
		  -ftrapv

CXX		= c++
CXXSTD		= c++17
CXXFLAGS	= -std=$(CXXSTD) $(COMMONFLAGS) \
		  -fno-exceptions

LD		= $(CC)
LDFLAGS		=
LIBS		= 

EXE		= nyx

SRCDIR		= src
SRCS		= $(shell find ${SRCDIR} -name "*.c") \
		  $(shell find ${SRCDIR} -name "*.cpp")
OBJS		= $(patsubst %, %.o, $(SRCS))

BUILT		= $(shell find ${SRCDIR} -name "*.o") \
		  $(shell find . -name $(EXE))

LINTER		= splint
