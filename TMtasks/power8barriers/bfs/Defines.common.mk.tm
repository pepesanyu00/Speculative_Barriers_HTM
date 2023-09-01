
CFLAGS +=

PROG := bfs_TM

SRCS += \
	main.c \
        parboil.c \
	$(LIB)/thread.c \
        $(LIB)/memory.c \
        $(LIB)/queue.c \
        $(LIB)/random.c \
        $(LIB)/mt19937ar.c
#
OBJS := ${SRCS:.c=.o}

