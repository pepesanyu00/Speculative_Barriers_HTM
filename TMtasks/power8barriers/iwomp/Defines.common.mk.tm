
CFLAGS +=

PROG := iwomp_TM

SRCS += \
	iwomp.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}

