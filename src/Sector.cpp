#include <math.h>

#include <plog/Log.h>

#include "definitions.h"
#include "Sector.h"

namespace TVS {

Sector::Sector(bool is_precomputing) :
  is_precomputing(is_precomputing) {
}

void Sector::init_storage() {
	this->nodes = new LinkedList::node[DEM_SIZE];
	this->band_of_sight = LinkedList(BAND_SIZE);

	this->isin = new double[DEM_SIZE];
	this->icos = new double[DEM_SIZE];
	this->icot = new double[DEM_SIZE];
	this->itan = new double[DEM_SIZE];

	this->orderiAT = new int[DEM_SIZE];
	this->tmp1 = new int[DEM_SIZE];
	this->tmp2 = new int[DEM_SIZE];
	if(!this->is_precomputing) {
		surfaceF = new float[DEM_SIZE];
		surfaceB = new float[DEM_SIZE];
		if(IS_STORE_RING_SECTORS) {
			rsectorF = new int*[DEM_SIZE];
			rsectorB = new int*[DEM_SIZE];
			size_dsF = new unsigned short[DEM_SIZE];
			size_dsB = new unsigned short[DEM_SIZE];
		}
	}
}

void Sector::setHeights(double *heights) {
	for (int point = 0; point < DEM_SIZE; point++) {
		this->nodes[point].idx = point;
		this->nodes[point].h = (float)heights[point]; //decameters
	}
}

Sector::~Sector() {
	delete [] isin;
	delete [] icos;
	delete [] icot;
	delete [] itan;

	delete [] orderiAT;
	delete [] tmp1;
	delete [] tmp2;

	if(!this->is_precomputing) {
		delete [] surfaceF;
		delete [] surfaceB;
		if(IS_STORE_RING_SECTORS) {
			delete [] rsectorF;
			delete [] rsectorB;
			delete [] size_dsF;
			delete [] size_dsB;
		}
	}
}

void Sector::loopThroughBands(int sector_angle) {
	bool starting, ending;
  this->sector_angle = sector_angle;
  this->openPreComputedDataFile();
  this->changeAngle();
	for(int point = 0; point < DEM_SIZE - 1; point++) {
		PoV = point % BAND_SIZE;
		this->sweepS(point);
		if(!this->is_precomputing) this->storers(this->orderiAT[point]);
		post_loop(point);
	}
  fclose(this->precomputed_data_file);
}

void Sector::changeAngle() {
  LOGD << "Changing sector angle to: " << this->sector_angle;
	this->quad = (this->sector_angle >= 90) ? 1 : 0;
	if (this->quad == 1) this->sector_angle -= 90;
	this->pre_computed_sin();
	this->sort();
	if (this->quad == 1) this->sector_angle += 90;
	this->setDistances(this->sector_angle);
	this->band_of_sight.Clear();
	this->band_of_sight.FirstNode(this->nodes[this->orderiAT[0]]);
}

void Sector::openPreComputedDataFile() {
  const char *mode;
  char path[100];
  this->preComputedDataPath(path, this->sector_angle);
  if(this->is_precomputing) {
    mode = "wb";
  } else {
    mode = "rb";
  }
  this->precomputed_data_file = fopen(path, mode);
  if(this->precomputed_data_file == NULL) {
    LOG_ERROR
      << "Couldn't open " << path
      << " for sector_angle: " << this->sector_angle;
    throw "Couldn't open file.";
  }
}

//precomputed values for trigonometry
void Sector::pre_computed_sin() {
	double ac;
	double s, c, ct, tn;

	ac = (SECTOR_SHIFT + this->sector_angle + 0.5) * TO_RADIANS;
	this->sect_angle = ac;
	s = std::sin(this->sect_angle);
	c = std::cos(this->sect_angle);
	tn = std::tan(this->sect_angle);
	ct = 1 / tn;

	for (int i = 0; i < DEM_SIZE; i++)
	{
		this->isin[i] = i * s;
		this->icos[i] = i * c;
		this->icot[i] = i * ct;
		this->itan[i] = i * tn;
	}
}

//Distance to segment over point 0 (NW)
void Sector::setDistances(int sector)
{
	double val;
	for (int j = 0; j < DEM_WIDTH; j++)
		for (int i = 0; i < DEM_HEIGHT; i++)
		{
			val= icos[j] + isin[i];
			if (quad == 1) val = icos[i] - isin[j];
			nodes[j * DEM_HEIGHT + i].d = val;
		}
}

void Sector::presort() {
	double ct, tn;
	this->tmp1[0] = 0;
	this->tmp2[0] = 0;

  tn = std::tan(this->sect_angle);
	ct = 1 / tn;
	for (int j = 1; j < DEM_WIDTH; j++) {
		this->tmp1[j] = this->tmp1[j - 1] + (int)std::min(DEM_HEIGHT, (int)floor(ct * j));
  }
	for (int i = 1; i < DEM_HEIGHT; i++) {
		this->tmp2[i] = this->tmp2[i - 1] + (int)std::min(DEM_WIDTH, (int)floor(tn * i));
  }
}


void Sector::sort()
{
	this->presort();
	double lx = DEM_WIDTH - 1;
	double ly = DEM_HEIGHT - 1;
	double x,y;
	int ind, xx, yy, p, np;
	for (int j = 1; j <= DEM_WIDTH; j++)
	{
		x = (j - 1);
		for (int i = 1; i <= DEM_HEIGHT; i++)
		{
			y = (i - 1);
			ind = i * j;
			ind += ((ly - y) < (icot[j - 1])) ? ((DEM_HEIGHT - i) * j - tmp2[DEM_HEIGHT - i] - (DEM_HEIGHT - i)) : tmp1[j - 1];
			ind += ((lx - x) < (itan[i - 1])) ? ((DEM_WIDTH - j) * i - tmp1[DEM_WIDTH - j] - (DEM_WIDTH - j)) : tmp2[i - 1];
			xx = j - 1;
			yy = i - 1;
			p = xx * DEM_HEIGHT + yy;
			np = (DEM_WIDTH - 1 - yy) * DEM_HEIGHT + xx;
			if (quad == 0)
			{
				nodes[p].oa = ind - 1;
				orderiAT[ind - 1] = np;
			} else
      {
				nodes[np].oa = ind - 1;
				orderiAT[DEM_SIZE - ind] = p;
			}
		}
	}
}

void Sector::post_loop(int point) {
  // Is the band of sight is starting to be populated?
	bool starting = (point < HALF_BAND_SIZE);
  // Is the band of sight coming to ending, therefore emptying?
	bool ending = (point >= (DEM_SIZE - HALF_BAND_SIZE - 1));

  this->atright = false;
	this->atright_trans = false;
	this->NEW_position = -1;
	this->NEW_position_trans = -1;
  LinkedList::node tmp = this->newnode_trans;

	if(!ending)
	{
		this->newnode = this->nodes[this->orderiAT[starting ? 2 * point + 1 : HALF_BAND_SIZE + point + 1]];
		this->atright = this->newnode.oa > this->band_of_sight.LL[this->PoV].Value.oa;
	}
	if (starting)
	{
		this->newnode_trans = this->nodes[this->orderiAT[2 * point + 2]];
		this->atright_trans = this->newnode_trans.oa > this->band_of_sight.LL[this->PoV].Value.oa;
	}

	if(!this->is_precomputing) {
		if (!ending) {
			fread(&this->NEW_position, 4, 1, this->precomputed_data_file);
			this->insert_node(this->newnode, this->NEW_position, !starting);
		}
		if(starting)
		{
			fread(&this->NEW_position_trans, 4, 1, this->precomputed_data_file);
			this->insert_node(this->newnode_trans, this->NEW_position_trans, false);
		}
	} else {
		if (!starting && !ending)
		{
			this->newnode = this->nodes[this->orderiAT[starting ? 2 * point + 1 : HALF_BAND_SIZE + point + 1]];
			this->NEW_position = -1;
			this->atright = this->newnode.oa > this->band_of_sight.LL[this->PoV].Value.oa;
			this->calcule_pos_newnode(true);
		}
		if (starting) {
			this->newnode = this->nodes[this->orderiAT[2 * point + 1]];
			this->NEW_position = -1;
			this->atright = this->newnode.oa > this->band_of_sight.LL[PoV].Value.oa;
			this->calcule_pos_newnode(false);
			this->newnode = this->nodes[this->orderiAT[2 * point + 2]];
			this->atright = this->newnode.oa > this->band_of_sight.LL[this->PoV].Value.oa;
			this->calcule_pos_newnode(false);
		}
	}
	if (ending) this->band_of_sight.Remove_two();
}

void Sector::calcule_pos_newnode(bool remove)
{
	int sweep = 0;
	if (newnode.oa < this->band_of_sight.LL[this->band_of_sight.First].Value.oa) {
		NEW_position = -2; // Before first
		this->band_of_sight.AddFirst(newnode, remove);
		fwrite(&NEW_position, 4, 1, this->precomputed_data_file);
		sweep = this->band_of_sight.First;
	}
	else if (newnode.oa > this->band_of_sight.LL[this->band_of_sight.Last].Value.oa) {
		NEW_position = -1;
		this->band_of_sight.AddLast(newnode, remove);
		fwrite(&NEW_position,4,1,this->precomputed_data_file);
	} else {
		sweep = this->band_of_sight.LL[this->band_of_sight.First].next;
		bool go_on = true;
		do
		{
			if (newnode.oa < this->band_of_sight.LL[sweep].Value.oa)
			{
				NEW_position = this->band_of_sight.LL[sweep].prev;
				this->band_of_sight.Add(newnode, NEW_position, remove);
				fwrite(&NEW_position, 4, 1, this->precomputed_data_file);
				sweep = this->band_of_sight.LL[NEW_position].next;
				go_on = false;
			}
			else
				sweep = this->band_of_sight.LL[sweep].next;
		} while (go_on);
	}
}


void Sector::insert_node(LinkedList::node newnode, int position, bool remove)
{
	if(position > -1)
	{
		this->band_of_sight.Add(newnode,position,remove);
	}else
	{
		if(position==-1)
		{
			this->band_of_sight.AddLast(newnode,remove);
		}else
		{
			this->band_of_sight.AddFirst(newnode,remove);
		}
	}

}

// Fulldata to store ring sector topology
void Sector::storers(int i)
{
	float surscale = PI_F / (360 * DEM_SCALE * DEM_SCALE); //hectometers^2
	this->surfaceF[i] = this->sur_dataF * surscale;
	this->surfaceB[i] = this->sur_dataB * surscale;

  //if(surfaceB[i] > 100) LOGI << i << ":" << sector_angle << ":" << surfaceB[i];

	if(IS_STORE_RING_SECTORS) {
		rsectorF[i] = new int[nrsF * 2];
		rsectorB[i] = new int[nrsB * 2];
		size_dsF[i] = 2*nrsF;
		size_dsB[i] = 2*nrsB;
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

// Ring sectors (along with sector angle) contain all the information
// to recreate full viewshed data.
void Sector::recordsectorRS() {
  FILE *fs;
  char path_ptr[100];
  int no_of_ring_sectors;
	this->ringSectorDataPath(path_ptr, this->sector_angle);
	fs = fopen(path_ptr, "wb");

  this->rsectorF[orderiAT[DEM_SIZE - 1]] = new int[0];
	this->rsectorB[orderiAT[DEM_SIZE - 1]] = new int[0];
	this->size_dsB[orderiAT[DEM_SIZE - 1]] = 0;
	this->size_dsF[orderiAT[DEM_SIZE - 1]] = 0;

  for (int point = 0; point < DEM_SIZE; point++) {
		no_of_ring_sectors = size_dsF[point];
    // Every point in the DEM gets a marker saying how much of the proceeding bytes
    // will have ring sector data in them.
    fwrite(&no_of_ring_sectors, 4, 1, fs);
		for (int iRS = 0; iRS < no_of_ring_sectors; iRS++) {
      fwrite(&this->rsectorF[point][iRS], 4, 1, fs);
		}

		no_of_ring_sectors = size_dsB[point];
    fwrite(&no_of_ring_sectors, 4, 1, fs);
		for (int iRS = 0; iRS < no_of_ring_sectors; iRS++) {
      fwrite(&this->rsectorB[point][iRS], 4, 1, fs);
		}
	}

	fclose(fs);

	for(int point = 0; point < DEM_SIZE; point++) {
		delete this->rsectorF[point];
		delete this->rsectorB[point];
	}
}

void Sector::sweepS(int _not_used)
{
	int sweep;

	float d = this->band_of_sight.LL[PoV].Value.d;
	float h = this->band_of_sight.LL[PoV].Value.h + SCALED_OBSERVER_HEIGHT;

  sweep = presweepF();
	if (PoV != this->band_of_sight.Last) do {
			delta_d = this->band_of_sight.LL[sweep].Value.d - d;
			delta_h = this->band_of_sight.LL[sweep].Value.h - h;
			sweeppt = this->band_of_sight.LL[sweep].Value.idx;
			kernelS(rsF, nrsF, sur_dataF);
			sweep = this->band_of_sight.LL[sweep].next;
		} while(sweep!=-1);

	closeprof(visible,true,false,false);

	sweep = presweepB();
	if (PoV != this->band_of_sight.First) do {
			delta_d = d-this->band_of_sight.LL[sweep].Value.d;
			delta_h = this->band_of_sight.LL[sweep].Value.h - h;
			sweeppt = this->band_of_sight.LL[sweep].Value.idx;
			kernelS(rsB, nrsB, sur_dataB);
			sweep = this->band_of_sight.LL[sweep].prev;
		} while(sweep!=-2);

	closeprof(visible,false,false,false);
}

// remember that rs could be &rsF or &rsB
void Sector::kernelS(int rs[][2], int &nrs, float &sur_data)
{
	float angle = delta_h/delta_d;
	bool above = (angle > max_angle);
	bool opening = above && (!visible);
	bool closing = (!above) && (visible);
	visible = above;
	max_angle = std::max(angle, max_angle);
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

int Sector::presweepF()
{
	visible=true;
	nrsF=0;
	sur_dataF=0;
	delta_d=0;
	delta_h=0;
	open_delta_d=0;
	open_delta_h=0;
	int sweep = this->band_of_sight.LL[PoV].next;
	sweeppt= this->band_of_sight.LL[PoV].Value.idx;
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
	int sweep = this->band_of_sight.LL[PoV].prev;
	sweeppt = this->band_of_sight.LL[PoV].Value.idx;
	rsB[0][0]=sweeppt;
	max_angle=-2000;
	return sweep;
}

// Close any open ring sectors.
//
// I think it's fair to assume that any open ring sectors on the edge of a DEM will stretch beyond
// therefore making the TVS calculations for points near the DEM boundaries incomplete. You would
// need the DEM stretching as far as any point can possibly see.
//
// NB. The closing of an open profile assumes that boundery DEM points can't be seen, which causes
// that point's surface to not be added to the total viewshed surface.
void Sector::closeprof(bool visible, bool fwd, bool vol, bool full)
{
	if(fwd) {
		nrsF++;
		if(visible)
		{
			rsF[nrsF-1][1]=sweeppt;
			sur_dataF += delta_d*delta_d - open_delta_d*open_delta_d;
		}

    // I think this means, "discount bands of sight that contain only one ring sector
    // and where the PoV is close to the edge". But I don't know why and also I don't
    // know why the ring sector is removed but the surface area is still counted.
		if((nrsF == 1) && (delta_d < 1.5F)) nrsF = 0;
	}
	else {
		nrsB++;
		if(visible)
		{
			rsB[nrsB-1][1]=sweeppt;
			sur_dataB+=delta_d*delta_d-open_delta_d*open_delta_d;
		}
		if((nrsB==1)&&(delta_d<1.5F)) nrsB=0; // safe data with small vs
	}
}

void Sector::ringSectorDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s%d.bin", RING_SECTOR_DIR, sector_angle);
}

void Sector::preComputedDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s%d.bin", SECTOR_DIR, sector_angle);
}

} // namespace TVS

