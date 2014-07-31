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
		
	Camera *camera = CameraManager::X().GetCamera();
	if (camera == 0)
	{
		MessageBox(0, L"Please connect a camera", L"No Device Connected", MB_OK);
		return 1;
	}

	int cameraWidth = camera->Width();
	int cameraHeight = camera->Height();

	char imageBuffer[800 * 600];
	

	camera->SetVideoType(SegmentMode);
	camera->Start();
	camera->SetTextOverlay(false);
	
	Core::DistortionModel lensDistortion;
	camera->GetDistortionModel(lensDistortion);

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
		Frame *frame = camera->GetFrame();
		if (frame)
		{
			frame->Rasterize(cameraWidth, cameraHeight, cameraWidth, 8, imageBuffer);

			img->imageData = imageBuffer;
			cvFlip(img, NULL, 1);


			CvPoint last = cvPoint(0,0);
			for (int i = 0; i<frame->ObjectCount(); i++)
			{
				cObject *obj = frame->Object(i);

				float x = obj->X();
				float y = obj->Y();


			//	std::cout << "(" << cvRound(x) << ", " << cvRound(y) << ") \n";

				if (init == 1 && isTrack == 1)
				{
					int cx = cvRound(x);
					int cy = cvRound(y);
					if (coords.size() == 0 || (coords.size() > 0 && coords[coords.size() - 1].isEqual(cx, cy, z_indez) == 0))
					{
						if (isNew == 1 && pos > 1)
							coords[coords.size() - 1].setEndCP();
						coords.push_back(Coordinate(inc, cvRound(x), cvRound(y), z_indez, inc, isNew));
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
				if( isTrack==0)
					isNew = 1;
				int radius = 10;

				Core::Undistort2DPoint(lensDistortion, x, y);
			}
			frame->Release();
		}
		inc++;
		//Sleep(2);

		//»»»»»»»»»»»»» WINDOW 
		cvShowImage("IRTRACK", img);

		int key = waitKey(10);
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


