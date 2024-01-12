#include <stdio.h>
#include <assert.h>

int main(void)  {
    int exclusionZone = 3;
    int maxTileHeight = 6;
    int maxTileWidth = 6;
    int tslength = 14;
    int m = 1;
    assert(maxTileHeight > exclusionZone);
    int mplength = tslength - m + 1;

    char A[mplength][mplength];
    for(int i=0 ; i < mplength; i++) {
        for(int j=0 ; j < mplength; j++) {
            A[i][j] = 0;
        }
    }
    for(int diag=exclusionZone+1; diag<mplength; diag++){
        int i=0;
        int j=diag;
        //printf("%d-%d\n", i,j);
        A[i][j] = 1;
        A[j][i] = 1;
        i=1;
        for(int j=diag+1;j<mplength;j++) {
            //printf("%d-%d\n", i,j);
            A[i][j] = 1;
            A[j][i] = 1;
            i++;
        }
    }
    for(int i=0 ; i < mplength; i++) {
        printf("%2d: ", i);
        for(int j=0 ; j < mplength; j++) {
            A[i][j]? printf("%2d ", j) : printf("   ");
        }
        printf("\n");
    }

    return -1;
}