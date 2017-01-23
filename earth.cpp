#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "LatLong-UTMconversion.h"
#include "constants.h"

#define degrees (180./PI)
#define rads (PI/180.)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

void LLtoUTM(int ReferenceEllipsoid, const double Lat, const double Long, 
			 double &UTMNorthing, double &UTMEasting, char* UTMZone)
{
//converts lat/long to UTM coords.  Equations from USGS Bulletin 1532
//East Longitudes are positive, West longitudes are negative. 
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees
	//Written by Chuck Gantz- chuck.gantz@globalstar.com



	double a = ellipsoid[ReferenceEllipsoid].EquatorialRadius;
	double eccSquared = ellipsoid[ReferenceEllipsoid].eccentricitySquared;
	double k0 = 0.9996;

	double LongOrigin;
	double eccPrimeSquared;
	double N, T, C, A, M;
	
//Make sure the longitude is between -180.00 .. 179.9
	double LongTemp = (Long+180)-int((Long+180)/360)*360-180; // -180.00 .. 179.9;

	double LatRad = Lat*deg2rad;
	double LongRad = LongTemp*deg2rad;
	double LongOriginRad;
	int    ZoneNumber;

//Esto lo necesito para extrapolar
	if(!strlen(UTMZone)){


	ZoneNumber = int((LongTemp + 180)/6) + 1;
  
	if( Lat >= 56.0 && Lat < 64.0 && LongTemp >= 3.0 && LongTemp < 12.0 )
		ZoneNumber = 32;

  // Special zones for Svalbard
	if( Lat >= 72.0 && Lat < 84.0 ) 
	{
	  if(      LongTemp >= 0.0  && LongTemp <  9.0 ) ZoneNumber = 31;
	  else if( LongTemp >= 9.0  && LongTemp < 21.0 ) ZoneNumber = 33;
	  else if( LongTemp >= 21.0 && LongTemp < 33.0 ) ZoneNumber = 35;
	  else if( LongTemp >= 33.0 && LongTemp < 42.0 ) ZoneNumber = 37;
	 }

//Esto lo necesito para extrapolar
	}else{
		char *kk;
		ZoneNumber = strtoul(UTMZone, &kk, 10);
	}


	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone
	LongOriginRad = LongOrigin * deg2rad;

	//compute the UTM Zone from the latitude and longitude
	//sprintf(UTMZone,"%d%c", ZoneNumber, UTMLetterDesignator(Lat));

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	N = a/sqrt(1-eccSquared*sin(LatRad)*sin(LatRad));
	T = tan(LatRad)*tan(LatRad);
	C = eccPrimeSquared*cos(LatRad)*cos(LatRad);
	A = cos(LatRad)*(LongRad-LongOriginRad);

	M = a*((1	- eccSquared/4		- 3*eccSquared*eccSquared/64	- 5*eccSquared*eccSquared*eccSquared/256)*LatRad 
				- (3*eccSquared/8	+ 3*eccSquared*eccSquared/32	+ 45*eccSquared*eccSquared*eccSquared/1024)*sin(2*LatRad)
									+ (15*eccSquared*eccSquared/256 + 45*eccSquared*eccSquared*eccSquared/1024)*sin(4*LatRad)
									- (35*eccSquared*eccSquared*eccSquared/3072)*sin(6*LatRad));
	
	UTMEasting = (double)(k0*N*(A+(1.-T+C)*A*A*A/6.
					+ (5.-18.*T+T*T+72.*C-58.*eccPrimeSquared)*A*A*A*A*A/120.)
					+ 500000.0);

	UTMNorthing = (double)(k0*(M+N*tan(LatRad)*(A*A/2.+(5.-T+9.*C+4.*C*C)*A*A*A*A/24.
				 + (61.-58.*T+T*T+600.*C-330.*eccPrimeSquared)*A*A*A*A*A*A/720.)));
	if(Lat < 0)
		UTMNorthing += 10000000.0; //10000000 meter offset for southern hemisphere
}

char UTMLetterDesignator(double Lat)
{
//This routine determines the correct UTM letter designator for the given latitude
//returns 'Z' if latitude is outside the UTM limits of 84N to 80S
	//Written by Chuck Gantz- chuck.gantz@globalstar.com
	char LetterDesignator;

	if((84 >= Lat) && (Lat >= 72)) LetterDesignator = 'X';
	else if((72 > Lat) && (Lat >= 64)) LetterDesignator = 'W';
	else if((64 > Lat) && (Lat >= 56)) LetterDesignator = 'V';
	else if((56 > Lat) && (Lat >= 48)) LetterDesignator = 'U';
	else if((48 > Lat) && (Lat >= 40)) LetterDesignator = 'T';
	else if((40 > Lat) && (Lat >= 32)) LetterDesignator = 'S';
	else if((32 > Lat) && (Lat >= 24)) LetterDesignator = 'R';
	else if((24 > Lat) && (Lat >= 16)) LetterDesignator = 'Q';
	else if((16 > Lat) && (Lat >= 8)) LetterDesignator = 'P';
	else if(( 8 > Lat) && (Lat >= 0)) LetterDesignator = 'N';
	else if(( 0 > Lat) && (Lat >= -8)) LetterDesignator = 'M';
	else if((-8> Lat) && (Lat >= -16)) LetterDesignator = 'L';
	else if((-16 > Lat) && (Lat >= -24)) LetterDesignator = 'K';
	else if((-24 > Lat) && (Lat >= -32)) LetterDesignator = 'J';
	else if((-32 > Lat) && (Lat >= -40)) LetterDesignator = 'H';
	else if((-40 > Lat) && (Lat >= -48)) LetterDesignator = 'G';
	else if((-48 > Lat) && (Lat >= -56)) LetterDesignator = 'F';
	else if((-56 > Lat) && (Lat >= -64)) LetterDesignator = 'E';
	else if((-64 > Lat) && (Lat >= -72)) LetterDesignator = 'D';
	else if((-72 > Lat) && (Lat >= -80)) LetterDesignator = 'C';
	else LetterDesignator = 'Z'; //This is here as an error flag to show that the Latitude is outside the UTM limits

	return LetterDesignator;
}

//La letra de la zona UTM indica el hemisferio
//
//
void UTMtoLL(int ReferenceEllipsoid, const double UTMNorthing, const double UTMEasting, const char* UTMZone,
			  double& Lat,  double& Long )
{
//converts UTM coords to lat/long.  Equations from USGS Bulletin 1532
//East Longitudes are positive, West longitudes are negative.
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees.
	//Written by Chuck Gantz- chuck.gantz@globalstar.com

	double k0 = 0.9996;
	double a = ellipsoid[ReferenceEllipsoid].EquatorialRadius;
	double eccSquared = ellipsoid[ReferenceEllipsoid].eccentricitySquared;
	double eccPrimeSquared;
	double e1 = (1-sqrt(1-eccSquared))/(1+sqrt(1-eccSquared));
	double N1, T1, C1, R1, D, M;
	double LongOrigin;
	double mu, phi1, phi1Rad;
	double x, y;
	int ZoneNumber;
	char* ZoneLetter;
	int NorthernHemisphere; //1 for northern hemispher, 0 for southern

	x = UTMEasting - 500000.0; //remove 500,000 meter offset for longitude
	y = UTMNorthing;

	ZoneNumber = strtoul(UTMZone, &ZoneLetter, 10);
	if((*ZoneLetter - 'N') >= 0)
		NorthernHemisphere = 1;//point is in northern hemisphere
	else
	{
		NorthernHemisphere = 0;//point is in southern hemisphere
		y -= 10000000.0;//remove 10,000,000 meter offset used for southern hemisphere
	}

	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	M = y / k0;
	mu = M/(a*(1-eccSquared/4-3*eccSquared*eccSquared/64-5*eccSquared*eccSquared*eccSquared/256));

	phi1Rad = mu	+ (3*e1/2-27*e1*e1*e1/32)*sin(2*mu) 
				+ (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
				+(151*e1*e1*e1/96)*sin(6*mu);
	phi1 = phi1Rad*rad2deg;

	N1 = a/sqrt(1-eccSquared*sin(phi1Rad)*sin(phi1Rad));
	T1 = tan(phi1Rad)*tan(phi1Rad);
	C1 = eccPrimeSquared*cos(phi1Rad)*cos(phi1Rad);
	R1 = a*(1-eccSquared)/pow(1-eccSquared*sin(phi1Rad)*sin(phi1Rad), 1.5);
	D = x/(N1*k0);

	Lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrimeSquared)*D*D*D*D/24
					+(61+90*T1+298*C1+45*T1*T1-252*eccPrimeSquared-3*C1*C1)*D*D*D*D*D*D/720);
	Lat = Lat * rad2deg;

	Long = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrimeSquared+24*T1*T1)
					*D*D*D*D*D/120)/cos(phi1Rad);
	Long = LongOrigin + Long * rad2deg;


}


void ed50twgs84(double &n, double &e,int dir,char *zona)
{
double rX,X=e;
double rY,Y=n;
double rZ=0,Z=0.;
double dX=-84.;
double dY=-107.;
double dZ=-120.;
UTMtoLL(14,Y,X,zona,rX,rY);
rX=rX*rads;
rY=rY*rads;
double cosrX=cos(rX);
double cosrY=cos(rY);
double cosrZ=cos(rZ);
double sinrX=sin(rX);
double sinrY=sin(rY);
double sinrZ=sin(rZ);
double term1 = X * cosrY * cosrZ + Y * cosrY * sinrZ - Z * sinrY;
double term2 = X * (sinrX * sinrY * cosrZ - cosrX * sinrZ) + Y * (sinrX * sinrY * sinrZ + cosrX * cosrZ) + Z * sinrX * cosrY;
double term3 = X * (cosrX * sinrY * cosrZ + sinrX * sinrZ) + Y * (cosrX * sinrY * sinrZ - sinrX * cosrZ) + Z * cosrX * cosrY;
term1 = X * cosrY * cosrZ + Y * cosrY * sinrZ - Z * sinrY;
term2 = X * (sinrX * sinrY * cosrZ - cosrX * sinrZ) + Y * (sinrX * sinrY * sinrZ + cosrX * cosrZ) + Z * sinrX * cosrY;
term3 = X * (cosrX * sinrY * cosrZ + sinrX * sinrZ) + Y * (cosrX * sinrY * sinrZ - sinrX * cosrZ) + Z * cosrX * cosrY;
 
  X = dX + term1;
  Y = dY + term2;
  Z = dZ + term3;
}

void ed50towgs84(double &n, double &e,int dir)
{


char zona[3];
strcpy(zona,"30N");
double fi,la;
int sou,des;
long double da=251;
long double df=0.000014192702259;
long double dX=-84;//84.87;
long double dY=-107;//96.49;
long double dZ=-120;//116.95;
long double a;
long double e2;
long double f;
if(!dir){
	sou=14;
	des=23;
	}
else {
	sou=23;
	des=14;}




if(dir){
	a=6378137;
	e2=0.0066943799913;
	f=298.257223563;
	}
else {
	a=6378388;
	e2=0.006722670022;
	f=297;
	dX=-dX;
	dY=-dY;
	dZ=-dZ;
	}
UTMtoLL(sou,n,e,zona,fi,la);
long double ofi=fi;
long double ola=la;
fi=fi*rads;
la=la*rads;
long double sf =sin(fi);
long double sf2=sin(2*fi);
long double sl =sin(la);
long double cf =cos(fi);
long double cl =cos(la);
long double s1 =sin(rads/3600.);


long double root=1-e2*sf*sf;
long double N=a/sqrt(root);
long double ro=a*(1-e2)/pow(root,(long double)0.333333333333333);
long double term=a*df+da/f;
long double dla=(-dX*sl+dY*cl)/(N*cf*s1);
long double dfi=(-dX*sf*cl-dY*sf*sl+dZ*cf+term*sf2)/(ro*s1);

fi=ofi-dfi/3600.;
la=ola-dla/3600.;
LLtoUTM(des,fi,la,n,e,zona);
}




void earth(char *KFN, char *JFN, int utm_n, int utm_e, int dim, int step)
		 {
char zona[3];
strcpy(zona,"30N");
//printf("%d %d %d %d\n",utm_n,utm_e,dim,step);
		 
        int xdim=dim;
        int xstep=step;
        int ydim=dim;
        int ystep=step;
				FILE *kml=fopen(KFN,"wt");
				/*
				AnsiString nombre=ChangeFileExt(google->FileName,"");
				*/
				
				fprintf(kml,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://earth.google.com/kml/2.0\">\n");
				fprintf(kml,"<GroundOverlay>\n\t<name>");
				fprintf(kml,JFN);
				fprintf(kml,"</name>\n\t<color>ffffffff</color>\n\t<Icon>\n\t\t<href>");//poner 80ffffff para semitransparente
				fprintf(kml,JFN);
				fprintf(kml,"</href>\n\t\t<viewBoundScale>0.75</viewBoundScale>\n\t</Icon>\n\t<LatLonBox id=\"khLatLonBox");
				fprintf(kml,JFN);
				fprintf(kml,"\">\n");
				double X1,Y1,X2,Y2,X3,Y3,X4,Y4,XC,YC,A1,A2,AM,LAT1,LAT2,LAT3,LAT4,LNG1,LNG2,LNG3,LNG4,LATC,LNGC;
				double XP,YP;

				//  1
				//4 C 2
				//  3

				X1=X3=utm_e+0.5*xstep*(1+xdim)-0.5*xstep;
				X2=   utm_e+1.0*xstep*(1+xdim)-0.5*xstep;
				X4=   utm_e+0.0*xstep*(1+xdim)-0.5*xstep;
				Y4=Y2=utm_n-0.5*ystep*(1+ydim)+0.5*ystep;	  //Revisar
				Y1=   utm_n-0.0*ystep*(1+ydim)-0.5*xstep;
				Y3=   utm_n-1.0*ystep*(1+ydim)-0.5*xstep;
				XC=utm_e+0.5*xstep*(1+xdim)-0.5*xstep;
				YC=utm_n-0.5*ystep*(1+ydim)+0.5*ystep;	  //Revisar
				ed50towgs84(Y1,X1,0);
				ed50towgs84(Y2,X2,0);
				ed50towgs84(Y3,X3,0);
				ed50towgs84(Y4,X4,0);
				ed50towgs84(YC,XC,0);
				UTMtoLL(23, Y1,X1,zona, LAT1,LNG1);
				UTMtoLL(23, Y2,X2,zona, LAT2,LNG2);
				UTMtoLL(23, Y3,X3,zona, LAT3,LNG3);
				UTMtoLL(23, Y4,X4,zona, LAT4,LNG4);
				UTMtoLL(23, YC,XC,zona, LATC,LNGC);
//cálculo de la cruz central no deformada pero girada
				YP=(LAT1-LAT3)/2;LAT1=LATC+YP;LAT3=LATC-YP;
				XP=(LNG2-LNG4)/2;LNG2=LNGC+XP;LNG4=LNGC-XP;
//Estimación del giro en horizontal: como quedaría punto dcho en latitud izqda
				LLtoUTM(23,LAT4,LNG2,YP,XP,zona);
				A1=atan2(Y2-YP,XP-X4);
//Estimación del giro en vertical: como quedaria punto bjo en longitud alto
				LLtoUTM(23,LAT3,LNG1,YP,XP,zona);
				A2=atan2(X3-XP,Y1-YP);
				AM=(A1*xdim+A2*ydim)/(ydim+xdim);
//Giramos en sentido contrario, ya que earth necesita colocar la foto antes de girar.
				double senAM=sin(AM);
				double cosAM=cos(AM);
				X1=XC;X3=XC;Y2=YC;Y4=YC;
				Y1=YC+(Y1-YC)/cosAM;
				Y3=YC-(YC-Y3)/cosAM;
				X2=XC+(X2-XC)/cosAM;
				X4=XC-(XC-X4)/cosAM;
				UTMtoLL(23, Y1,X1,zona, LAT1,LNG1);
				UTMtoLL(23, Y2,X2,zona, LAT2,LNG2);
				UTMtoLL(23, Y3,X3,zona, LAT3,LNG3);
				UTMtoLL(23, Y4,X4,zona, LAT4,LNG4);


				fprintf(kml,"\t\t<north>");
				fprintf(kml,"%15.10f",LAT1);
				fprintf(kml,"</north>\n");
				fprintf(kml,"\t\t<south>");
				fprintf(kml,"%15.10f",LAT3);
				fprintf(kml,"</south>\n");
				fprintf(kml,"\t\t<east>");
				fprintf(kml,"%15.10f",LNG2);
				fprintf(kml,"</east>\n");
				fprintf(kml,"\t\t<west>");
				fprintf(kml,"%15.10f",LNG4);
				fprintf(kml,"</west>\n");
				fprintf(kml,"\t\t<rotation>");
				fprintf(kml,"%15.10f",AM*rad2deg);
				fprintf(kml,"</rotation>\n");
				fprintf(kml,"\t</LatLonBox>\n</GroundOverlay>\n</kml>\n");
				fclose(kml);
		  }
