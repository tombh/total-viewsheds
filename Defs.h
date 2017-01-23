/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/


// Definition of structures and variables used.

#include <math.h>

#ifndef DEFS_H

	// dim =2000*2000; size maps
	// xdim=2000;	   size axis x of maps 
	// ydim=2000;	   size axis y of maps	
	// scale=10;       input map precision 10meter
	// obs_h=10;       observer's height in meters
	
#define DEFS_H
#define xdim 2000
#define ydim 2000
#define dim 4000000
#define SCALE 10.0
#define OBS_H 1.5
#define NTHREAD 12

	// Initial sector shift in degrees, to avoid point alignment
	// Bandwith and half bandwith of data structure
	// Order of sqrt(dim)
	
#define SHIFT 0.001
#define BW 1001

using namespace std;
		
const double PI = M_PI;				
const double mpi= M_PI/2;			
const double tmpi= 3*M_PI/2;
const double tograd = 360/(2*M_PI);	// conver const of radians to degree	
const double torad =(2*M_PI)/360;		// conver const of degree to radians
const float  PI_F=3.14159265358979f;

	

	//Struct node
	struct node {	//	This structure has the information of one point
        int idx;	//	identifier of poit.
        int oa;		//	orden given the point in our algorithm.
        float h;	//	height at that id it.
        float d;	//	distance the point from axis x.
    	};

	
	// Node used in the Linkded List
	struct LinkedListNode{ //It is a node whit next an previous identifier
		public:		
		node Value;
        short next;// Identifier the next node in Linked List.
        short prev;// Identifier the Previous node in Linked List.
	};

	
	


#endif
