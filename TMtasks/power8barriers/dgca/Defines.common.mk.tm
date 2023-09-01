
CFLAGS +=

PROG := dgca_TM

SRCS += \
	dgca.c \
	$(LIB)/thread.c \
	$(LIB)/random.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/memory.c

#
OBJS := ${SRCS:.c=.o}

