# ==============================================================================
#
# Defines.common.mk.fback
#
# ==============================================================================

PROG := ssca2comp_TM

SRCS += \
  createPartition.c \
  alg_radix_smp.c \
  computeGraph.c \
  genScalData.c \
  getUserParameters.c \
  globals.c \
  ssca2comp.c \
  $(LIB)/memory.c \
  $(LIB)/mt19937ar.c \
  $(LIB)/random.c \
  $(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}

# ==============================================================================
#
# End of Defines.common.mk.fback
#
# ==============================================================================
