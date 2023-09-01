
CFLAGS +=

PROG := iwomp_CS

SRCS += \
	iwomp.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}

