// Downcam Driver
// Authors: P-LAN, Wurbledood, alexsleat, everto151

//Includes
#include <iostream>
#include <math.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <ros/ros.h>
#include "dwncamDriver.h"
#include "image_transport/image_transport.h"
#include "ini.h"
#include "INIReader.h"
#include "inifile.h"
#include "cv_bridge/CvBridge.h"
#include "sensor_msgs/Image.h"
#include "std_msgs/Char.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Int32.h"

// Defines
// Input Modes: 0 = GSCAM
//              1 = VIDEO
//              2 = IMAGE
//              3 = IPCAM
#define INPUT_MODE		1
#define VIDEO_FILENAME	"Test.avi"
#define IMAGE_FILENAME	"Test.jpg"

//Threshold values
int min_hue_;
int max_hue_;
int min_sat_;
//erode/dilate passes
float erode_passes_;
int dilate_passes_;

// Downcam Class
class Downcam
{

protected:

	ros::NodeHandle nh_;
	//For receiving new threshold values on the fly
	ros::Subscriber min_hue_sub_;
	ros::Subscriber max_hue_sub_;
	ros::Subscriber min_saturation_sub_;
	ros::Subscriber max_saturation_sub_;
	ros::Subscriber min_brightness_sub_;
	ros::Subscriber max_brightness_sub_;
	
	/*Advertises image X, Y and angle*/
	ros::Publisher p_X_;
	ros::Publisher p_Y_; 
	ros::Publisher p_Z_; 
	ros::Publisher p_theta_;
	
	//Messages for sending the data
	std_msgs::Int32 pipe_X_, pipe_Y_;
	std_msgs::Float32 pipe_Z_, pipe_theta_;
	
	image_transport::ImageTransport it_;
	image_transport::Subscriber image_sub_;
	sensor_msgs::CvBridge bridge_;
	IplImage *cv_input_;
	cv::Mat img_in_;
	cv::Mat img_hsv_;
	cv::Mat img_hue_;
	cv::Mat img_sat_;
	cv::Mat img_thresh_;
	cv::Mat img_bin_;
	cv::Mat img_out_;
	cv::Mat img_split[2];
	int  edges_allocated_,
		 edges1_index_, edges1_length_, edges1_marker_, edges2_index_, edges2_length_, edges2_marker_,
		 moving_average_size_, moving_length_, moving_index_, i_, j_, k_, n_, n2_, min_area_percentage_;
	int *edges1_, *edges2_, *blanket1_, *blanket2_;
	//float erode_passes_;
	double sx_, sy_, sxx_, sxy_, alpha_, beta_, sx2_,
		   sy2_, sxx2_, sxy2_, alpha2_, beta2_, alpha3_, beta3_, a1_, a2_, a3_;
	double *moving_alpha1_, *moving_beta1_, *moving_alpha2_, *moving_beta2_;
	CvPoint pipe_centre_, point1_, point2_;
	char angle_text_[50];
	//Counter for taking screenshots
	int screenshot_counter_, screenshot_trigger_,image_counter_;
	char *logpath;
	//filepath for saving images
	char savepath[300];

public:

	Downcam(ros::NodeHandle & nh):nh_ (nh), it_ (nh_)
	{
		char* ass = "dwncam2_config.ini";
		char file_path[300];

		//setup publishers to realease pipe details
		p_X_ = nh_.advertise<std_msgs::Char>("pipe_X", 100);
		p_Y_ = nh_.advertise<std_msgs::Char>("pipe_Y", 100);
		p_Z_ = nh_.advertise<std_msgs::Char>("pipe_Z", 100);
		p_theta_ = nh_.advertise<std_msgs::Char>("pipe_theta", 100);

		// Listen For Image Messages On A Topic And Setup Callback
		ROS_INFO("Getting Cam");
		image_sub_ = it_.subscribe("/gscam/image_raw", 1, &Downcam::imageCallback, this);
		ROS_INFO("Got Cam");
		
		//Listen for new values
		min_hue_sub_ = nh_.subscribe("vision_hue_min", 1000, &Downcam::vision_hue_minCallback, this);
		max_hue_sub_ = nh_.subscribe("vision_hue_max", 1000, &Downcam::vision_hue_maxCallback, this);
		min_saturation_sub_ = nh_.subscribe("vision_saturation_min", 1000, &Downcam::vision_saturation_minCallback, this);
		max_saturation_sub_ = nh_.subscribe("vision_saturation_max", 1000, &Downcam::vision_saturation_maxCallback, this);
		min_brightness_sub_ = nh_.subscribe("vision_brightness_min", 1000, &Downcam::vision_brightness_minCallback, this);
		max_brightness_sub_ = nh_.subscribe("vision_brightness_max", 1000, &Downcam::vision_brightness_maxCallback, this);

		// Open HighGUI Windows
		cv::namedWindow ("input", 1);
//		cv::namedWindow ("thresholded image", 1);
//		cv::namedWindow ("binary image", 1);
//		cv::namedWindow ("segmented output", 1);
		ROS_INFO("Opened Windows");

		//Read in the environmental variable that stores the location of the config files
		char *configpath = getenv("SUB_CONFIG_PATH");
		if (configpath == NULL)
		{
		std::cout << "Problem getting SUB_CONFIG_PATH variable." << std::endl;
		exit(-1);
		}
		//filepath=configpath+filename
		sprintf(file_path, "%s%s", configpath, ass);

		/* New Reading of the Config File here...This class doesn't take default values */
		CIniFile ini;
		ini.Load(file_path);
		
		//Read the values in
		min_hue_ = atoi(ini.GetKeyValue("hue" , "min_hue").c_str());
		max_hue_ = atoi(ini.GetKeyValue("hue" , "max_hue").c_str());			
		min_sat_ = atoi(ini.GetKeyValue("saturation" , "min_sat").c_str());
		erode_passes_ = strtod(ini.GetKeyValue("erode" , "erode_passes").c_str(), NULL);		
		dilate_passes_ = atoi(ini.GetKeyValue("dilate" , "dilate_passes").c_str());			
		min_area_percentage_ = atoi(ini.GetKeyValue("pipe" , "min_area_percentage").c_str());
		moving_average_size_ = atoi(ini.GetKeyValue("angle" , "moving_average_size").c_str());
		screenshot_trigger_ = atoi(ini.GetKeyValue("counters" , "screenshot_trigger").c_str());
		
		//Assume that if both min and max hue = 0, then file reading failed. Setup default vals
		if ((min_hue_ == 0) && (max_hue_ == 0))
		{
			std::cout << "Failed To Load \"~/projectChimaera/Config/dwncam2_config.ini\"\n";
			min_hue_ = 4;
			max_hue_ = 34;
			min_sat_ = 50;
			erode_passes_ = 10;
			dilate_passes_ = 10;
			min_area_percentage_ = 20;
			moving_average_size_ = 10;
			screenshot_trigger_ = 500;
		}
		
		//Print the values out.
		std::cout << "min_hue = " << min_hue_ << "\n";
		std::cout << "max_hue = " << max_hue_ << "\n";
		std::cout << "min_saturation = " << min_sat_ << "\n";
		std::cout << "erode_passes = " << erode_passes_ << "\n";
		std::cout << "dilation_passes = " << dilate_passes_ << "\n";
		std::cout << "min_area_percentage = " << min_area_percentage_ << "\n";
		std::cout << "moving_average_size = " << moving_average_size_ << "\n";

		//Read the location of the log file
		logpath = getenv("SUB_LOG_PATH");
		if (logpath == NULL)
		{
		std::cout << "Problem getting SUB_LOG_PATH variable." << std::endl;
		exit(-1);
		}

		sprintf(savepath, "%s%s", logpath, "/dwncam_images"); 

		//start the screenshot counter
		screenshot_counter_ = 0;
		image_counter_ = 0;
		
		// Make Sure The Moving Average Size Is Greater Than Zero
		if (moving_average_size_ < 1){
			moving_average_size_ = 1;
		}
		
		// Set The Initial Value For Dynamically Allocating The Edge Arrays
		edges_allocated_ = 0;

		// Allocate Memory For The Moving Average Arrays
		moving_alpha1_ = (double*)(malloc(moving_average_size_ * sizeof(double)));
		moving_beta1_ = (double*)(malloc(moving_average_size_ * sizeof(double)));
		moving_alpha2_ = (double*)(malloc(moving_average_size_ * sizeof(double)));
		moving_beta2_ = (double*)(malloc(moving_average_size_ * sizeof(double)));
		
		// Set The Initial Used Length & Index Of The Moving Average Arrays
		moving_length_ = 0;
		moving_index_ = 0;

	}
	
	//Message callback functions
	void vision_hue_minCallback(const std_msgs::Char::ConstPtr& msg)
	{
		uchar data;
		data = msg->data;
		printf("New Hue Value = %u \n", data);
		min_hue_ = (int)data;
		return;
	}
	
	void vision_hue_maxCallback(const std_msgs::Char::ConstPtr& msg)
	{
		uchar data;
		data = msg->data;
		printf("New Brightness Value = %u \n", data);
		max_hue_ = (int)data;
		return;
	}
	
	void vision_saturation_minCallback(const std_msgs::Char::ConstPtr& msg)
	{
		uchar data;
		data = msg->data;
		printf("New Saturation Value = %u \n", data);
		min_sat_ = (int)data;
		return;
	}
	
	void vision_saturation_maxCallback(const std_msgs::Char::ConstPtr& msg)
	{
		uchar data;
		data = msg->data;
		printf("New Brightness Value = %u \n", data);
		return;
	}	
	
	void vision_brightness_minCallback(const std_msgs::Char::ConstPtr& msg)
	{
		uchar data;
		data = msg->data;
		printf("New Brightness Value = %u \n", data);
		return;
	}
	
	void vision_brightness_maxCallback(const std_msgs::Char::ConstPtr& msg)
	{
		uchar data;
		data = msg->data;
		printf("New Brightness Value = %u \n", data);
		return;
	}

	// Find Centre Function
	CvPoint find_centre(cv::Mat *img_input, int min_area)
	{
		int row, col, area = 0, mx = 0, my = 0;
		CvPoint output;
		for (row = 0; row < img_input->rows; row++){
			for (col = 0; col < img_input->cols; col++){
				if (img_input->at<uchar>(row, col) == 255){
					area += 1;
					mx += (row + 1);
					my += (col + 1);
				}
			}
		}
		if (area < min_area || area == 0){
			output.x = -1;
			output.y = -1;
		} else{
			output.x = (int)(my / area);
			output.y = (int)(mx / area);
		}
		return output;
	}



	// Blanket Function
	void blanket(int *input, int *output, int index, int length, int side)
	{
		int i, j, k, bestbefore, bestafter, pixel;
		double currentslope;
		for (i = index; i < index + length; i++){
			output[i] = -1;
			if (input[i] != -1){
				bestbefore = i;
				bestafter = i;
				k = 0;
				for (j = 0; j < 20; j++){
					pixel = i;
					currentslope = 0.0;
					if (bestafter > bestbefore){
						currentslope = (double)((double)(input[bestafter] - input[bestbefore]) / (double)(bestafter - bestbefore));
					}
					if (k == 0){
						pixel = i - (int)(j / 2) - 1;
						if (pixel >= index){
							if (input[pixel] != -1){
								if (side == 0){
									if ((double)((double)(input[bestafter] - input[pixel]) / (double)(bestafter - pixel)) > currentslope){
										bestbefore = pixel;
									}
								} else{
									if ((double)((double)(input[bestafter] - input[pixel]) / (double)(bestafter - pixel)) < currentslope){
										bestbefore = pixel;
									}
								}
							}
						}
					} else{
						pixel = i + (int)((j + 1) / 2);
						if (pixel < index + length){
							if (input[pixel] != -1){
								if (side == 0){
									if ((double)((double)(input[pixel] - input[bestbefore]) / (double)(pixel - bestbefore)) < currentslope){
										bestafter = pixel;
									}
								} else{
									if ((double)((double)(input[pixel] - input[bestbefore]) / (double)(pixel - bestbefore)) > currentslope){
										bestafter = pixel;
									}
								}
							}
						}
					}
					k = 1 - k;
				}
				if (input[bestbefore] != input[bestafter]){
					output[i] = input[bestbefore] + (int)((double)((double)(i - bestbefore) * (double)(input[bestafter] - input[bestbefore]) / (double)(bestafter - bestbefore)));
				} else{
					output[i] = input[bestbefore];
				}
			}
		}
		for (i = index; i < index + length; i++){
			if (output[i] == -1){
				bestbefore = -1;
				bestafter = -1;
				for (j = i - 1; j >= index; j--){
					if (output[j] != -1){
						bestbefore = j;
						break;
					}
				}
				for (j = i + 1; j < index + length; j++){
					if (output[j] != -1){
						bestafter = j;
						break;
					}
				}
				if (bestbefore == -1 && bestafter == -1){
					output[i] = 0;
				} else if (bestbefore == -1){
					output[i] = output[bestafter];
				} else if (bestafter == -1){
					output[i] = output[bestbefore];
				} else{
					output[i] = output[bestbefore] + (int)((double)((double)(i - bestbefore) * (double)(output[bestafter] - output[bestbefore]) / (double)(bestafter - bestbefore)));
				}
			}
		}
		for (i = index; i < index + length; i++){
			pixel = 0;
			bestbefore = 0;
			for (j = i - 5; j <= i + 5; j++){
				if (j >= index && j < index + length){
					pixel += output[j];
					bestbefore += 1;
				}
			}
			output[i] = (int)(pixel / bestbefore);
		}
	}



	// Dilate Image Function
	void dilate_image(const cv::Mat* img_src_, cv::Mat* img_dst_, int passes)
	{
		int i, j;
		cv::Mat_<uchar> img_dilate_;
		cv::Mat_ <uchar> img_inv_;
		cv::Mat_ <float> distance_map_;
		//if passes == 0 don't try and execute the loop, return gracefully
		if(passes == 0){return;}
		//copy the input image so we don't accidently doitwise_not something silly to it
		img_dilate_ = img_src_->clone ();
		//As the distance map contains the distance for each pixel to the closest black pixel, to dilate
		//we will need to know the distance to the closest white pixel, easiest way is probably
		//to invert the input image
		cv::bitwise_not(img_dilate_, img_inv_); 
		//create a distance map (calculates each pixels distance from the closest black pixel)
		cv::distanceTransform(img_inv_, distance_map_, CV_DIST_L2, 3);//CV_DIST_L2 and 3x3 apparently gives a fast, coarse estimate (should be good enough for what we need)
		//do the dirty work, this is basically a threshold of the distance map with the results 
		//applied to the input image
		for(i = 0; i < (img_dilate_.rows); i++)
		{
				for(j = 0; j < (img_dilate_.cols); j++)
				{
					//don't waste time trying to turn pixels that are already white
					if(((int)(distance_map_(i,j)) <= passes) && (distance_map_(i,j) != 0.0))
					{
						//if the distance from the current pixel to the closest black (background)
						//pixel is less or equal to the desired number of passes turn it black
						img_dilate_(i,j) = 255;
					}
				}
			}
			//send the new image on its way and release the image memory
			*img_dst_ = img_dilate_.clone();
			img_dilate_.release();
			img_inv_.release();
			
			return;
	}



	// Erode Image Function
	void erode_image(const cv::Mat* img_src_, cv::Mat* img_dst_, float passes)
	{
		int i, j;
		cv::Mat_<uchar> img_erode_;
		cv::Mat_ <float> distance_map_;
		//if passes == 0 don't try and execute the loop, return gracefully
		if(passes == 0){return;}
		//copy the input image so we don't accidently do something silly to it
		img_erode_ = img_src_->clone ();
		//create a distance map (calculates each pixels distance from the closest black pixel)
		cv::distanceTransform(img_erode_, distance_map_, CV_DIST_L2, 3);//CV_DIST_L2 and a mask of 3x3 apparently gives a fast coarse estimate
		//do the dirty work, this is basically a threshold of the distance map with the results 
		//applied to the input image
		for(i = 0; i < (img_erode_.rows); i++)
		{
				for(j = 0; j < (img_erode_.cols); j++)
				{
					//don't waste time trying to turn pixels that are already black to black
					if(((int)(distance_map_(i,j)) <= passes) && (distance_map_(i,j) != 0.0))
					{
						//if the distance from the current pixel to the closest black (background)
						//pixel is less or equal to the desired number of passes turn it black
						img_erode_(i,j) = 0;
					}
				}
			}
			//send the new image on its way and release the image memory
			*img_dst_ = img_erode_.clone();
			img_erode_.release();
			
			return;	
	}

	int map(long x, long in_min, long in_max, long out_min, long out_max)
	{
		return (int)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
	}

	// Image Callback Function
	void imageCallback(const sensor_msgs::ImageConstPtr & msg_ptr)
	{

		// Declare Variables
		const int from_to[] = {0, 0, 1, 1};

		// Convert ROS Input Image Message To IplImage
		try{
			cv_input_ = bridge_.imgMsgToCv(msg_ptr, "bgr8");
 		} catch (sensor_msgs::CvBridgeException error){
			ROS_ERROR("CvBridge Input Error");
 		}
 
		// Convert IplImage To cv::Mat
		img_in_ = cv::Mat(cv_input_).clone();

		// Convert The Input Image From BGR To HSV
		cv::cvtColor (img_in_, img_hsv_, CV_BGR2HSV);

		// Initialise The Hue, Sat & Thresh Images
		img_hue_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		img_sat_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		img_thresh_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		
		// Get The Hue & Sat Channels From The HSV Image
		img_split[0] = img_hue_;
		img_split[1] = img_sat_;
		cv::mixChannels(&img_hsv_, 3, img_split, 2, from_to, 2);

		// Threshold The Image
		for (i_ = 0; i_ < img_hue_.rows; i_++){
			for (j_ = 0; j_ < img_hue_.cols; j_++){
				if (img_hue_.at<uchar>(i_, j_) >= min_hue_ &&
				img_hue_.at<uchar>(i_, j_) <= max_hue_ &&
				img_sat_.at<uchar>(i_, j_) >= min_sat_){
					img_thresh_.at<uchar>(i_, j_) = 255;
				}
			}
		}

		/* After thresholding the image can still be very 'messy' 
		 * with large amounts of noise. This can cause the centre to be 
		 * found incorrectly and cause trouble when we try to find
		 * the pipes angle and estimated size/distance, so it needs to 
		 * be cleaned up.
		 */

		// Create A Copy Of The Thresholded Image
		img_bin_ = img_thresh_.clone();

		// Close The Image Using Erosion & Dilation
		erode_image(&img_bin_, &img_bin_, erode_passes_);
		dilate_image(&img_bin_, &img_bin_, dilate_passes_);
		
		// Perform A Median Blue On The Image - Not Convinced That This Helps Much
		//cv::medianBlur(img_bin_, img_bin_, 9);
		
		// Create The Segmented Output Image
		img_out_ = img_in_.clone();
		for (i_ = 0; i_ < img_out_.rows; i_++){
			for (j_ = 0; j_ < img_out_.cols; j_++){
				if (img_bin_.at<uchar>(i_, j_) == 0){
					img_out_.at<uchar>(i_, (j_ * 3)) = 0;
					img_out_.at<uchar>(i_, (j_ * 3) + 1) = 0;
					img_out_.at<uchar>(i_, (j_ * 3) + 2) = 0;
				}
			}
		}

		// Find The Centre Of The Pipe
		pipe_centre_ = find_centre(&img_bin_, (int)((double)((double)(img_bin_.rows) *
		(double)(img_bin_.cols) * (double)(min_area_percentage_) / 100.0)));

		// If We Have Found A Pipe
		if (pipe_centre_.x >= 0){

			// Dynamically Allocate Memory For The Edge Arrays if Required
			if (edges_allocated_ != img_bin_.rows){
				edges1_ = (int*)(malloc(img_bin_.rows * sizeof(int)));
				edges2_ = (int*)(malloc(img_bin_.rows * sizeof(int)));
				blanket1_ = (int*)(malloc(img_bin_.rows * sizeof(int)));
				blanket2_ = (int*)(malloc(img_bin_.rows * sizeof(int)));
				edges_allocated_ = img_bin_.rows;
			}

			// Get The Edges
			edges1_index_ = -1;
			edges1_length_ = -1;
			edges1_marker_ = -1;
			edges2_index_ = -1;
			edges2_length_ = -1;
			edges2_marker_ = -1;
			for (i_ = 0; i_ < img_bin_.rows; i_++){
				edges1_[i_] = -1;
				for (j_ = 0; j_ < img_bin_.cols; j_++){
					if (img_bin_.at<uchar>(i_, j_) == 255){
						edges1_[i_] = j_;
						if (edges1_marker_ == -1){
							edges1_marker_ = i_;
						}
						break;
					}
				}
				if (edges1_[i_] == -1 || i_ == img_bin_.rows - 1){
					if (edges1_marker_ != -1){
						if (edges1_[i_] == -1){
							if (i_ - edges1_marker_ > edges1_length_){
								edges1_index_ = edges1_marker_;
								edges1_length_ = i_ - edges1_marker_;
							}
						} else{
							if (i_ + 1 - edges1_marker_ > edges1_length_){
								edges1_index_ = edges1_marker_;
								edges1_length_ = i_ + 1 - edges1_marker_;
							}
						}
						edges1_marker_ = -1;
					}
				}
				edges2_[i_] = -1;
				for (j_ = img_bin_.cols - 1; j_ >= 0; j_--){
					if (img_bin_.at<uchar>(i_, j_) == 255){
						edges2_[i_] = j_;
						if (edges2_marker_ == -1){
							edges2_marker_ = i_;
						}
						break;
					}
				}
				if (edges2_[i_] == -1 || i_ == img_bin_.rows - 1){
					if (edges2_marker_ != -1){
						if (edges2_[i_] == -1){
							if (i_ - edges2_marker_ > edges2_length_){
								edges2_index_ = edges2_marker_;
								edges2_length_ = i_ - edges2_marker_;
							}
						} else{
							if (i_ + 1 - edges2_marker_ > edges2_length_){
								edges2_index_ = edges2_marker_;
								edges2_length_ = i_ + 1 - edges2_marker_;
							}
						}
						edges2_marker_ = -1;
					}
				}
			}

			// If We Have Edges
			if (edges1_index_ >= 0 && edges2_index_ >= 0){

				// Blanket The Left & Right Edges
				blanket(edges1_, blanket1_, edges1_index_, edges1_length_, 0);
				blanket(edges2_, blanket2_, edges2_index_, edges2_length_, 1);
				
				// TEMPORARY SECTION ########### FOR DEBUGGING
				img_thresh_ = cv::Mat::zeros(img_thresh_.rows, img_thresh_.cols, CV_8UC3);
				for (i_ = 0; i_ < edges1_length_; i_++){
					img_thresh_.at<uchar>(edges1_index_ + i_, 3 * edges1_[edges1_index_ + i_]) = 255;
				}
				for (i_ = 0; i_ < edges2_length_; i_++){
					img_thresh_.at<uchar>(edges2_index_ + i_, (3 * edges2_[edges2_index_ + i_]) + 1) = 255;
				}
				for (i_ = 0; i_ < edges1_length_; i_++){
					img_thresh_.at<uchar>(edges1_index_ + i_, (3 * blanket1_[edges1_index_ + i_]) + 2) = 255;
				}
				for (i_ = 0; i_ < edges2_length_; i_++){
					img_thresh_.at<uchar>(edges2_index_ + i_, (3 * blanket2_[edges2_index_ + i_]) + 1) = 255;
					img_thresh_.at<uchar>(edges2_index_ + i_, (3 * blanket2_[edges2_index_ + i_]) + 2) = 255;
				}

				// Calculate The Variables Required For Simple Linear Regression On Each Side Of The Pipe
				n_ = edges1_length_;
				sx_ = 0.0;
				sy_ = 0.0;
				sxx_ = 0.0;
				sxy_ = 0.0;
				for (i_ = 0; i_ < edges1_length_; i_++){
					sx_ += (double)(blanket1_[edges1_index_ + i_]);
					sy_ += (double)(edges1_index_ + i_);
					sxx_ += (double)(pow(blanket1_[edges1_index_ + i_], 2.0));
					sxy_ += (double)((double)(edges1_index_ + i_) * (double)(blanket1_[edges1_index_ + i_]));
				}
				n2_ = edges2_length_;
				sx2_ = 0.0;
				sy2_ = 0.0;
				sxx2_ = 0.0;
				sxy2_ = 0.0;
				for (i_ = 0; i_ < edges2_length_; i_++){
					sx2_ += (double)(blanket2_[edges2_index_ + i_]);
					sy2_ += (double)(edges2_index_ + i_);
					sxx2_ += (double)(pow(blanket2_[edges2_index_ + i_], 2.0));
					sxy2_ += (double)((double)(edges2_index_ + i_) * (double)(blanket2_[edges2_index_ + i_]));
				}

				// Get The Left Line
				beta_ = (double)(((double)(n_ * sxy_) - (double)(sx_ * sy_)) / ((double)(n_ * sxx_) - (double)(pow(sx_, 2))));
				alpha_ = (double)((double)(sy_ / (double)(n_)) - (double)(beta_ * sx_ / (double)(n_)));
				if (isnanf(beta_) || beta_ > 999){
					beta_ = 999;
				}
				moving_alpha1_[moving_index_] = alpha_;
				moving_beta1_[moving_index_] = beta_;

				// Get The Right Line
				beta2_ = (double)(((double)(n2_ * sxy2_) - (double)(sx2_ * sy2_)) / ((double)(n2_ * sxx2_) - (double)(pow(sx2_, 2))));
				if (isnanf(beta2_) || beta2_ > 999){
					beta2_ = 999;
				}
				alpha2_ = (double)((double)(sy2_ / (double)(n2_)) - (double)(beta2_ * sx2_ / (double)(n2_)));
				moving_alpha2_[moving_index_] = alpha2_;
				moving_beta2_[moving_index_] = beta2_;

				// Update The Moving Average Index & Length
				moving_index_ += 1;
				if (moving_index_ > moving_length_){
					moving_length_ = moving_index_;
				}
				if (moving_index_ >= moving_average_size_){
					moving_index_ = 0;
				}

				// Get The Moving Average Lines
				alpha_ = 0.0;
				beta_ = 0.0;
				alpha2_ = 0.0;
				beta2_ = 0.0;
				for (i_ = 0; i_ < moving_length_; i_++){
					alpha_ += moving_alpha1_[i_];
					beta_ += moving_beta1_[i_];
					alpha2_ += moving_alpha2_[i_];
					beta2_ += moving_beta2_[i_];
				}
				alpha_ /= moving_length_;
				beta_ /= moving_length_;
				alpha2_ /= moving_length_;
				beta2_ /= moving_length_;

				// Plot The Left Line
				point1_.x = (int)(-alpha_ / beta_);
				point1_.y = 0;
				point2_.x = (int)((double)(img_bin_.rows - 1.0 - alpha_) / beta_);
				point2_.y = img_bin_.rows - 1;
				//line(img_bin_, point1_, point2_, cvScalar(128, 128, 128, 0), 1, 8, 0);
				line(img_in_, point1_, point2_, cvScalar(0, 0, 255, 0), 1, 8, 0);

				// Plot The Right Line
				point1_.x = (int)(-alpha2_ / beta2_);
				point1_.y = 0;
				point2_.x = (int)((double)(img_bin_.rows - 1.0 - alpha2_) / beta2_);
				point2_.y = img_bin_.rows - 1;
				//line(img_bin_, point1_, point2_, cvScalar(128, 128, 128, 0), 1, 8, 0);
				line(img_in_, point1_, point2_, cvScalar(0, 0, 255, 0), 1, 8, 0);

				// Get The Central Line
				a1_ = (double)(atan(fabs(beta_)));
				a2_ = (double)(atan(fabs(beta2_)));
				if (a1_ < 0.0){
					a1_ += CV_PI;
				}
				if (a2_ < 0.0){
					a2_ += CV_PI;
				}
				if (beta_ >= 0.0 && beta2_ >= 0.0){
					a3_ = (double)((double)(a1_ + a2_) / 2.0);
					beta3_ = tan(a3_);
				} else if (beta_ < 0.0 && beta2_ < 0.0){
					a3_ = (double)((double)(a1_ + a2_) / 2.0);
					beta3_ = -tan(a3_);
				} else{
					a3_ = a1_ + (double)((double)(CV_PI - a1_ - a2_) / 2.0);
					if (a3_ < (double)(CV_PI / 2.0)){
						beta3_ = -tan(a3_);
					} else{
						a3_ = CV_PI - a3_;
						beta3_ = tan(a3_);
					}
				}
				if (isnanf(beta3_) || beta3_ > 999){
					beta3_ = 999;
				}
				if (beta_ == beta2_){
					point1_.x = 0;
					point1_.y = (double)((double)(alpha_ + alpha2_) / 2.0);
				} else{
					point1_.x = (double)((double)(alpha2_ - alpha_) / (double)(beta_ - beta2_));
					point1_.y = (double)(alpha_ + (double)(beta_ * point1_.x));
				}
				alpha3_ = (double)(point1_.y) - (double)(beta3_ * point1_.x);
				point1_.x = (int)(-alpha3_ / beta3_);
				point1_.y = 0;
				point2_.x = (int)((double)(img_bin_.rows - 1.0 - alpha3_) / beta3_);
				point2_.y = img_bin_.rows - 1;
				line(img_in_, point1_, point2_, cvScalar(255, 0, 0, 0), 1, 8, 0);

				// Get The Angle
				a1_ = (double)((double)(atan(fabs(beta3_))) * 180.0 / (double)(CV_PI));
				if (a1_ < 0.0){
					a1_ += 180.0;
				}
				if (beta3_ >= 0.0){
					a1_ -= 90.0;
				} else{
					a1_ = 90.0 - a1_;
				}
				sprintf(angle_text_, "Pipe Angle: %.2f", a1_);
				putText(img_in_, angle_text_, cvPoint(50, 75), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 0, 0), 1, CV_AA);

				// Display The Pipe Centre Point On The Input Image
				point1_.x = pipe_centre_.x - 100;
				point1_.y = pipe_centre_.y;
				point2_.x = pipe_centre_.x + 100;
				point2_.y = pipe_centre_.y;
				line(img_in_, point1_, point2_, cvScalar(255, 0, 255, 0), 1, 8, 0);
				point1_.x = pipe_centre_.x;
				point1_.y = pipe_centre_.y - 100;
				point2_.x = pipe_centre_.x;
				point2_.y = pipe_centre_.y + 100;
				line(img_in_, point1_, point2_, cvScalar(255, 0, 255, 0), 1, 8, 0);

				// Get The Z Distance
				n_ = 0;
				n2_ = 0;
				k_ = 0;
				for (i_ = 0; i_ < img_bin_.rows; i_++){
					point1_.x = (int)((double)((double)(i_) - alpha3_) / beta3_);
					if (point1_.x >= 0 && point1_.x < img_bin_.cols){
						n2_ = 0;
						point1_.y = i_;
						for (j_ = point1_.x - 1; j_ >= 0; j_--){
							point1_.y -= (int)(-1.0 / beta3_);
							if (point1_.y >= 0 && point1_.y < img_bin_.rows){
								if (img_bin_.at<uchar>(point1_.y, j_) == 255){
									k_ += 1;
									if (n2_ == 0){
										n_ += 1;
										n2_ = 1;
									}
								} else{
									break;
								}
							} else{
								break;
							}
						}
						point1_.y = i_;
						for (j_ = point1_.x + 1; j_ < img_bin_.cols; j_++){
							point1_.y += (int)(-1.0 / beta3_);
							if (point1_.y >= 0 && point1_.y < img_bin_.rows){
								if (img_bin_.at<uchar>(point1_.y, j_) == 255){
									k_ += 1;
									if (n2_ == 0){
										n_ += 1;
										n2_ = 1;
									}
								} else{
									break;
								}
							} else{
								break;
							}
						}
					}
				}
				if (n_ > 0){
					a2_ = (double)(100.0 * (double)(n_) / (double)(k_));
				} else{
					a2_ = 100.0;
				}
				sprintf(angle_text_, "Z Distance: %.2f", a2_);
				putText(img_in_, angle_text_, cvPoint(250, 75), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 0, 0), 1, CV_AA);

				//save image when pipe first found and every X frames following
				if((screenshot_counter_ == 0)||(screenshot_counter_ == screenshot_trigger_))
				{
					//create the filepath from the image counter and the image name
					sprintf(savepath, "%simg_in_%i.jpg",logpath, image_counter_);
					//save
					cv::imwrite(savepath, img_in_);
					image_counter_++;
					screenshot_counter_ = 0;
				}
				//counter will only be 0 once, so we will always getr an image saved of the
				//first contact with the pipe
				screenshot_counter_++;
				//normalise the coordinates to 0->200
				//normalise coordinates from image size to -100 -> +100
			    pipe_centre_.x = map(pipe_centre_.x, 0, img_in_.cols, 0, 200);
				pipe_centre_.y = map(pipe_centre_.y, 0, img_in_.rows, 0, 200);
				// Publish The Centre X, Centre Y, Z Distance & Angle To ROS
				//X 
				pipe_X_.data = pipe_centre_.x; 
				p_X_.publish(pipe_X_);
				//Y
				pipe_Y_.data = pipe_centre_.y; 
				p_Y_.publish(pipe_Y_);
				//Z
				pipe_Z_.data = a2_; 
				p_Z_.publish(pipe_Z_);
				//theta
				pipe_theta_.data = a1_; 
				p_theta_.publish(pipe_theta_);
			}

		} else{
				pipe_centre_.x = -1;
				pipe_centre_.y = -1;
				// Publish -1 signifying no pipe
				//X 
				pipe_X_.data = pipe_centre_.x; 
				p_X_.publish(pipe_X_);
				//Y
				pipe_Y_.data = pipe_centre_.y; 
				p_Y_.publish(pipe_Y_);
			// Reset The Used Length & Index Of The Moving Average Arrays
			moving_length_ = 0;
			moving_index_ = 0;

		}

		// Display The Input image
		cv::imshow ("input", img_in_);
		// Display The Thresholded Image
		//cv::imshow ("thresholded image", img_thresh_);
		// Display The Binary Image
		//cv::imshow ("binary image", img_bin_);
		// Display The Segmented Image
		//cv::imshow ("segmented output", img_out_);
		
		// Allow The HighGUI Windows To Stay Open
		cv::waitKey(3);

	}
	
};



int main(int argc, char **argv)
{

	// Declare Variables
	cv::Mat video_frame;
	IplImage video_frame2;
	sensor_msgs::CvBridge bridge2_;

 	// Initialize ROS Node
 	ros::init(argc, argv, "ihr_demo1");
 
	// Start Node & Create A Node Handle
	ros::NodeHandle nh;

 	// Instantiate Demo Object
	ROS_INFO("Online");
	Downcam d(nh);

  	// Spin & Provide An Input If Necessary
	if (INPUT_MODE == 0){
		ros::spin();
	} else if (INPUT_MODE == 1){
		cv::VideoCapture cap(VIDEO_FILENAME);
		if(!cap.isOpened()){
			return -1;
		}
		while(ros::ok()){
			ros::spinOnce();
			if (cap.grab()){
				cap.retrieve(video_frame);
				video_frame2 = video_frame;
				d.imageCallback(bridge2_.cvToImgMsg(&video_frame2, "passthrough"));
			} else{
				cap.release();
				cap.open(VIDEO_FILENAME);
				if(!cap.isOpened()){
					return -1;
				}
			}
		}
	} else if (INPUT_MODE == 2){
		while(ros::ok()){
			ros::spinOnce();
			video_frame = cvLoadImage(IMAGE_FILENAME, CV_LOAD_IMAGE_COLOR);
			video_frame2 = video_frame;
			d.imageCallback(bridge2_.cvToImgMsg(&video_frame2, "passthrough"));
		}
	} else if (INPUT_MODE == 3){
		cv::VideoCapture vcap;
		//http://192.168.2.30/mjpg/video.mjpg
		const std::string videoStreamAddress = "rtsp://192.168.2.30:554/ipcam.sdp";
		/* it may be an address of an mjpeg stream, 
		e.g. "http://user:pass@cam_address:8081/cgi/mjpg/mjpg.cgi?.mjpg" */
		//open the video stream and make sure it's opened
		if (!vcap.open(videoStreamAddress)){
			std::cout << "Error opening video stream" << std::endl;
			return -1;
		}
		while (ros::ok()){
			ros::spinOnce();
			while (!vcap.read(video_frame)){
				std::cout << "No frame" << std::endl;
				vcap.release();
				vcap.open(videoStreamAddress);
			}
			video_frame2 = video_frame;
			d.imageCallback(bridge2_.cvToImgMsg(&video_frame2, "passthrough"));
		}
	} else{
		ros::spin();

	}

	// Return
	return 0;

}
