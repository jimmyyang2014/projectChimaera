#include "cv.h"
#include "highgui.h"
#include <stdlib.h>
#include <stdio.h>

#define DELAY 2

int main(int argc, char** argv){
	// Set up variables
	char window_name[] = "Webcam Test";
	CvCapture *capture = 0;
	IplImage *frame = 0;

	int quit = 0;

	int height,width,step,channels;
	uchar *data;
	int i,j,k;

	//set the camera capture
	capture = cvCaptureFromCAM(0);

	// create a new window with auto sizing
	cvNamedWindow(window_name, 1);

	// create a window
	cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 

	while(quit != 'q'){

		//grab frame from webcam
		frame = cvQueryFrame(capture);

		// display frame
		cvShowImage(window_name,frame);

		height    = frame->height;
		width     = frame->width;
		step      = frame->widthStep;
		channels  = frame->nChannels;
		data      = (uchar *)frame->imageData;
		//printf("Processing a %dx%d image with %d channels\n",height,width,channels); 

		// invert the image
		for(i=0;i<height;i++){
			for(j=0;j<width;j++){
				for(k=0;k<channels;k++){
					data[i*step+j*channels+k]=255-data[i*step+j*channels+k];
				}
			}
		}

		// show the image
		cvShowImage("mainWin", frame );
		

		//check for q to quit
		quit = cvWaitKey(1);

		// delay for a moment, delay is under 2 it doesn't seem to work
		cvWaitKey(DELAY);

	}

	// before quitting it releases memory
	cvReleaseCapture(&capture);
	cvDestroyWindow(window_name);

	return 0;
}
