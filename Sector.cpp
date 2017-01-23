/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/

#include <stdio.h>
#include <omp.h>
//#include <mmintrin.h>
#include "Sector.h"
//#include "AuxFunc.h"


using namespace std;
	        

void Sector::default_parms()
{
	bw = BW; 
	hbw = (bw-1)/2; 
	origin = SHIFT; 
	obs_h=OBS_H/SCALE;
}

Sector::Sector()
{ 
	default_parms();
        precomputed = true;
        nocompute=true;
        nostore=true;
        fullstore=true;
        volume=false;
}


Sector::Sector(int i)
{
	default_parms();
        precomputed = true;
        nocompute=true;
        nostore=true;
        fullstore=true;
        volume=false;
        coverstore=true;
        towerloc=i;
        printf("%d %d\n",i,towerloc);
}

// Surface, Volume and Horizon kernels
Sector::Sector(bool nostore_i, bool fullstore_i, bool volume_i)
{ 
	default_parms();
        vectorize=true;
        vectorize=false;
        precomputed = true;
        nocompute=false;
        coverstore=false;
	coverinput=false;
        nostore=nostore_i;
        fullstore=fullstore_i;
        volume=volume_i;
 }
	// ============================================================================================
	// ============================================================================================
  // Initial tasks:
	// ============================================================================================
	// ============================================================================================

void Sector::init_storage()
{
nodes= new node[dim];
MyLinkedList = LinkedList(bw); 

isin = new double[dim]; 
icos = new double[dim]; 
icot = new double[dim]; 
itan = new double[dim]; 
orderiAT = new int[dim];
tmp1 = new int[dim]; 
tmp2 = new int[dim];
if(!nostore){
		surfaceF=new float[dim];
		surfaceB=new float[dim];
		if(volume)volumeF=new float[dim];
		if(volume)volumeB=new float[dim];
        	if(fullstore){
        		rsectorF = new int*[dim];
        		rsectorB = new int*[dim];
        		size_dsF=new unsigned short[dim];
        		size_dsB=new unsigned short[dim];
        		}
		}
if(coverstore){
		coverage_out=new bool[dim];
    rsectorF = new int*[1];
    rsectorB = new int*[1];
    size_dsF=new unsigned short[1];
    size_dsB=new unsigned short[1];
	  }
if(vectorize)init_vec(bw);	
}

void Sector::setHeights(double *heights)
{
for (int i = 0; i < dim; i++) {
	nodes[i].idx = i;
       	nodes[i].h = (float)heights[i]; //decameters
     	}
}
 
	// ============================================================================================
	// ============================================================================================
  // Finishing tasks:
	// ============================================================================================
	// ============================================================================================

Sector::~Sector()
{
delete isin;
delete icos;
delete icot;
delete itan;

delete orderiAT;
delete tmp1;
delete tmp2;

if(!nostore){
        delete surfaceB;
        delete surfaceF;
        if(volume)delete volumeB;
        if(volume)delete volumeF;
        if(fullstore||coverstore){
         	delete rsectorF;
         	delete rsectorB;
         	delete size_dsF;
         	delete size_dsB;
	 	}
if(coverstore)delete coverage_out;
	//if(coverinput)delete coverage_in;
	}
if(vectorize)dispose_vec();
}



	// ============================================================================================
	// ============================================================================================
  // per sector specific functions:
	// ============================================================================================
	// ============================================================================================

//Task to do when a new direction is taken
void Sector::change(int n)
{
sector = n;
sect_angle_g=(origin+sector+0.5);
quad = (sector >= 90) ? 1 : 0;
if (quad == 1) sector -= 90; 
pre_computed_sin();
sort();
if (quad == 1) sector += 90; 
setDistances(sector);
MyLinkedList.Clear();
MyLinkedList.FirstNode(nodes[orderiAT[0]]);
}

//precomputed values for trigonometry
void Sector::pre_computed_sin()
{
double ac, al, ar;
double s, c, ct, tn;

ac = (origin + sector + 0.5) * torad;
al = (origin + sector + 0.0) * torad;
ar = (origin + sector + 1.0) * torad;
sect_angle = ac;
s = sin(sect_angle); 
c = cos(sect_angle); 
tn = tan(sect_angle); 
ct = 1 / tn;

for (int i = 0; i < dim; i++)
{
    icos[i] = i * c;
    isin[i] = i * s;
    icot[i] = i * ct;
    itan[i] = i * tn;
 }
}

        

//Distance to segment over point 0 (NW)
void Sector::setDistances(int sector)
{
double val;
for (int j = 0; j < xdim; j++)
	for (int i = 0; i < ydim; i++)
		{
		val= icos[j] + isin[i];
            	if (quad == 1) val = icos[i] - isin[j];
		nodes[j * ydim + i].d = val;
    }
}   



void Sector::presort()
{ 
double ct,tn;
tmp1[0] = 0;
tmp2[0] = 0;
	
tn = tan(sect_angle); 
ct = 1 / tn;
for (int j = 1; j < xdim; j++)
	tmp1[j] = tmp1[j - 1] + (int)min(ydim, (int)floor(ct * j));
for (int i = 1; i < ydim; i++)
        tmp2[i] = tmp2[i - 1] + (int)min(xdim, (int)floor(tn * i));
}
      

void Sector::sort()
{
presort();
double lx = xdim - 1;
double ly = ydim - 1;
double x,y;
int ind,xx,yy,p,np;
for (int j = 1; j <= xdim; j++)
{
x = (j - 1);
    for (int i = 1; i <= ydim; i++)
        {
        y = (i - 1);
        ind = i * j;
        ind += ((ly - y) < (icot[j - 1])) ? ((ydim - i) * j - tmp2[ydim - i] - (ydim - i)) : tmp1[j - 1];
        ind += ((lx - x) < (itan[i - 1])) ? ((xdim - j) * i - tmp1[xdim - j] - (xdim - j)) : tmp2[i - 1];
        xx = j - 1;
        yy = i - 1;
        p = xx * ydim + yy;
        np = (xdim - 1 - yy) * ydim + xx;
        if (quad == 0)
        {
            nodes[p].oa = ind - 1;
            orderiAT[ind - 1] = np;
        }else
        {
            nodes[np].oa = ind - 1;
            orderiAT[dim - ind] = p;
        }
    }
    
}
       
}

	// ============================================================================================
	// ============================================================================================
	// Data structure manteinance
	// ============================================================================================
	// ============================================================================================


void Sector::post_loop(int i, bool opening, bool closing)
{
	atright =false;
	atright_trans =false;
	NEW_position = -1;
	NEW_position_trans = -1;
	node tmp = newnode_trans;
	
	if(!closing)
	{
		newnode = nodes[orderiAT[opening ? 2*i+1:hbw+i+1]];
		atright = newnode.oa > MyLinkedList.LL[POV].Value.oa;
	}
	if (opening)
	{
		newnode_trans = nodes[orderiAT [2*i+2]];
		atright_trans = newnode_trans.oa > MyLinkedList.LL[POV].Value.oa;
	}	
	
  if(precomputed){
            if (!closing)
            {
              fread(&NEW_position,4,1,listfileR);
              insert_node(newnode, NEW_position, !opening);
            }
            if(opening)
            {
             fread(&NEW_position_trans,4,1,listfileR);
             insert_node(newnode_trans,NEW_position_trans,false);
            }
		}
	else
            {
            if (!opening&&!closing)
                          {
			                newnode = nodes[orderiAT[opening ? 2 * i + 1 : hbw + i + 1]];
                    	NEW_position = -1;
                    	atright = newnode.oa > MyLinkedList.LL[POV].Value.oa;
                    	calcule_pos_newnode(true);
              }
            if (opening){
                        newnode = nodes[orderiAT[2 * i + 1]];
                        NEW_position = -1;
                        atright = newnode.oa > MyLinkedList.LL[POV].Value.oa;
                        calcule_pos_newnode(false);
                        newnode = nodes[orderiAT[2 * i + 2]];
                        atright = newnode.oa > MyLinkedList.LL[POV].Value.oa;
                        calcule_pos_newnode(false);
                        }
		}
	if (closing) MyLinkedList.Remove_two();
 }

void Sector::calcule_pos_newnode(bool remove)
{
            int sweep = 0;
            if (newnode.oa < MyLinkedList.LL[MyLinkedList.First].Value.oa)
            {
                NEW_position = -2; // Before first
                MyLinkedList.AddFirst(newnode, remove);//MyLL
                fwrite(&NEW_position,4,1,listfileW);
                sweep = MyLinkedList.First;
            }
            else if (newnode.oa > MyLinkedList.LL[MyLinkedList.Last].Value.oa)
            {
                NEW_position = -1;
                MyLinkedList.AddLast(newnode, remove);
                fwrite(&NEW_position,4,1,listfileW);
            }

            else
            {
                sweep = MyLinkedList.LL[MyLinkedList.First].next;
                bool go_on = true;
                do
                {
                    if (newnode.oa < MyLinkedList.LL[sweep].Value.oa)
                    {
                        NEW_position = MyLinkedList.LL[sweep].prev;
                        MyLinkedList.Add(newnode, NEW_position, remove);//MyLL
                        fwrite(&NEW_position,4,1,listfileW);
                        sweep = MyLinkedList.LL[NEW_position].next;
                        go_on = false;
                    }
                    else
                        sweep = MyLinkedList.LL[sweep].next;
                } while (go_on);
            }
}


void Sector::insert_node(node newnode, int position, bool remove)
{
	if(position > -1)
	{
		 MyLinkedList.Add(newnode,position,remove);
	}else
	{
		if(position==-1)
		{
			MyLinkedList.AddLast(newnode,remove);
		}else
		{
			MyLinkedList.AddFirst(newnode,remove);
		}
	}
	 
}



	// ============================================================================================
	// ============================================================================================
	// Main loop accross point
	// ============================================================================================
	// ============================================================================================

void Sector::setType()
{
type=1;
if(vectorize)type=3;
if(volume)type=4;
if(coverstore)type=2;
if(coverinput)type=5;
if(nocompute&&!coverstore)type=0;
printf("Type %d\n",type);
}


void Sector::loop()
{
bool opening,closing;
for(int i=0; i<dim-1; i++) {
	POV=i%bw;  
	kernel_select(i, type);
	opening= (i<hbw);
	closing= (i>= (dim-hbw-1));
	post_loop(i,opening,closing);
	}
}




	// ============================================================================================
	// ============================================================================================
  // End of direction specific functions: Store results
	// ============================================================================================
	// ============================================================================================


//Fulldata to store ring sector topology 
//
void Sector::storers(int i, bool volume, bool fulldata)
{
float surscale=PI_F/(360*SCALE*SCALE);  //results in hectometers ^2 ^3
surfaceF[i]=sur_dataF * surscale ;
surfaceB[i]=sur_dataB * surscale ;

if(volume)
{
float volscale=surscale/(3*SCALE*SCALE*SCALE);
volumeF[i]=vol_dataF * volscale ;
volumeB[i]=vol_dataB * volscale ;
}
if(fulldata){
	rsectorF[i] = new int[nrsF * 2];
	rsectorB[i] = new int[nrsB * 2];
	size_dsF[i]=2*nrsF;
	size_dsB[i]=2*nrsB;
	for (int j = 0; j < nrsF; j++)
	{
		rsectorF[i][2 * j + 0] = rsF[j][0];
		rsectorF[i][2 * j + 1] = rsF[j][1];
	}
	for (int j = 0; j < nrsB; j++)
	{
		rsectorB[i][2 * j + 0] = rsB[j][0];
		rsectorB[i][2 * j + 1] = rsB[j][1];
	}
}
}


void Sector::storers()
{
	rsectorF[0] = new int[nrsF * 2];
	rsectorB[0] = new int[nrsB * 2];
	size_dsF[0]=2*nrsF;
	size_dsB[0]=2*nrsB;
	for (int j = 0; j < nrsF; j++)
	{
		rsectorF[0][2 * j + 0] = rsF[j][0];
		rsectorF[0][2 * j + 1] = rsF[j][1];
	}
	for (int j = 0; j < nrsB; j++)
	{
		rsectorB[0][2 * j + 0] = rsB[j][0];
		rsectorB[0][2 * j + 1] = rsB[j][1];
	}
	printf("%03d %03d\n",2*nrsF,2*nrsB);
}



// When fullstore active, viewshed geometry is saved
//
void Sector::recordsectorRS(string  name)
{
	FILE *fs;
	const char *c = name.c_str();
        rsectorF[orderiAT[dim - 1]] = new int[0];
        rsectorF[orderiAT[dim - 1]] = new int[0];
        size_dsB[orderiAT[dim - 1]]=0;
        size_dsB[orderiAT[dim - 1]]=0;
	fs=fopen(c,"wb");
	
	for (int i = 0;i<dim;i++)
	{
		int j = size_dsF[i];
		fwrite(&j,4,1,fs);
		for (int k = 0;k <j;k++)
		{
			fwrite(&rsectorF[i][k],4,1,fs);
		}
		
		j = size_dsB[i];
		fwrite(&j,4,1,fs);
		for (int k = 0;k<j;k++)
		{
			fwrite(&rsectorB[i][k],4,1,fs);
		}
		
	}
	
	fclose(fs);
	
	for(int i=0; i<dim; i++)
	{
		delete rsectorF[i];
		delete rsectorB[i];
	}
		
}











	// ============================================================================================
	// ==========================  Kernels         =======================================
	// ============================================================================================

  // Sector oriented sweep algorithm can be used with different kernels
  // Wrapping sweep procedure, kernel procedure and additional functions:


void Sector::kernel_select(int &i, int type)
{
switch(type)
{
	case 0:
	break;

	case 1:
	sweepS(i);
	if(!nostore)storers(orderiAT[i],false,fullstore);
	break;

	case 2:
	if(orderiAT[i]==towerloc){
		printf("Towerfound\n");
		sweepS(i);
		save_cover();
		i=dim;
		}
	break;

	case 3:
	sweepS_vec(i);
	break;

	case 4:
	sweepV(i);
	if(!nostore)storers(orderiAT[i],true,fullstore);
	break;

	case 5:
	sweepC(i);
	if(!nostore)storers(orderiAT[i],false,fullstore);
	break;

}
}


	// ============================================================================================
	// ==========================  Kernel 1: Simple viewshed
	// ============================================================================================



void Sector::sweepS(int i)
{
	float d = MyLinkedList.LL[POV].Value.d;
	float h = MyLinkedList.LL[POV].Value.h + obs_h;


	//printf("i:  %d  sweeppt:  %d\n",i,sweeppt);
	int sweep=presweepF();
	if (POV != MyLinkedList.Last) do{
                    delta_d = MyLinkedList.LL[sweep].Value.d - d; 
                    delta_h = MyLinkedList.LL[sweep].Value.h - h;
                    sweeppt = MyLinkedList.LL[sweep].Value.idx;
                    kernelS(rsF,nrsF,sur_dataF);
			//if(espt==0)printf("%f\t%f\t%f\t%d\n",delta_d,delta_h,h,visible);
                    sweep = MyLinkedList.LL[sweep].next;
	}while(sweep!=-1);

	closeprof(visible,true,false,false);

	if(sweeppt==-1){
	printf("i:  %d  sweeppt:  %d\n",i,sweeppt);
	printf("%f %f\n",d,h);
	printf("%d %f \n",nrsF ,sur_dataF);
	}

	sweep=presweepB();
	if (POV != MyLinkedList.First) do{
                    delta_d = d-MyLinkedList.LL[sweep].Value.d; 
                    delta_h = MyLinkedList.LL[sweep].Value.h - h;
                    sweeppt = MyLinkedList.LL[sweep].Value.idx;
                    kernelS(rsB,nrsB, sur_dataB);
                    sweep = MyLinkedList.LL[sweep].prev;
	}while(sweep!=-2);
	//printf("i:  %d  sweeppt:  %d\n",i,sweeppt);

	closeprof(visible,false,false,false);
}

        void Sector::kernelS(int rs[][2], int &nrs, float &sur_data)
        {
        float angle = delta_h/delta_d;
        bool above = (angle > max_angle);
        bool opening = above && (!visible);
        bool closing = (!above) && (visible);
        visible = above;
        max_angle = max(angle, max_angle);
        if (opening)
        {
                open_delta_d = delta_d;
                nrs++;
                rs[nrs][0] = sweeppt;
        }
        if (closing)
        {
                sur_data += (delta_d * delta_d - open_delta_d * open_delta_d);
                rs[nrs][1] = sweeppt;
        }
        }
        
        
        

	// ============================================================================================
	// ==========================  Kernel 2: Simple viewshed - vectorized (not recommended)
	// ============================================================================================

  //Failed attempt. Very expensive, as vectorization requires additional storage and
  //produces cache faults

void Sector::sweepS_vec(int i)
{
	float d = MyLinkedList.LL[POV].Value.d;
	float h = MyLinkedList.LL[POV].Value.h + obs_h;
	cnt_vec=0;
	int pos;
	int sweep=presweepF();
	if (POV != MyLinkedList.Last) do{
		    //sweeppt_vec[cnt_vec]=MyLinkedList.LL[sweep].Value.idx;
		    delta_d_vec[cnt_vec]=MyLinkedList.LL[sweep].Value.d;
		    height_vec[cnt_vec++]=MyLinkedList.LL[sweep].Value.h;
                    sweep = MyLinkedList.LL[sweep].next;
	}while(sweep!=-1);
	pos=cnt_vec;
	sweep=presweepB();
	if (POV != MyLinkedList.First) do{
		    //sweeppt_vec[cnt_vec]=MyLinkedList.LL[sweep].Value.idx;
		    delta_d_vec[cnt_vec]=MyLinkedList.LL[sweep].Value.d;
		    height_vec[cnt_vec++]=MyLinkedList.LL[sweep].Value.h;
                    sweep = MyLinkedList.LL[sweep].prev;
	}while(sweep!=-2);

//	for(int j=0;j<BW-1;j++)

	return;
	visible=true;
	max_angle=-2000;
	open_delta_d=0;
	delta_d=0;
#pragma ivdep
	for(int j=0;j<pos;j++){
		delta_d_vec[j]-=d;
		delta_d_vec[j]/=(height_vec[j]-h);
		/*
                bool above = (angle > max_angle);
                bool opening = above && (!visible);
                bool closing = (!above) && (visible);
                visible = above;
        	if (opening)
        	{
                	open_delta_d = delta_d;
                	nrsF++;
                	rsF[nrsF][0] = MyLinkedList.LL[sweeppt_vec[j]].Value.idx;
        	}
        	if (closing)
        	{
                	sur_dataF += (delta_d * delta_d - open_delta_d * open_delta_d);
                	rsF[nrsF][1] =  MyLinkedList.LL[sweeppt_vec[j]].Value.idx;
        	}*/
		}
	closeprof(visible,true,false,false);
	visible=true;
	max_angle=-2000;
	open_delta_d=0;

	delta_d=0;
#pragma ivdep
	for(int j=pos;j<cnt_vec;j++){
		delta_d_vec[j]-=d;
		delta_d_vec[j]/=(height_vec[j]-h);
		//float delta_d=(d-MyLinkedList.LL[sweeppt_vec[j]].Value.d);
		//height_vec[j]=(MyLinkedList.LL[sweeppt_vec[j]].Value.h-h)/delta_d;
		/*
                bool above = (angle > max_angle);
                bool opening = above && (!visible);
                bool closing = (!above) && (visible);
                visible = above;
 		if (opening)
        	{
                	open_delta_d = delta_d;
                	nrsB++;
                	rsB[nrsB][0] = MyLinkedList.LL[sweeppt_vec[j]].Value.idx;
        	}
        	if (closing)
        	{
                	sur_dataB += (delta_d * delta_d - open_delta_d * open_delta_d);
                	rsB[nrsB][1] =  MyLinkedList.LL[sweeppt_vec[j]].Value.idx;
        	}*/
		}
	closeprof(visible,false,false,false);



/*
*/
}


  void Sector::kernelS_vec(int rs[][2], int &nrs, float &sur_data)
  {
  bool opening = above_vec[cnt_vec] && (!visible);
  bool closing = (!above_vec[cnt_vec]) && (visible);
  visible = above_vec[cnt_vec];
  if (opening)
  {
          open_delta_d = delta_d_vec[cnt_vec];
          nrs++;
          rs[nrs][0] = sweeppt_vec[cnt_vec];
  }
  if (closing)
  {
          sur_data += (delta_d_vec[cnt_vec] * delta_d_vec[cnt_vec] - open_delta_d * open_delta_d);
          rs[nrs][1] = sweeppt_vec[cnt_vec];
  }
  }
  
  
	void Sector::init_vec(int bw)
	{
	above_vec= new bool[bw];
	delta_d_vec= new float[bw];
	height_vec= new float[bw];
	sweeppt_vec= new int[bw];
	}
	void Sector::dispose_vec()
	{
	delete above_vec;
	delete delta_d_vec;
	delete sweeppt_vec;
	delete height_vec;
	}


	// ============================================================================================
	// ==========================  Kernel 3: Volumetric viewshed
	// ============================================================================================


void Sector::sweepV(int i)
{
	float d = MyLinkedList.LL[POV].Value.d;
	float h = MyLinkedList.LL[POV].Value.h + obs_h;
	int sweep;

	visible=true;
	nrsF=0;
	sur_dataF=0;
	vol_dataF=0;
	delta_d=0;
	delta_h=0;
	open_delta_d=0;
	open_delta_h=0;
	sweep = MyLinkedList.LL[POV].next;
	sweeppt= MyLinkedList.LL[POV].Value.idx;
	rsF[0][0]=sweeppt;
	max_angle=-2000; 

	if (POV != MyLinkedList.Last) do{
                    delta_d = MyLinkedList.LL[sweep].Value.d - d; 
                    delta_h = MyLinkedList.LL[sweep].Value.h - h;
                    sweeppt = MyLinkedList.LL[sweep].Value.idx;
                    kernelV(rsF, nrsF, sur_dataF, vol_dataF);
                    sweep = MyLinkedList.LL[sweep].next;
	}while(sweep!=-1);

	closeprof(visible,true,true,false);

	visible=true;
	nrsB=0;
	sur_dataB=0;
	vol_dataB=0;
	delta_d=0;
	delta_h=0;
	open_delta_d=0;
	open_delta_h=0;
	sweep = MyLinkedList.LL[POV].prev;
	sweeppt= MyLinkedList.LL[POV].Value.idx;
	rsB[0][0]=sweeppt;
	max_angle=-2000; 

	if (POV != MyLinkedList.First) do{
                    delta_d = d-MyLinkedList.LL[sweep].Value.d; 
                    delta_h = MyLinkedList.LL[sweep].Value.h - h;
                    sweeppt = MyLinkedList.LL[sweep].Value.idx;
                    kernelV(rsB, nrsB, sur_dataB, vol_dataB);
                    sweep = MyLinkedList.LL[sweep].prev;
	}while(sweep!=-2);

	closeprof(visible,false,true,false);
}


  void Sector::kernelV(int rs[][2], int &nrs, float &sur_data, float &vol_data)
  {
  float idd=1/delta_d;
  float angle = delta_h*idd;
  bool above = (angle > max_angle);
  bool opening = above && (!visible);
  bool closing = (!above) && (visible);
  visible = above;
  max_angle = max(angle, max_angle);
  if (opening)
  {
          open_delta_d = delta_d;
          open_delta_h = delta_h;
          nrs++;
          rs[nrs][0] = sweeppt;
  }
  if (closing)
  {
          sur_data += (delta_d * delta_d - open_delta_d * open_delta_d);
          float mean = (delta_d + open_delta_d);
          vol_data += mean * fabs(open_delta_d * delta_d - open_delta_d * delta_h);
          rs[nrs][1] = sweeppt;
  }
  }


	// ============================================================================================
	// ==========================  Kernel 4: Viewshed with "already covered areas" for coverage
	// ============================================================================================

void Sector::sweepC(int i)
{
	float d = MyLinkedList.LL[POV].Value.d;
	float h = MyLinkedList.LL[POV].Value.h + obs_h;
	delta_d=0;


	//printf("i:  %d  sweeppt:  %d\n",i,sweeppt);
	int sweep=presweepF();
	if (POV != MyLinkedList.Last) do{
                    sweeppt = MyLinkedList.LL[sweep].Value.idx;
		    bool alreadycovered=coverage_in[sweeppt];
		    if(alreadycovered&&visible)sur_dataF+=delta_d*delta_d;
                    delta_d = MyLinkedList.LL[sweep].Value.d - d; 
		    if(alreadycovered&&visible)sur_dataF-=delta_d*delta_d;
                    delta_h = MyLinkedList.LL[sweep].Value.h - h;
                    kernelS(rsF,nrsF,sur_dataF);
			//if(espt==0)printf("%f\t%f\t%f\t%d\n",delta_d,delta_h,h,visible);
                    sweep = MyLinkedList.LL[sweep].next;
	}while(sweep!=-1);

	closeprof(visible,true,false,false);

	if(sweeppt==-1){
	printf("i:  %d  sweeppt:  %d\n",i,sweeppt);
	printf("%f %f\n",d,h);
	printf("%d %f \n",nrsF ,sur_dataF);
	}

	delta_d=0;
	sweep=presweepB();
	if (POV != MyLinkedList.First) do{
                    sweeppt = MyLinkedList.LL[sweep].Value.idx;
		    bool alreadycovered=coverage_in[sweeppt];
		    if(alreadycovered&&visible)sur_dataB+=delta_d*delta_d;
                    delta_d = d-MyLinkedList.LL[sweep].Value.d; 
		    if(alreadycovered&&visible)sur_dataB-=delta_d*delta_d;
                    delta_h = MyLinkedList.LL[sweep].Value.h - h;
                    kernelS(rsB,nrsB, sur_dataB);
                    sweep = MyLinkedList.LL[sweep].prev;
	}while(sweep!=-2);
	//printf("i:  %d  sweeppt:  %d\n",i,sweeppt);

	closeprof(visible,false,false,false);
}


//Distance between two points
float Sector::distance(int a, int b)
{
            float ax = a % 2000;
            float ay = a / 2000;
            float bx = b % 2000;
            float by = b / 2000;
            return (float)((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
}

float Sector::distance(int x1, int x2, int y1, int y2)
{
return (float)((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

bool Sector::belongtoFsector(float angle)
{
if(sector!=179)return ((angle>=sect_angle_g-0.5)&&(angle<=sect_angle_g+0.5)) ;
else 
{
	if((angle>=sect_angle_g-0.5)&&(angle<=sect_angle_g+0.5))return true;
	return false;
}
}

bool Sector::belongtoBsector(float angle)
{
if(sector!=179)return ((angle>=sect_angle_g+180-0.5)&&(angle<=sect_angle_g+180+0.5));
else 
{
	if(angle <= origin)return true;
	if(angle >= 359+origin )return true; 
	return false;
}
}


//Determine if two points are visible
//
bool Sector::pointeval(int i, int cpx, int cpy)
{
//just for malaga test
//if(i%2000 < 900) return  true;
int x=i/2000;
int y=i%2000;
float angle =atan2((y - cpy), (x - cpx));
if(angle<0)angle+=2*PI_F;
angle/=torad;
if(angle<0)printf("%f ",angle);
if(angle>=360)printf("%f ",angle);

bool FS=belongtoFsector(angle);
bool BS=belongtoBsector(angle);
if(!FS&&!BS)return false;

float d = distance(cpy, y, cpx, x);
if(FS)
for (int k1 = 0; k1 < nrsF; k1++)
      if ((d >= lsF[k1*2]) && (d <= lsF[k1*2+1])) {cntcoverage++;return true; }

if(BS)
for (int k1 = 0; k1 < nrsB; k1++)
      if ((d >= lsB[k1*2]) && (d <= lsB[k1*2+1]))  {cntcoverage++;return true; }
return false;
}


void Sector::save_cover()
{
		storers();
		int cpx=towerloc/2000;
		int cpy=towerloc%2000;
		lsF=new float[2*nrsF];
		lsB=new float[2*nrsB];
		for(int j=0;j<nrsF;j++){lsF[2*j]=distance(towerloc,rsF[j][0]);lsF[2*j+1]=distance(towerloc,rsF[j][1]);}
		for(int j=0;j<nrsB;j++){lsB[2*j]=distance(towerloc,rsB[j][0]);lsB[2*j+1]=distance(towerloc,rsB[j][1]);}
		for(int j=0;j<dim;j++) if(pointeval(j,cpx,cpy))coverage_out[j]=true;
		delete lsF;
		delete lsB;
		printf("%d \n",cntcoverage);
}




	// ============================================================================================
	// ==========================  Kernels: Common functions:
	// ============================================================================================




int Sector::presweepF()
{
visible=true;
nrsF=0;
sur_dataF=0;
delta_d=0;
delta_h=0;
open_delta_d=0;
open_delta_h=0;
int sweep = MyLinkedList.LL[POV].next;
sweeppt= MyLinkedList.LL[POV].Value.idx;
rsF[0][0]=sweeppt;
max_angle=-2000; 
return sweep;
}

int Sector::presweepB()
{
visible=true;
nrsB=0;
sur_dataB=0;
delta_d=0;
delta_h=0;
open_delta_d=0;
open_delta_h=0;
int sweep = MyLinkedList.LL[POV].prev;
sweeppt= MyLinkedList.LL[POV].Value.idx;
rsB[0][0]=sweeppt;
max_angle=-2000; 
return sweep;
}



void Sector::closeprof(bool visible, bool fwd, bool vol, bool full)
{
if(fwd) {
	nrsF++;
	if(visible)
		{
		rsF[nrsF-1][1]=sweeppt;
		sur_dataF+=delta_d*delta_d-open_delta_d*open_delta_d;
		if(vol)vol_dataF+=(delta_d+open_delta_d)*fabs(delta_d*open_delta_h-delta_h*open_delta_d);
		}
	if((nrsF==1)&&(delta_d<1.5F))nrsF=0;
	}
else    {
	nrsB++;
	if(visible)
		{
		rsB[nrsB-1][1]=sweeppt;
		sur_dataB+=delta_d*delta_d-open_delta_d*open_delta_d;
		if(vol)vol_dataB+=(delta_d+open_delta_d)*fabs(delta_d*open_delta_h-delta_h*open_delta_d);
		}
	if((nrsB==1)&&(delta_d<1.5F))nrsB=0; // safe data with small vs
	}
}








