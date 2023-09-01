
CFLAGS +=

PROG := cholesky_CS

SRCS += \
	cholesky.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}

