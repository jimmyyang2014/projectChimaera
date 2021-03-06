//Includes
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <ros/ros.h>
#include "std_msgs/UInt32.h"

//motor PWM values, 1500 is stop, 1000 full reverse, 2000 full forward
int depth_right = 500;
int depth_left = 500;
int yaw_left = 500;
int yaw_right = 500;
int pitch = 500;
int go = 0;

ros::Publisher p_depth_right;
ros::Publisher p_depth_left; 
ros::Publisher p_yaw_right;
ros::Publisher p_yaw_left;
ros::Publisher p_pitch;
ros::Publisher p_go;

std_msgs::UInt32 msgDR;
std_msgs::UInt32 msgDL;
std_msgs::UInt32 msgYR;
std_msgs::UInt32 msgYL;
std_msgs::UInt32 msgP;
std_msgs::UInt32 msgGO;

//callback functions for each slider

void depth_right_cb(int pos)
{
	msgDR.data = pos+1000;
	p_depth_right.publish(msgDR);
}

void depth_left_cb(int pos)
{
	msgDL.data = pos+1000;
	p_depth_left.publish(msgDL);
}

void yaw_right_cb(int pos)
{
	//printf("\nYaw right = %d\n", pos+1000);
	msgYR.data = pos+1000;
	p_yaw_right.publish(msgYR);
}

void yaw_left_cb(int pos)
{
	msgYL.data = pos+1000;
	p_yaw_left.publish(msgYL);
}

void pitch_cb(int pos)
{
	msgP.data = pos+1000;
/*	switch (pos)
	{
		case 0: 
			msgP.data = 1000;
			break;
		case 1:
			msgP.data = 1500;
			break;
		case 2: 
			msgP.data = 2000;
			break;
	}
*/
	p_pitch.publish(msgP);
}


void go_cb(int pos)
{
	msgGO.data = pos;
	printf("Go Signal = %i\n", pos);
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "motorspammer");	//Initialize the node
		
	//Our node handle
	ros::NodeHandle motorspammer_nh;
	
	/*Advertises our various messages*/
	p_depth_right = motorspammer_nh.advertise<std_msgs::UInt32>("pidRampDepthRight", 100);
	p_depth_left = motorspammer_nh.advertise<std_msgs::UInt32>("pidRampDepthLeft", 100); 
	p_yaw_right = motorspammer_nh.advertise<std_msgs::UInt32>("pidRampYawRight", 100);
	p_yaw_left = motorspammer_nh.advertise<std_msgs::UInt32>("pidRampYawLeft", 100);
	p_pitch = motorspammer_nh.advertise<std_msgs::UInt32>("pidRampPitch", 100);
	
	p_go = motorspammer_nh.advertise<std_msgs::UInt32>("pilotGo", 100);	
	
	//create the window
	cvNamedWindow("MotorSpammer",0);
	
	//Create track Bars
	cvCreateTrackbar("Depth Right (CH1)","MotorSpammer", &depth_right,1000,&depth_right_cb);
	cvCreateTrackbar("Depth Left (CH2)_","MotorSpammer", &depth_left,1000,&depth_left_cb);
	cvCreateTrackbar("Yaw Right (CH3)___","MotorSpammer", &yaw_right,1000,&yaw_right_cb);	
	cvCreateTrackbar("Yaw Left (CH4)__","MotorSpammer", &yaw_left,1000,&yaw_left_cb);
	cvCreateTrackbar("PITCH (CH5)______","MotorSpammer", &pitch,1000,&pitch_cb);	
	
	cvCreateTrackbar("GO", "MotorSpammer", &go, 1, &go_cb);

//	msgDR.data = msgDL.data = msgYR.data = msgYL.data = msgP.data = 1500;

	while (ros::ok())
	{

		// Allow The HighGUI Windows To Stay Open
		cv::waitKey(3);
			
	//		p_depth_right.publish(msgDR);
	//		p_depth_left.publish(msgDL);
	//		p_yaw_right.publish(msgYR);
	//		p_yaw_left.publish(msgYL);
	//		p_pitch.publish(msgP);
			p_go.publish(msgGO);
	
	}
}
