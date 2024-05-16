import os

def read_indices(file_path):
    indices = []
    with open(file_path, 'r') as file:
        lines = file.readlines()
        for line in lines[8:]:  # Ignorar las primeras 8 líneas
            parts = line.strip().split(',')
            if len(parts) > 3:  # Asegurarse de que haya suficientes elementos en la línea
                indices.append(parts[3])
    return indices

def compare_indices(files):
    # Obtener los índices de cada archivo
    indices_list = [read_indices(file) for file in files]

    # Comparar los índices de los archivos
    if all(indices == indices_list[0] for indices in indices_list):
        print("Sí")
    else:
        print("No")

if __name__ == "__main__":
    # Carpeta donde se encuentran los archivos
    folder = 'results'

    # Obtener la lista de archivos en la carpeta, excluyendo aquellos que contienen "Unprot" en su nombre
    #PARA EXCLUIR UNPROT
    files = [os.path.join(folder, file) for file in os.listdir(folder) if os.path.isfile(os.path.join(folder, file)) and "Unprot" not in file and "d0" not in file]

    #files = [os.path.join(folder, file) for file in os.listdir(folder) if os.path.isfile(os.path.join(folder, file)) and "d0" not in file]
    # Comparar los índices de los archivos
    compare_indices(files)
