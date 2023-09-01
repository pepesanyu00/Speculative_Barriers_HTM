
CFLAGS +=

PROG := stencil_TM

SRCS += \
	main.c \
        parboil.c \
        file.c \
        kernels.c \
	$(LIB)/thread.c \
        $(LIB)/memory.c
#
OBJS := ${SRCS:.c=.o}

