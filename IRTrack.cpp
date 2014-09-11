// IRTrack.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <gl\gl.h>		
#include <gl\glu.h>	
#include <iostream>
#include <fstream>
#include <phidget21.h>

#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"

#include "cameramanager.h"
#include "cameralibrary.h"
#include "Coordinate.h"


using namespace cv;
using namespace CameraLibrary;

//GLOBALS
int z_indez = 50;
int slider = 50;
int isTrack = 0;
int cameraCount = 0;
Camera *camera[kMaxCameras];
cModuleSync *sync = new cModuleSync();
float focalLenght;
//»»»»»»»»»»»»»»»»»»»»»»»»»»»»» COMMON FUNCTIONS
void changeZ(int, void*)
{
	z_indez = (int)slider;
}
//»»»»»»»»»»»»»»»»»»»»»»»»»»»»» FILE FUNCTIONS
void saveFile(vector<Coordinate> coords, int cameraWidth)
{
	ofstream myfile("example.jcs");
	if (myfile.is_open())
	{
		myfile << "JR Points_CSV\n";
		for (int i = 0; i < coords.size(); i++)
		{
			coords[i].mirror(cameraWidth);
			if (i == coords.size() - 1)
				coords[i].setEndCP();
			myfile << coords[i].getString();
		}
		myfile.close();
	}
	else cout << "Unable to open file";

}
//»»»»»»»»»»»»»»»»»»»»»»»»»»»»» PHIDGETS FUNCTIONS
int CCONV AttachHandler(CPhidgetHandle IFK, void *userptr)
{
	int serialNo;
	const char *name;

	CPhidget_getDeviceName(IFK, &name);
	CPhidget_getSerialNumber(IFK, &serialNo);

	printf("%s %10d attached!\n", name, serialNo);

	return 0;
}

int CCONV DetachHandler(CPhidgetHandle IFK, void *userptr)
{
	int serialNo;
	const char *name;

	CPhidget_getDeviceName(IFK, &name);
	CPhidget_getSerialNumber(IFK, &serialNo);

	printf("%s %10d detached!\n", name, serialNo);

	return 0;
}

int CCONV ErrorHandler(CPhidgetHandle IFK, void *userptr, int ErrorCode, const char *unknown)
{
	printf("Error handled. %d - %s", ErrorCode, unknown);
	return 0;
}
int __stdcall SensorChangeHandler(CPhidgetInterfaceKitHandle IFK, void *usrptr, int Index, int Value)
{
	printf("Sensor: %d > Value: %d\n", Index, Value);
	//Insert your code here
	return 0;
}
int __stdcall InputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *usrptr, int Index, int Value)
{
	if (Index == 0)
		isTrack = Value;
	//Insert your code here
	return 0;
}
int getCenterPoint(Point dist)
{
	int x[3] = { 0, 1, 2 };
	for (int i = 2; i >= 0; i--)
	{
		if (dist.x != x[i] && dist.y != x[i])
		{
			return(x[i]);
		}
	}
}
Point getLongDistance(Point p1, Point p2, Point p3)
{
	Point points = Point(0,1);

	float d=0.0;
	float d12 = sqrt( pow((p2.x - p1.x), 2) + pow((p2.y - p1.y), 2) );
	if (d12 > d)
		d = d12;
	float d23 = sqrt(pow((p3.x - p2.x), 2) + pow((p3.y - p2.y), 2));
	if (d23 > d)
	{
		d = d23;
		points = Point(1,2);
	}
	float d31 = sqrt(pow((p1.x - p3.x), 2) + pow((p1.y - p3.y), 2));
	if (d31 > d)
	{
		d = 31;
		points = Point(2,0);
	}
	return(points);
}
Point getPencil(Point p1, Point p2, Point p3)
{
	Point p[3] = { p1, p2, p3 };

	Point dist = getLongDistance(p1, p2, p3);

	//» p3 = center*3 - p2 - p1
	float penX = p[getCenterPoint(dist)].x * 3 - p[dist.x].x - p[dist.y].x;
	float penY = p[getCenterPoint(dist)].y * 3 - p[dist.x].y - p[dist.y].y;
	Point pen = Point(penX, penY);

	return(pen);
}
int _tmain(int argc, _TCHAR* argv[])
{
	printf("==============================================================================\n");
	printf("== IR Camera Tracking                                 TARAMBOLA DEVELOPMENT ==\n");
	printf("==============================================================================\n\n");


	printf("Waiting for cameras to spin up...");

	//== Initialize Camera Library ==----
	CameraLibrary_EnableDevelopment();

	CameraManager::X().WaitForInitialization();

	//== Verify all cameras are initialized ==--

	if (CameraManager::X().AreCamerasInitialized())
		printf("complete\n\n");
	else
		printf("failed\n\n");

	printf("\n\n\nI << Iniciar reconhecimento\n");
	printf("S << salvar ficheiro\n");
	printf("R << recomeçar\n");
	printf("\n\n\nESC << Sair\n");
		
	CameraList list;
	CameraManager::X().GetCameraList(list);

	for (int i = 0; i < list.Count(); i++)
	{
		printf("Device %d: %s", i, list[i].Name());
		camera[i] = CameraManager::X().GetCamera(list[i].UID());
		if (camera[i] == 0)
			printf("Unable to connect camera...");
		else
		{

			cameraCount++;
		}

	}
	if (cameraCount == 0)
	{
		MessageBox(0, L"Please connect a camera", L"No Device Connected", MB_OK);
		CameraManager::X().Shutdown();
		return 1;
	}

	int cameraWidth = camera[0]->Width();
	int cameraHeight = camera[0]->Height();

	uchar imageBuffer[800 * 600];
	uchar imageBuffer2[800 * 600];
	

	for (int i = 0; i < cameraCount; i++)
	{
		sync->AddCamera(camera[i]);
	}

	for (int i = 0; i < cameraCount; i++)
	{
		camera[i]->SetVideoType(SegmentMode);
		//camera->SetVideoType(MJPEGMode);
		camera[i]->Start();
		camera[i]->SetTextOverlay(false);
	}
	
	Core::DistortionModel lensDistortion;
	camera[0]->GetDistortionModel(lensDistortion);

	focalLenght = camera[0]->FocalLength();

	//*********** PHIDGETS *************
	CPhidgetInterfaceKitHandle ifKit = 0;
	CPhidgetInterfaceKit_create(&ifKit);

	CPhidget_set_OnAttach_Handler((CPhidgetHandle)ifKit, AttachHandler, NULL);
	CPhidget_set_OnDetach_Handler((CPhidgetHandle)ifKit, DetachHandler, NULL);
	CPhidget_set_OnError_Handler((CPhidgetHandle)ifKit, ErrorHandler, NULL);

	CPhidget_open((CPhidgetHandle)ifKit, -1);
	CPhidget_waitForAttachment((CPhidgetHandle)ifKit, 2500);

	CPhidgetInterfaceKit_set_OnSensorChange_Handler(ifKit, SensorChangeHandler, NULL);
	CPhidgetInterfaceKit_set_OnInputChange_Handler(ifKit, InputChangeHandler, NULL);
	//*********** @PHIDGETS *************

	//************ WINDOW **************
	namedWindow("IRTRACK", CV_WINDOW_AUTOSIZE);
	createTrackbar("Z INDEX", "IRTRACK", &slider, 299, changeZ);
	//************ @WINDOW **************
	//*************IMAGE ***************
	IplImage *img = cvCreateImage(cvSize(cameraWidth, cameraHeight), IPL_DEPTH_8U, 1);
	IplImage *img2 = cvCreateImage(cvSize(cameraWidth, cameraHeight), IPL_DEPTH_8U, 1);
	Mat imgRGB;
	imgRGB = Mat(cameraHeight, cameraWidth, CV_8UC3);

	vector<CvPoint> points;
	vector<Coordinate> coords;
	int isNew = 1;
	int inc = 0;
	int init = 0;
	int restart = 0;
	int pos = 1;

	while (1)
	{

		if (restart == 1)
		{
			coords.clear();
			points.clear();
			restart = 0;
		}
		FrameGroup *frameGroup = sync->GetFrameGroup();
		if (frameGroup && frameGroup->Count()>1)
		{
			imgRGB.setTo(0);
			for (int k = 0; k < frameGroup->Count(); k++)
			{
				Frame * frame = frameGroup->GetFrame(k);
				
				assert(frame!= NULL);

				frame->Rasterize(cameraWidth, cameraHeight, cameraWidth, 8, imageBuffer);

				img->imageData = (char *)imageBuffer;
				cvFlip(img, NULL, 1);


				CvPoint last = cvPoint(0, 0);
				if (frame->ObjectCount() >= 3)
				{
					Point points[3];
					//for (int i = 0; i < frame->ObjectCount(); i++)
					for (int i = 0; i < 3; i++)
					{
						cObject *obj = frame->Object(i);

						assert(obj!=NULL);

						float x = obj->X();
						float y = obj->Y();

						Core::Undistort2DPoint(lensDistortion, x, y);


						points[i] = Point(x, y);

						int a = (0 * cameraWidth) + (cameraWidth - cvRound(x));
						int b = cvRound(y);
						circle(imgRGB, cvPoint(a, y), obj->Width() / 2, CV_RGB(0, 255, 0));
						char c[30] = "P";
						char c2[30] = "";
						char c3[30] = "(";
						char cx[5] = "100";
						char cy[5] = "100";
						char cw[5] = ""; 
						char ch[5] = "";
						sprintf_s(c2, "%d", k);
						sprintf_s(cx, "%d", a);
						sprintf_s(cy, "%d", b);
						sprintf_s(cw, "%d", obj->Width());
						sprintf_s(ch, "%d", obj->Height());
						strcat_s(c, c2);
						strcat_s(c, c3);
						strcat_s(c, cx);
						strcat_s(c, ", ");
						strcat_s(c, cy);
						strcat_s(c, ")");
						strcat_s(c, "-W:");
						strcat_s(c, cw);
						strcat_s(c, ", H:");
						strcat_s(c, ch);

					}

					//»»»»»»»»»»»»»»
					Point pencil = getPencil(points[0], points[1], points[2]);
					int a = (0 * cameraWidth) + (cameraWidth - cvRound(pencil.x));
					int b = cvRound(pencil.y);
					circle(imgRGB, cvPoint(a, pencil.y),10 / 2, CV_RGB(255, 0, 0));
					char c[30] = "P";
					char c2[30] = "";
					char c3[30] = "(";
					char cx[5] = "100";
					char cy[5] = "100";
					char cw[5] = "";
					char ch[5] = "";
					sprintf_s(c2, "%d", k);
					sprintf_s(cx, "%d", a);
					sprintf_s(cy, "%d", b);
					sprintf_s(cw, "%d",10);
					sprintf_s(ch, "%d", 10);
					strcat_s(c, c2);
					strcat_s(c, c3);
					strcat_s(c, cx);
					strcat_s(c, ", ");
					strcat_s(c, cy);
					strcat_s(c, ")");
					strcat_s(c, "-W:");
					strcat_s(c, cw);
					strcat_s(c, ", H:");
					strcat_s(c, ch);
					//putText(imgRGB, c, cvPoint(x, y), 1,1, cvScalar(255.0, 0.0, 0.0, 0.0));
					//flip(imgRGB, imgRGB, 1);
					putText(imgRGB, c, cvPoint(a, b - 20), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 250, 0), 1, CV_AA);

					if (init == 1)// && isTrack == 1)
					{
						int cx = cvRound(a);
						int cy = cvRound(b);
						if (coords.size() == 0 || (coords.size() > 0 && coords[coords.size() - 1].isEqual(cx, cy, z_indez) == 0))
						{
							if (isNew == 1 && pos > 1)
								coords[coords.size() - 1].setEndCP();
							coords.push_back(Coordinate(inc, cvRound(a), cvRound(b), z_indez, inc, isNew));
							//	points.push_back(cvPoint(cvRound(cameraWidth - x), cvRound(y)));

							for (int i = 0; i < coords.size(); i++)
							{
								CvPoint p1 = cvPoint((cameraWidth - coords[i].getPoint().x), (coords[i].getPoint().y));
								if (i == 0 || coords[i].getNew() == 1)
									cvLine(img, p1, p1, CV_RGB(0, 0, 255), 1);
								else
								{
									CvPoint p0 = cvPoint((cameraWidth - coords[i - 1].getPoint().x), (coords[i - 1].getPoint().y));
									cvLine(img, p0, p1, CV_RGB(0, 0, 255), 1);
								}
							}
							isNew = 0;
							//last = cvPoint(cvRound(x), cvRound(y));
							pos++;
						}
					}
					if (isTrack == 0)
						isNew = 1;
					int radius = 10;
				}
				frame->Release();
			}
			frameGroup->Release();
		}
		inc++;
		//Sleep(2);

		//»»»»»»»»»»»»» WINDOW 
		//cvShowImage("IRTRACK", img);
		imshow("IRTRACK", imgRGB);

		int key = waitKey(2);

		cout << "\n" << key;
		if (key==115) saveFile(coords, cameraWidth);
		else if (key == 114) restart = 1;
		else if (key == 105) init = 1;
		else if (key == 27) break;

	}

	CPhidget_close((CPhidgetHandle)ifKit);
	CPhidget_delete((CPhidgetHandle)ifKit);

	CameraManager::X().Shutdown();
	return 0;
}


