#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int read_data(int nx, int ny, int nz, FILE *fp) {
  float res;
  int i, j, k;
  int out;
  for (i = 0; i < nz; i++) {
    for (j = 0; j < ny; j++) {
      for (k = 0; k < nx; k++) {
        out = fread(&res, sizeof (float), 1, fp);
        assert(out);
        printf("%f\n",res);
      }
    }
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    printf("Usage: convert2ASCII input nx ny nz > output\n");
    return -1;
  }

  int nx = atoi(argv[2]);
  int ny = atoi(argv[3]);
  int nz = atoi(argv[4]);

  FILE* f = fopen(argv[1],"rb");

  if (f == NULL){
    fputs("Error opening file\n", stderr);
    return -1;
  }

  read_data(nx, ny, nz, f);
  return 0;
}
