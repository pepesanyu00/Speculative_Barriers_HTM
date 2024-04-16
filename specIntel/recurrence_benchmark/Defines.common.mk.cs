
CFLAGS +=

PROG := recurrence_CS

SRCS += \
	recurrence.c \
	$(LIB)/thread.c \
        $(LIB)/memory.c
#
OBJS := ${SRCS:.c=.o}

