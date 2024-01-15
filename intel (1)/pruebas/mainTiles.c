#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <omp.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int main(void)
{
    int exclusionZone = 3;
    int maxTileHeight = 6;
    int maxTileWidth = 6;
    int tslength = 600;
    int m = 1;
    assert(maxTileHeight > exclusionZone);
    int mplength = tslength - m + 1;

    char A[mplength][mplength];
    for (int i = 0; i < mplength; i++)
    {
        for (int j = 0; j < mplength; j++)
        {
            A[i][j] = 0;
        }
    }
    omp_set_num_threads(8);
//int num_tile_rows = ceil(mplength / (double)(maxTileHeight));
//int num_tile_cols = ceil(mplength / (double)(maxTileWidth));

//Del código del git de SCAMP, así calculan los tiles:
//Se obtienen coordenadas del vértice superior izquierdo del tile
//Los tiles simétricos
/*for (int offset = 0; offset < num_tile_rows - 1; ++offset) {
      for (int diag = 0; diag < num_tile_cols - 1 - offset; ++diag) {
        printf("%d-%d\n",diag, (diag + offset));
      }
    }

    //Los tiles asimétricos del final de la matriz
    for (int i = 0; i < num_tile_rows; ++i) {
      printf("%d,%d\n",i, (num_tile_cols - 1));
    }*/
#pragma omp parallel proc_bind(close)
    {

        int tid = omp_get_thread_num();
        //Se puede conseguir lo mismo así, obteniendo las coordenadas del tile con respecto a las subsecuencias
        for (int tileii = 0; tileii < mplength; tileii += maxTileHeight)
        {
#pragma omp for schedule(static) nowait
            for (int tilej = tileii; tilej < mplength; tilej += maxTileWidth)
            {
                //RIC Para ir diagonalmente por los tiles le igualar tilei a tilej - tileii
                //    Para recorrer los tiles en el orden de los for poner: int tilei = tileii;
                //int tilei = tilej - tileii;
                int tilei = tileii;
                printf("TID: %d. Coordenadas del tile %d:%d\n", tid, tilei, tilej);
                //Si i==j ==> Coordenada de la diagonal principal. Sólo se calcula el upper triangle.
                //Triángulo superior
                int subi = tilei;
                int subj = MIN(MAX(tilei + exclusionZone + 1, tilej), mplength); //subsequence j
                for (int jj = subj; jj < MIN(tilej + maxTileWidth, mplength); jj++)
                {
                    //printf("TID: %d. %2d-%2d <-- Producto escalar (fila)\n", tid, subi, jj);
                    A[subi][jj] = 1;
                    A[jj][subi] = 1;
                    subi++;
                    for (int k = jj + 1; k < MIN(tilej + maxTileWidth, mplength); k++, subi++)
                    {
                        //printf("TID: %d. %2d:%2d\n", tid, subi, k);
                        A[subi][k] = 1;
                        A[k][subi] = 1;
                    }
                    subi = tilei; //subsequence i
                }
                if (tilei != tilej)
                {
                    //Si el tile difiere en sus coordenadas es un tile de interior y se calcula el lower triangle también
                    //Triángulo inferior
                    int subi = tilei + 1; //+1 pq el upper tile hace el subi 0 (tilei)
                    int subj = tilej;     //subsequence j
                    for (int ii = subi; ii < MIN(MIN(tilei + maxTileHeight, subj - exclusionZone), mplength); ii++)
                    {
                        //printf("TID: %d. %2d-%2d <-- Producto escalar (columna)\n", tid, ii, subj);
                        A[ii][subj] = 1;
                        A[subj][ii] = 1;
                        subj++;
                        //El triángulo inferior puede ser un trapecio si está en el límite de la matriz por lo que hay que controlar
                        //la coordenada j (en el triángulo superior da igual pq es un triángulo rectángulo)
                        for (int k = ii + 1; (k < MIN(MIN(tilei + maxTileHeight, subj - exclusionZone), mplength)) &&
                                             (subj < mplength);
                             k++, subj++)
                        {
                            //printf("TID: %d. %2d:%2d\n", tid, k, subj);
                            A[k][subj] = 1;
                            A[subj][k] = 1;
                        }
                        subj = tilej; //subsequence j
                    }
                }
            }
        }
    }
    for (int i = 0; i < mplength; i++)
    {
        printf("%2d: ", i);
        for (int j = 0; j < mplength; j++)
        {
            A[i][j] ? printf("%2d ", j) : printf("   ");
        }
        printf("\n");
    }

    return -1;
}
