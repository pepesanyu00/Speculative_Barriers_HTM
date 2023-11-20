import sys


if len(sys.argv) < 2:
    print("Input Error: usage ./matrix.py size")
    exit()
size = sys.argv[1]


# Crear una matriz de 10x10 inicializada con ceros
matriz = [[0] * int(size) for _ in range(int(size))]

# Ruta al archivo de texto con los valores de x e y
archivo_txt = 'salida.txt'

# Leer el archivo de texto y extraer los valores
with open(archivo_txt, 'r') as file:
    lista = []
    for line in file:
        tileii = 0
        tilej = 0
        x = 0
        y = 0
        vals= line.split()
        for el in vals:
            val = el.split(":")
            if val[0] == 'tileii':
                tileii = val[1]
            elif val[0] == 'tilej':
                tilej = val[1]
            elif val[0] == 'x':
                x = int(val[1])
            else:
                y = int(val[1])
        lista.append(tileii+ ","+tilej+ "," +str(x)+ "," +str(y))

tiles = {}
for elemento in lista:
    i = elemento[0]
    j = elemento[2]
    tile = f"tile_{i}"
    if tile not in tiles:
        tiles[tile] = []

    tiles[tile].append(elemento)

i = 1
for tile in tiles.values():
    for elemento in tile:
        elemento = elemento.split(',')
        matriz[int(elemento[2])][int(elemento[3])] = i
    i+=1



# Imprimir la matriz resultante
for fila in matriz:
    print(fila)