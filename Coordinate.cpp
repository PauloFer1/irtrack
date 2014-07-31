#include "stdafx.h"
#include "Coordinate.h"
#include <iostream>


using namespace std;


Coordinate::Coordinate(int inc, int x, int y, int z, int time, int isNew)
{
	setCoordinate(inc, x, y, z, time, isNew);
}
void Coordinate::setCoordinate(int incp, int xp, int yp, int zp, int timep, int isNewp)
{
	inc = incp;
	x = xp;
	y = yp;
	z = zp;
	time = timep;
	isNew = isNewp;
}
string Coordinate::getString()
{
	string suffix = "CP_P,";
	if (isNew == 1)
		suffix = "CP_S,";
	if (isNew == 2)
		suffix = "CP_E,";

	stringstream builder;
	builder << suffix << x << "," << y << "," << z << "," << "0.000" << endl;


	return(builder.str());
}
int Coordinate::isEqual(int xe, int ye, int ze)
{
	if (xe != x)
		return 0;
	if (ye != y)
		return 0;
	if (ze != z)
		return 0;
	return(1);
}
void Coordinate::mirror(int width)
{
	x = width - x;
}
void Coordinate::setZ(int index)
{
	z = index;
}
void Coordinate::setEndCP()
{
	isNew = 2;
}
CvPoint Coordinate::getPoint()
{
	return(cvPoint(x,y));
}
int Coordinate::getNew()
{
	return(isNew);
}

