/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <omp.h>

#define _USE_MATH_DEFINES
using namespace std;

#include "Defs.h"
#include "Sector.h"
#include "AuxFunc.h"

void earth(char *KFN, char *JFN, int utm_n, int utm_e, int Dim, int step );

bool verbose=true;

double *heights;	
int utm_n=4080000,utm_e=360000,step=10;
int obsheight =0;

string pathfsort= "./sort/";		//Directory order file 
string pathfrsect = "./rsector/";	//Directory ring sector file (fullstore only)
char filename[100]; // Input filename
char bas_name[100]; // Input filename without path and extension
char surfname[100]; // Total viewshed result
char surhname[100]; // Total viewshed result (hdr)
char volfname[100]; // Volumetric visibility results
char volhname[100]; // Volumetric visibility results (hdr)
char pngfname[100]; // Graphical output filename
char pngname[100];  // Same, without path
char kmlfname[100]; // Google earth viz file



bool nocompute=false;    // For timing structure manteinance
bool fullstore=false;    // Save all viewshad topologies
bool nostore=false;	 // For timing algorithms
bool volume=false;	 // Volumetric viewshed
bool horizon=false;	 // Horizon kernel
bool coverinput=false;	 // Cell tower kernels
bool coveroutput=false;
bool *coverage_in;
bool *coverage_out;
int tower;

float *sur;	//Total
float *vol;


//==================================================================================
//	Computationally intensive section
//==================================================================================

void compute()
{// Generate files with ring sector of all points  

bool first=false;

#ifdef _OPENMP
omp_set_num_threads(NTHREAD);
if(coveroutput)
omp_set_num_threads(1);
#endif



	
#ifdef _OPENMP
#pragma omp parallel
#endif
	{
//==================================================================================
//	Parallel job
//==================================================================================
float *cumsur;	//Per thread job
float *cumvol;
	int nthreads=1;
	int idt;
#ifdef _OPENMP
	nthreads =omp_get_num_threads();	
	idt=omp_get_thread_num();
#endif


	double timer0,timer1,timer2,timer3,timer4;
	Sector *A;

	if(!coveroutput)
	 A=new Sector(nostore, fullstore, volume);
	
//==================================================================================
// Warning: Coverage section not parallelized
//==================================================================================
	if(coveroutput) {
		A=new Sector(tower);
		printf("Calculating viewshed for tower at %d\n",tower);
		}
//==================================================================================


	A->coverinput=coverinput;
	A->coverage_out=coverage_out; //Revisar. Solo si hay coverage out previo
	A->coverage_in=coverage_in;
	A->setType();
	A->init_storage(); 
	A->setHeights(heights);

	// Per thread storage
	if(!nostore){
		cumsur=new float[dim];
		if(volume) cumvol=new float[dim];
        	for (int i = 0; i < dim; i++) cumsur[i] = 0;
        	if(volume) for (int i = 0; i < dim; i++) cumvol[i] = 0;
		}
	
//==================================================================================
//      Sector loop
//==================================================================================
	int s=0; int ss=0, nsect=180;
	timer0=dtime();
#ifdef _OPENMP
	nsect=180/nthreads;
	//nsect=1;
	ss=omp_get_thread_num()*nsect;
#endif
	for(s=ss;s<ss+nsect;s++){ //sector loop
        	FILE *fs;
	       	string nfile;
		nfile= pathfsort+"nn_list_" + converInt(s) + ".bin";
		fs=fopen(nfile.c_str(),"rb");

		A->listfileR = fs;         
		A->change(s);
		timer1=dtime();
		A->loop();
		timer2=dtime();
		fclose(fs);                
	        if(verbose) cout<<"calculated  sector:"<<s<<endl;
		if(!nostore) {
		        string fnin= pathfrsect+ "/rsector_" + converInt(s) + ".bin";
			if (fullstore) A->recordsectorRS(fnin);
        		for (int i = 0; i < dim; i++) cumsur[i]+=A->surfaceF[i]+A->surfaceB[i]; 
        		if(volume) 
				for (int i = 0; i < dim; i++) cumvol[i] +=A->volumeF[i]+A->volumeB[i];
			}
	        if(verbose)printf("Thread %02d - T=%f\n",idt, (timer2-timer1));
		} //end sector loop
//
//==================================================================================

if(coveroutput){
			int cntx=0;
			for(int i=0;i<dim;i++){
				if(A->coverage_out[i])cntx++;
				if(heights[i]<=0)A->coverage_out[i]=true;//No need for coverage at sea level
				}
			saveCover(A->coverage_out);
		if(verbose)printf("Total cuvertura %d \n",cntx);
}

//==================================================================================
// Data Collection from threads WEST to EAST order is now recovered
//==================================================================================
if(!nostore){
#ifdef _OPENMP
#pragma omp critical
	{
#endif
	if(verbose)printf("Thread %02d - T=%f\n",idt, (timer2-timer0));
	if(first==false){
		for(int i=0;i<dim;i++)sur[i]=cumsur[(i%xdim)*ydim+(i/xdim)];
		if(volume)
			for(int i=0;i<dim;i++)vol[i]=cumvol[(i%xdim)*ydim+(i/xdim)];
		first=true;
		}
	else
		{
		for(int i=0;i<dim;i++)sur[i]+=cumsur[(i%xdim)*ydim+(i/xdim)];
		if(volume)
			for(int i=0;i<dim;i++)vol[i]+=cumvol[(i%xdim)*ydim+(i/xdim)];
		}
#ifdef _OPENMP
	}
#endif
	delete cumsur;
	if(volume)delete cumvol;
	}
//==================================================================================
// End Parallel section
//==================================================================================

delete A;
} //end parallel


//==================================================================================
//	End Compute
//==================================================================================
}

//==================================================================================
//      Allocate, deallocate & Store results
//==================================================================================

void allocate()
{
// Algorithm to calculate coverage. Comment second line. No parallelized
if(coveroutput){
	printf("Reading coverage as output\n");
	nostore=true;
	coverage_out=new bool[dim];
	if(coverinput)coverinput=readCover(coverage_out);
 
 
	}

// Allocate data
if(!nostore){
	sur=new float[dim];
	if(volume) vol=new float[dim];
	}

if(coverinput)
	{
	printf("Reading coverage as input\n");
	coverage_in=new bool[dim];
	coverinput=readCover(coverage_in);
	}
}


// Free memory and store results
void deallocate()
{
if(!nostore){
	printf("storing results...\n");
	string path="./output/";
       	FILE *fs;

       	fs=fopen(surfname,"wb");
       	fwrite(sur,dim,4,fs);
       	fclose(fs);
	writeheader(utm_n,utm_e,step,surhname);
	delete sur;
       	if(volume){
       		fs=fopen(volfname,"wb");
       		fwrite(vol,dim,4,fs);
       		fclose(fs);
	        writeheader(utm_n,utm_e,step,volhname);
	        delete vol;
		}
	}
}



//==================================================================================
//	Filenames
//==================================================================================

void setfilenames(int argc, char *argv[])
{
if(argc>1){
	sscanf(argv[1],"%d_%d_%d",&utm_n,&utm_e,&step);
	sprintf(bas_name,"%07d_%07d_%03d",utm_n,utm_e,step);
	}
else
	strcpy(bas_name,"4080000_0360000_010");
sprintf(filename,"input/%s.bil",bas_name);
sprintf(surfname,"output/%s_sur.flt",bas_name);
sprintf(surhname,"output/%s_sur.hdr",bas_name);
sprintf(volfname,"output/%s_vol.flt",bas_name);
sprintf(volhname,"output/%s_vol.hdr",bas_name);
sprintf(pngfname,"output/%s_sur.png",bas_name);
sprintf(pngname,"%s_sur.png",bas_name);
sprintf(kmlfname,"output/%s_sur.kml",bas_name);
if(volume){
sprintf(pngfname,"output/%s_vol.png",bas_name);
sprintf(pngname,"%s_vol.png",bas_name);
sprintf(kmlfname,"output/%s_vol.kml",bas_name);
}
}




//==================================================================================
//	MAIN
//==================================================================================



int main(int argc,char*argv[])
{	
//------------------------------------------------------------
//	Kernel selection (usually, viewshed)
//------------------------------------------------------------

volume=false;
coveroutput=false;
coverinput=false;
nostore=false;
fullstore=false;

setfilenames(argc,argv);

//------------------------------------------------------------
//------------------------------------------------------------
//	Input values
//------------------------------------------------------------

if(coverinput)
	try{
		tower=maxSurf(surfname);				
	}catch(int E) { }
printf("Input file: %s\n",filename);
if(!nocompute)
	heights=readHeights(filename);

// warning xdim  ydim   should be determined from model

//------------------------------------------------------------
//	Just do it!
//------------------------------------------------------------

allocate();
if(verbose)printf("Start computing\n");
compute();
if(verbose)printf("End computing\n");
deallocate();
if(verbose)printf("End write results\n");

//------------------------------------------------------------
//------------------------------------------------------------

if(true){
	float * surf=readSurf(surfname);
	data2image(pngfname,surf, 10000);
	earth(kmlfname,pngname, utm_n,utm_e,xdim,step);
}

//------------------------------------------------------------
//	Cleaning tasks
//------------------------------------------------------------

if(!nocompute)
	delete heights;
exit(0);
}
