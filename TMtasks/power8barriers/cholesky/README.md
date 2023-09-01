# Algoritmo Cholesky (Livermore loops 2) paralelizado y chunkeado

## Requisitos:

+ Se espera un directorio ../STM-implementations con los STM/HTM
+ Si es un STM, se espera una librería libstm.a en ../STM-implementations/xxx/lib/
+ Si es un HTM, se espera un tm.h en ../STM-implementations/xxx/include/

Adicionalmente, las API se espera que estén en ../STM-implementations/tmapi.h.xxx donde
xxx es el nombre asociado al TM. El script compile.sh, lo primero que hace es una copia
de tmapi.h.xxx -> tmapi.h, que es el fichero que carga floyd.c para la API

## Compilación y lanzamiento de los distintos perfiles

Para compilar se debe usar el script `compile.sh` con el perfil correcto:

+ __Secuencial__: usar script con perfil `seq`. Ejecutar con `-x seq`
+ __Unprotected__: usar script con perfil `seq`. Ejecutar con `-x unp`
+ __HTM + barrera__: Usar script con `p8|p8ord|p8ordwsb` según el HTM. Ejecutar con `-x htm`
+ __HTM sin barrera__: (Resultados incorrectos salvo p8ord). Usar script con `p8|p8ord|p8ordwsb` según el HTM. Ejecutar con `-x htmnb`
+ __HTM con barrera especulativa__: (Sólo para p8ordwsb y p8ordwsbnt). Usar script con `p8ordwsb` según el HTM. Ejecutar con `-x htmwsb`

## Flags y defines

+ DEBUG_ENABLED: Información extra de debug (no válido para benchmark, pues produce overhead)
+ SP_BARRIERS:   Se debe habilitar sólo si se está compilando con un STM/HTM que admita barreras especulativas. Se usa para que la
                 compilacion no falle si no las hay. El Makefile ya se encarga de habilitarlo cuando sea necesario
