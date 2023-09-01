
CFLAGS +=

PROG := histo_TM

SRCS += \
        main.c \
        parboil.c \
        util.c \
        $(LIB)/thread.c \
        $(LIB)/memory.c
#
OBJS := ${SRCS:.c=.o}

