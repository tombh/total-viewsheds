/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "AuxFunc.h"
#include "Defs.h"
#include "lodepng.h"
#include <iostream>
#include <sstream>
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

using namespace std;
		

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//				Useful functions
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------


string converInt(int number){ 
string Result;			
ostringstream convert; 
	convert<<number;		
	Result=convert.str();
	if (number<10){
		Result= "00"+ Result;
	}else if (number<100){
		Result= "0"+ Result;
	}
return Result; 
}

double dtime() {
double tseconds = 0.0;
struct timeval mytime;
	gettimeofday(&mytime,(struct timezone *) 0);
	tseconds = (double) (mytime.tv_sec + (double)mytime.tv_usec * 1.0e-6);
return (tseconds) ;
}


//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//                           Graphical output section
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
color *my_palette;

std::vector<unsigned char>  data2vect(float * data, float maxval)
{
std::vector<unsigned char> image;
image.resize(xdim * ydim * 4);
for(unsigned i = 0; i < xdim*ydim; i++)
  {
    int v=1024.0*(data[i]/maxval);
    if(v<0)v=0;
    if(v>1023)v=1023;
    //int x=(i%xdim)*ydim+i/xdim;
    image[4 * i + 0] = my_palette[v].R;
    image[4 * i + 1] = my_palette[v].G;
    image[4 * i + 2] = my_palette[v].B;
    image[4 * i + 3] = 255;
  }
return image;
}
void data2image(const char* filename, float *data ,float maxval)
{
my_palette=build_palette();
printf("aqui\n");
std::vector<unsigned char> image=data2vect(data,maxval);
lodepng::encode(filename, image, xdim, ydim);
delete my_palette;
}

float interp( float val, float y0, float x0, float y1, float x1 ) {
    return (val-x0)*(y1-y0)/(x1-x0) + y0;
}

float base( float val ) {
    if ( val <= -0.75 ) return 0;
    else if ( val <= -0.25 ) return interp( val, 0.0, -0.75, 1.0, -0.25 );
    else if ( val <= 0.25 ) return 1.0;
    else if ( val <= 0.75 ) return interp( val, 1.0, 0.25, 0.0, 0.75 );
    else return 0.0;
}

float red( float gray ) {
    return 256*base( gray - 0.5 );
}
float green( float gray ) {
    return 256*base( gray );
}
float blue( float gray ) {
    return 256*base( gray + 0.5 );
}
color get_color_jet(int index)
{
color c ;
c.R=(int)red(index/1024.0);
c.G=(int)green(index/1024.0);
c.B=(int)blue(index/1024.0);
return c;
}

color get_color_jet2(int index)
{
   float v=index;
   float dv=1024;
   color c={255,255,255};
   if (v < (0.25 * dv)) {
      c.R = 0;
      c.G = 256*(4 * v / dv);
   } else if (v < (0.5 * dv)) {
      c.R = 0;
      c.B = 256*(1 + 4 * (0.25 * dv - v) / dv);
   } else if (v < (0.75 * dv)) {
      c.R = 256*(4 * (v - 0.5 * dv) / dv);
      c.B = 0;
   } else {
      c.G = 256*(1 + 4 * (0.75 * dv - v) / dv);
      c.B = 0;
   }
   return(c);
}

color * build_palette()
{
color* cm = new color[1024];
for (int i = 0; i < 1024; i++)
	cm[i] = get_color_jet2(i);
return cm;
}



//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
// 			Mask file read/write (Cell tower location kernel)
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

void saveCover(bool * c)
{
FILE * f;
fwrite(c,4000000,1,f=fopen("cover.dat","wb"));
fclose(f);
}



bool readCover(bool * c)
{
try{
FILE * f;
fread(c,4000000,1,f=fopen("cover.dat","rb"));
fclose(f);
int cnt=0;
for(int i=0;i<4000000;i++)if(c[i])cnt++;
printf("Cover data read success %d points masked\n",cnt);
return true;
}catch(int E){return false;}
}


//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//			Height file (in most kernel)
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

double *readHeights(char * nmde)
{//Read heights of the points on the map and positions according to order A
        FILE * f;
        unsigned short num;
        double *h=new double[dim*sizeof(double)];
        f=fopen(nmde,"rb");
        if(f==NULL)
        {
		printf("Error opening %s\n",nmde);
        }else
        {
                for(int i=0;i<dim;i++)
                {
                        fread(&num,2,1,f);
                        h[i/xdim+ydim*(i%xdim)]=num/SCALE; //internal representation from top to row
                }
        }
        return h;
}


//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//			Headr for flt arcgis
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
void writeheader(int n, int e, int step, char *filename)
{
char *endian="LSBFIRST";
if(IS_BIG_ENDIAN)strcpy(endian,"MSBFIRST");
FILE *f=fopen(filename,"wt");
fprintf(f,"NROWS        %d\n", xdim);
fprintf(f,"NCOLS        %d\n", ydim);
fprintf(f,"XLLCORNER    %d\n", e);
fprintf(f,"YLLCORNER    %d\n", n-step*ydim);
fprintf(f,"CELLSIZE     %d\n", step);
fprintf(f,"NODATA_VALUE -1.0\n");
fprintf(f,"BYTEORDER    %s\n",endian);
fclose(f);
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//			Get point with max coverage, for cell tower kernel
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

int maxSurf(char * nmds)
{
        FILE * f;
        float num;
        float maxnum=-10000;
        int idx=0;
        f=fopen(nmds,"rb");
        if(f==NULL)
        {
		printf("Error opening %s\n",nmds);
        }else
        {
                for(int i=0;i<dim;i++)
                {
                        fread(&num,4,1,f);
                        if(num<0)num=0;
                        if(num>maxnum){
                                maxnum=num;idx=i;
                                }
                }
        }
        printf("%d %f\n",idx,maxnum);
        return idx;
}

float * readSurf(char * nmds)
{
float *f=new float[dim];
FILE *F=fopen(nmds,"rb");
fread(f,dim,4,F);
fclose(F);
return f;
}

