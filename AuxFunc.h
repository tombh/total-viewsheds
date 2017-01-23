/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/

// auxiliary functions


#include <string>
#include <time.h>
#include <sys/time.h>



#ifndef AUXFUN_H
#define AUXFUN_H

using namespace std;


struct color
{
     unsigned char R;
     unsigned char G;
     unsigned char B;
};



string converInt(int number); // Convert the int to string
double dtime();
bool readCover(bool *data);
void saveCover(bool *data);
double *readHeights(char *x);
int maxSurf(char * x);
color *build_palette();
void data2image(const char* filename, float *data ,float maxval=10000);
float * readSurf(char *nmds);
void writeheader(int n, int e, int step, char *filename);

#endif
