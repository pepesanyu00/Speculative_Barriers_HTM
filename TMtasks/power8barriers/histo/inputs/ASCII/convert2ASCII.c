#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: convert2ASCII [-r] input > output\n   -r: the size of the input is reduced to be used in GEMS\n");
    return -1;
  }
  int reduce = 0;
  if (argc == 3) {
    reduce = 1;
  } 

  unsigned int img_width, img_height;
  unsigned int histo_width, histo_height;

  FILE* f = fopen(argv[1],"rb");
  int result = 0;

  result += fread(&img_width,    sizeof(unsigned int), 1, f);
  result += fread(&img_height,   sizeof(unsigned int), 1, f);
  result += fread(&histo_width,  sizeof(unsigned int), 1, f);
  result += fread(&histo_height, sizeof(unsigned int), 1, f);

  if (result != 4){
    fputs("Error reading input and output dimensions from file\n", stderr);
    return -1;
  }

  unsigned int* img = (unsigned int*) malloc (img_width*img_height*sizeof(unsigned int));
  
  result = fread(img, sizeof(unsigned int), img_width*img_height, f);
  fclose(f);
  
  if (reduce) {
    printf("%d %d\n",img_width/2, img_height/2); //Divido entre dos cada dimension de la imagen: de 996x1040 a 498x520
    printf("%d %d\n",histo_width, histo_height/8); //Divido height entre 8 --> 256x4096 pasa a ser 256x512
    for(int i=0;i < img_width*img_height/4;i++) //Cojo los 498x520 puntos de la imagen
      printf("%d\n",img[i]/8); //Divido los valores entre 8 (los valores son los idices del histograma)
  } else {
    printf("%d %d\n",img_width, img_height);
    printf("%d %d\n",histo_width, histo_height);
    for(int i=0;i < img_width*img_height;i++)
      printf("%d\n",img[i]);
  }
  return 0;
}
