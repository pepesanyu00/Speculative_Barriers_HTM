# ==============================================================================
#
# Defines.common.mk.fback
#
# ==============================================================================

PROG := ssca2cut_TM

SRCS += \
  alg_radix_smp.c \
  computeGraph.c \
  cutClusters.c \
  createPartition.c \
  genScalData.c \
  getUserParameters.c \
  globals.c \
  ssca2cut.c \
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
