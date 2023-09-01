
CFLAGS +=

PROG := cholesky_TM

SRCS += \
	cholesky.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}

