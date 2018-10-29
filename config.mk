COMMONFLAGS	= -Werror -Wall -pedantic \
		  -O2 -g

CC		= cc
CSTD		= iso9899:1999
CFLAGS		= -std=$(CSTD) $(COMMONFLAGS) \
		  -ftrapv

LD		= $(CC)
LDFLAGS		=
LIBS		= 

EXE		= nyx

SRCDIR		= src
SRCS		= $(shell find ${SRCDIR} -name "*.c")
OBJS		= $(patsubst %, %.o, $(SRCS))

BUILT		= $(shell find ${SRCDIR} -name "*.o") \
		  $(shell find . -name $(EXE))

LINTER		= splint
