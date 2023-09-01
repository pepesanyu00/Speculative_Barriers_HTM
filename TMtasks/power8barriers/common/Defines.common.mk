# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CC       := gcc
# -mhtm para los builtins de htm de Power8
CFLAGS   += -g -Wall -mhtm
CFLAGS   += -O2
CFLAGS   += -I$(LIB)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := gcc
LIBS     += -lpthread -lm

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib

LOGTM_DIR := ../common
# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
