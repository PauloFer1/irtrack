#ifndef COORDINATE_H
#define COORDINATE_H

#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include "opencv2\core\core.hpp"


using namespace std;

class Coordinate{
private:
	int x;
	int y;
	int z;
	int time;
	int inc;
	int isNew;	

public:
	Coordinate(){}
	Coordinate(int inc, int x, int y, int z, int time, int isNew);
	void setCoordinate(int inc, int x, int y, int z, int time, int isNew);
	string getString();
	void mirror(int width);
	void setZ(int index);
	void setEndCP();
	CvPoint getPoint();
	int getNew();
	int isEqual(int x, int y, int z);
	
};
#endif