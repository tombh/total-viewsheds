/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/

#include "Defs.h"
#include "LinkedList.h"
#include <string>

#ifndef SECTOR_H
#define SECTOR_H
 
class Sector{
        
	public:
	FILE *listfileW;	//Pointer to file write
	FILE *listfileR;	//Pointer to file read
		
	int sector;		//Number sector
        
        node *nodes;		//Pointer nodes
	node newnode;		//New node of Linked List
	node newnode_trans;	//transitory node

        int bw; 		//Size windows sweep
	int hbw ; 		//Size Windows sweep/2
	LinkedList MyLinkedList;//Linked list used
		
	int type; //kernel selection
	
	//Computational stage
	bool precomputed;   	//If precomputed is false the files ordering no have been generated
   	bool nocompute;		//If nocompute is false no have been calculate de ring sectors
   	bool nostore;		//If nostore is false no have been genereated the file with ring sector
	bool fullstore;
	bool vectorize;

	bool coverstore;        //Store coverage data for one tower
	bool coverinput;
	bool *coverage_in;
	bool *coverage_out;
	int towerloc;
	int cntcoverage;
	bool pointeval(int i, int cpx, int cpy);
	float  distance(int x1, int x2, int y1, int y2);
	float  distance(int i, int j);
	float *lsF, *lsB;
	bool belongtoFsector(float angle);
	bool belongtoBsector(float angle);
	void save_cover();

	float *delta_d_vec;
	float *height_vec;
        bool *above_vec;
        int *sweeppt_vec;

		
	int *orderAO, *orderAT, *orderA, *orderiA, *orderAOT, *orderiAT; // Pointers orderings
  
	double origin; 		//The angle windows sweep used         
	double sect_angle;	//The angle windows sweep of sector used
	double sect_angle_g;	//The angle windows sweep of sector used
        double *isin, *icos, *icot, *itan;//Trigonometric variable 
        int *tmp1, *tmp2;	//Temporal poninters 

        int **rsectorF;	//Rings sector rigth nodes
        int **rsectorB; 	//Ring sector left nodes
	unsigned short *size_dsF;//size number sector rigth
	unsigned short *size_dsB;//size number sector rigth
	float *surfaceF;
	float *surfaceB;
	float *volumeF;
	float *volumeB;
        bool volume;	

	
        int quad;		//Quadrant of maps
       			
	double obs_h;		//Height at which an observer is
        
	
	bool foundn;		//Place to insert new node
	bool foundn_trans;	//Place to insert nes node
	bool foundd;		//Next dead
	bool foundd_trans;	//Next dad
	bool foundm;		//next main node
	bool atright;
	bool atright_trans;
		
	int NEW_position;	//Position new node
	int NEW_position_trans; //Auxliary new position node 
	int POV;		//Position actuallly node
	int NextPOV;		//Next position node Linked List

             
        //Sweep variables
        float open_delta_d;
        float delta_d;
        float open_delta_h;
        float delta_h;
        bool visible;
        float sur_dataF;
        float vol_dataF;
        float sur_dataB;
        float vol_dataB;
	float max_angle;
	int sweeppt;
	//int sweep;
        int cnt_vec;

	int rsF[1000][2]; //Temporal storage of visible ring sector up to 1000(large enough)
        int rsB[1000][2]; //Temporal storage of visible ring sector up to 1000(large enough)
        int nrsF;//Number ring sector to right point
        int nrsB;//Number ting sector to left point





		
	//FUNCTIONS**********************************************************************************************************		        
	//
	Sector();		//Create Sector.		
	Sector(int I);		//Create Sector. Tower coverage calc
	Sector(bool a, bool b, bool c);		//Create Sector.		
	~Sector();		//dISPOSE Sector.		

        void pre_computed_sin();//Calculate  the trigonometric variable sino
        void change(int n);	//Change the Sector of 0 to 179
        void setDistances(int sector);//Calculated distance from the axis starting
     
        void setHeights(double heights[]);//Gives each node its natural position and elevation
        void presort();       	//Ordering  based paper
        void sort();		//Ordering  based paper
     
	void loop(); //Analisys files if files are generated
	void loop(int i); //Given a tower, compute coverage file
	//if coverage file exist, add coverage
	//
        void post_loop(int i, bool opening, bool closing);//Operations after analysis
              
	void storers(int i, bool volume, bool fulldata); // Storage dates of all ring sector this sector
	void storers(); // same just for tower point
       
	void calcule_pos_newnode(bool remove); //Calculated the position the new node in the linked List
	void insert_node(node newnode, int position, bool remove);// Insert and remove node of linded list
         
	int presweepF();
	int presweepB();
	void sweepS(int i);    // Performs sweep node of sector (surface kernel);
	void sweepC(int i);    // Performs sweep node of sector (surface kernel with already-covered areas);
	void sweepV(int i);    // Performs sweep node of sector (surface and volume kernel) ;
	void sweepS_vec(int i);    // Performs sweep node of sector (surface and volume kernel) ;
        void kernelV(int rs[][2], int &nrs, float &sur_data, float &vol_data);
        void kernelS(int rs[][2], int &nrs, float &sur_data);
        void kernelS_vec(int rs[][2], int &nrs, float &sur_data);
	void kernel_select(int &i, int type);

	void init_vec(int hbw);
	void dispose_vec();
	void default_parms();
	void init_storage();
	void setType();

        void closeprof(bool visible, bool fwd, bool vol, bool full);
        //void closeprof(bool visible, int rs[1000][2], int &nrs, bool forward, bool volume);//
	void recordsectorRS(string fn); //Record dates rings sectors in files


   
};
#endif
