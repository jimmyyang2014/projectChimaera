#include <ros/ros.h>
#include "sensor_msgs/Image.h"
#include "image_transport/image_transport.h"
#include "cv_bridge/CvBridge.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#include "dwncamDriver.h"
//For Reading Config Files
#include "ini.h"
#include "INIReader.h"
#include <iostream>

//Threshold values
int min_hue;
int max_hue;
int min_sat;
//erode/dilate passes
int erode_passes;
int dilate_passes;
	
// ROS/OpenCV HSV Demo
// Based on http://www.ros.org/wiki/cv_bridge/Tutorials/UsingCvBridgeToConvertBetweenROSImagesAndOpenCVImages

class Demo
{
	
typedef cv::Vec<uchar, 1> Vec1b;	
typedef cv::Vec<uchar, 3> Vec3b;

protected:
	ros::NodeHandle nh_;
	image_transport::ImageTransport it_;
	image_transport::Subscriber image_sub_;
	sensor_msgs::CvBridge bridge_;
	
	cv::Mat img_in_;
	cv::Mat img_in_proc_;
	cv::Mat img_hsv_;
	cv::Mat img_hue_;
	cv::Mat img_sat_;
	cv::Mat img_bin_;
	cv::Mat img_out_;
	cv::Mat img_tmp_;
	cv::Mat img_regions_;
	
	IplImage *cv_input_;
	//to hold Ipl of input image so histograms can be generated
	IplImage *img_inh_;
	
	int regions_allocated_, pixelcount_, region_count_, largest_regions_count_, pixels_passed_, is_pipe_, i_, j_, k_, l_, m_, n_, n2_;
	int *regions_, *region_lengths_, *largest_regions_;
	double sx_, sy_, sxx_, sxy_, alpha_, beta_, sx2_, sy2_, sxx2_, sxy2_, alpha2_, beta2_;
	CvPoint point1_, point2_;


public:

	Demo(ros::NodeHandle & nh):nh_ (nh), it_ (nh_)
	{
		ROS_INFO("Getting Cam");
		// Listen for image messages on a topic and setup callback
		image_sub_ = it_.subscribe ("/gscam/image_raw", 1, &Demo::imageCallback, this);
		// Open HighGUI Window
		ROS_INFO("Got Cam");
		cv::namedWindow ("input", 1);
		cv::namedWindow ("binary image", 1);
		//cv::namedWindow ("segmented output", 1);
		//cv::namedWindow ("preprocessed image", 1);
		ROS_INFO("Opened window?");
		regions_allocated_ = 0;
		pixelcount_ = -1;
	}


	void findCentre(void)
	{
		int x, y, count, x_estimate, y_estimate, x_centre, y_centre;
		int x_draw1, x_draw2, y_draw1, y_draw2;
		CvScalar s;

		x_estimate = y_estimate = count = 0;

		IplImage ipl_img = img_bin_;
		IplImage ipl_seg = img_out_;
		//use the first order moments to find centre of black region
		for(x=0;x<ipl_img.height;x++)
		{
			for(y=0;y<ipl_img.width;y++)
			{
				s = cvGet2D(&ipl_img,x,y);
				if(s.val[0] == 255)
				{ //if we have found a white pixel
					count++; //increase counters
					x_estimate += x;
					y_estimate += y;
				}
			}
		}

		if(count < 3000)
		{ //arbritrary size threshold
			printf("No Target in sight saw only %d\n",count);
		}
		else
		{
			x_centre = y_estimate / count; //estimate center of x
			y_centre = x_estimate / count; //and y


			x_draw1 = x_centre + 100; //this is for drawing only
			x_draw2 = x_centre - 100;
			y_draw1 = y_centre + 100;
			y_draw2 = y_centre - 100;

			s.val[0] = 100;
			s.val[1] = 100;
			s.val[2] = 100;

			CvPoint pt1 = {x_draw1,y_centre};
			CvPoint pt2 = {x_draw2,y_centre};
			CvPoint pt3 = {x_centre,y_draw1};
			CvPoint pt4 = {x_centre,y_draw2};
			//Draw cross to threshold image
			cvLine(&ipl_img, pt1 , pt2, s, 1, 8,0);
			cvLine(&ipl_img, pt3 , pt4, s, 1, 8,0);

			s.val[0] = 0;
			s.val[1] = 0;
			s.val[2] = 255;
			//Draw cross to segmented image
			cvLine(&ipl_seg, pt1 , pt2, s, 1, 8,0);
			cvLine(&ipl_seg, pt3 , pt4, s, 1, 8,0);

			img_bin_ = cv::Mat (&ipl_img).clone ();
			img_out_ = cv::Mat (&ipl_seg).clone ();

			printf("X: %d Y: %d Count: %d Yeahhhhhhhhhhhhhhhhhhhhhh buoy!\n",x_centre,y_centre,count);
		}

		return;
	}
	
	/******************Function for drawing the histogram********************/
		//Input: Histogram generated by CvCalcHist
		//		 Scales for incraesing/decreasing histogarm size
		//Output: IplImage that contains the histogram
	/***********************************************************************/
	IplImage* DrawHistogram(CvHistogram *hist, float scaleX=1, float scaleY=1)
	{
		float histMax = 0;
		cvGetMinMaxHistValue(hist, 0, &histMax, 0, 0);

		//create and blank out an image of the desired size:

		IplImage* imgHist = cvCreateImage(cvSize(256*scaleX, 64*scaleY), 8 ,1);
		cvZero(imgHist);		
		//run through the histogram and draw the bars to the image
	    for(int i=0;i<255;i++)
		{
			float histValue = cvQueryHistValue_1D(hist, i);
			float nextValue = cvQueryHistValue_1D(hist, i+1);
	 
			CvPoint pt1 = cvPoint(i*scaleX, 64*scaleY);
			CvPoint pt2 = cvPoint(i*scaleX+scaleX, 64*scaleY);
			CvPoint pt3 = cvPoint(i*scaleX+scaleX, (64-nextValue*64/histMax)*scaleY);
			CvPoint pt4 = cvPoint(i*scaleX, (64-histValue*64/histMax)*scaleY);
	 
			int numPts = 5;
			CvPoint pts[] = {pt1, pt2, pt3, pt4, pt1};
	 
			cvFillConvexPoly(imgHist, pts, numPts, cvScalar(255));
		}
		
		return imgHist;
	}
	//dilate function
	void dilate_image(const cv::Mat* img_src_, cv::Mat* img_dst_, int passes)
	{
		int i, j, k;
		int marker = 2;
		//if passes is zereo return
		if(passes == 0){return;}
		//convert to cv::Mat_ for ease of pixel access
		cv::Mat_<uchar> img_dilate_;// =  cv::Mat::zeros(img_src_->rows, img_src_->cols, CV_8U);
		//img_dilate_ = img_src_;
		img_dilate_ = img_src_->clone ();
		
		for(k = 0; k < passes; k++)
		{
			//Find a seed pixel
			//Note bounds restriced 
			for(i = 1; i < (img_dilate_.rows - 1); i++)
			{
					for(j = 1; j < (img_dilate_.cols - 1); j++)
					{
						if(marker == 2)
						{
							if(img_dilate_(i,j) == 255)
							{
								//check 8 neighbouring pixels, mark any black ones
								if(img_dilate_(i-1,j-1) == 0){img_dilate_(i-1,j-1) = marker;}
								if(img_dilate_(i-1,j) == 0){img_dilate_(i-1,j) = marker;}
								if(img_dilate_(i-1,j+1) == 0){img_dilate_(i-1,j+1) = marker;}
								if(img_dilate_(i,j-1) == 0){img_dilate_(i,j-1) = marker;}
								if(img_dilate_(i,j+1) == 0){img_dilate_(i,j+1) = 1;}
								if(img_dilate_(i+1,j-1) == 0){img_dilate_(i,j-1) = marker;}
								if(img_dilate_(i+1,j) == 0){img_dilate_(i,j) = marker;}
								if(img_dilate_(i+1,j+1) == 0){img_dilate_(i,j+1) = marker;}
							}
						}
						else
						{
							if(img_dilate_(i,j) == marker - 1)
							{
								//check 8 neighbouring pixels, mark any black ones
								if(img_dilate_(i-1,j-1) == 0){img_dilate_(i-1,j-1) = marker;}
								if(img_dilate_(i-1,j) == 0){img_dilate_(i-1,j) = marker;}
								if(img_dilate_(i-1,j+1) == 0){img_dilate_(i-1,j+1) = marker;}
								if(img_dilate_(i,j-1) == 0){img_dilate_(i,j-1) = marker;}
								if(img_dilate_(i,j+1) == 0){img_dilate_(i,j+1) = 1;}
								if(img_dilate_(i+1,j-1) == 0){img_dilate_(i,j-1) = marker;}
								if(img_dilate_(i+1,j) == 0){img_dilate_(i,j) = marker;}
								if(img_dilate_(i+1,j+1) == 0){img_dilate_(i,j+1) = marker;}
							}
						}
				}
			}
			marker++;
		}
		
			for(i = 0; i < (img_dilate_.rows); i++)
			{
				for(j = 1; j < (img_dilate_.cols); j++)
				{	//If the value is anything but black or white, turn it white
					if(img_dilate_(i,j) != 0 && img_dilate_(i,j) != 255){img_dilate_(i,j) = 255;}
				}
			}	
		
			*img_dst_ = img_dilate_;
			img_dilate_.release();
		
			return;
	}
	
	void erode_image(const cv::Mat* img_src_, cv::Mat* img_dst_, int passes)
	{
		int i, j, k;
		
		//if passes is zereo return
		if(passes == 0){return;}
		//convert to cv::Mat_ for ease of pixel access
		cv::Mat_<uchar> img_erode_;// =  cv::Mat::zeros(img_src_->rows, img_src_->cols, CV_8U);
		//img_dilate_ = img_src_;
		img_erode_ = img_src_->clone ();
		
		for(k = 0; k < passes; k++)
		{
			//Find a seed pixel
			//Note bounds restriced 
			for(i = 1; i < (img_erode_.rows - 1); i++)
			{
					for(j = 1; j < (img_erode_.cols - 1); j++)
					{
						if(img_erode_(i,j) == 0)
						{
							//check 8 neighbouring pixels, mark any white ones
							if(img_erode_(i-1,j-1) == 255){img_erode_(i-1,j-1) = 1;}
							if(img_erode_(i-1,j) == 255){img_erode_(i-1,j) = 1;}
							if(img_erode_(i-1,j+1) == 255){img_erode_(i-1,j+1) = 1;}
							if(img_erode_(i,j-1) == 255){img_erode_(i,j-1) = 1;}
							if(img_erode_(i,j+1) == 255){img_erode_(i,j+1) = 1;}
							if(img_erode_(i+1,j-1) == 255){img_erode_(i,j-1) = 1;}
							if(img_erode_(i+1,j) == 255){img_erode_(i,j) = 1;}
							if(img_erode_(i+1,j+1) == 255){img_erode_(i,j+1) = 1;}
						}
					}
			}
				for(i = 0; i < (img_erode_.rows); i++)
				{
					for(j = 1; j < (img_erode_.cols); j++)
					{
						if(img_erode_(i,j) == 1){img_erode_(i,j) = 0;}
					}
				}
		}	
			*img_dst_ = img_erode_;
			img_erode_.release();
		
			return;
	}
	
	
	
	// Beware: This function has a small bug for some images. Will fix soon.
	int findRegions(cv::Mat *img_input, int *regions, int *region_lengths, int colour)
	{
		int region_count = 0, pixels_passed = 0, row, col, surrounding_region, joined_region, count;
		for (row = 0; row < img_input->rows; row++){
			for (col = 0; col < img_input->cols; col++){
				if (img_input->at<uchar>(row, col) == colour){
					surrounding_region = 0;
					if (row > 0){
						surrounding_region = regions[pixels_passed - img_input->cols + col];
						if (surrounding_region == 0 && col > 0){
							surrounding_region = regions[pixels_passed - img_input->cols + col - 1];
							if (surrounding_region == 0){
								surrounding_region = regions[pixels_passed + col - 1];
							}
						}
					}
					if (surrounding_region == 0){
						if (row > 0 && col + 1 < img_input->cols){
							surrounding_region = regions[pixels_passed - img_input->cols + col + 1];
						}
						if (surrounding_region == 0){
							region_count += 1;
							regions[pixels_passed + col] = region_count;
							region_lengths[region_count - 1] = 1;
						} else{
							regions[pixels_passed + col] = surrounding_region;
							region_lengths[surrounding_region - 1] += 1;
						}
					} else{
						regions[pixels_passed + col] = surrounding_region;
						region_lengths[surrounding_region - 1] += 1;
						if (col + 1 < img_input->cols){
							if (regions[pixels_passed - img_input->cols + col + 1] != 0 &&
							regions[pixels_passed - img_input->cols + col + 1] != surrounding_region){
								joined_region = regions[pixels_passed - img_input->cols + col + 1];
								for (count = 0; count < pixels_passed + col; count++){
									if (regions[count] == joined_region){
										regions[count] = surrounding_region;
										region_lengths[joined_region - 1] -= 1;
										region_lengths[surrounding_region - 1] += 1;
									}
								}
							}
						}
					}
				} else{
					regions[pixels_passed + col] = 0;
				}
			}
			pixels_passed += img_input->cols;
		}
		return region_count;
	}
	
	
	
	
	void imageCallback(const sensor_msgs::ImageConstPtr & msg_ptr)
	{
		
		int i, j;
		cv::Mat img_v_;

		// Convert ROS Imput Image Message to IplImage
		try
		{
			cv_input_ = bridge_.imgMsgToCv (msg_ptr, "bgr8");
 		}
		catch (sensor_msgs::CvBridgeException error)
		{
			ROS_ERROR ("CvBridge Input Error");
 		}
 		
		// Convert IplImage to cv::Mat
		img_in_ = cv::Mat (cv_input_).clone ();
		//variables for histogram
	/*	int numBins = 256;
		float range[] = {0, 255};
		float *ranges[] = { range };
		//create the histogram
		CvHistogram *hist = cvCreateHist(1, &numBins, CV_HIST_ARRAY, ranges, 1);
		cvClearHist(hist);
		//initilise the images to store R G B channels
	    IplImage* imgRed = cvCreateImage(cvGetSize(cv_input_), 8, 1);
		IplImage* imgGreen = cvCreateImage(cvGetSize(cv_input_), 8, 1);
		IplImage* imgBlue = cvCreateImage(cvGetSize(cv_input_), 8, 1);
		//split the input image 
		cvSplit(cv_input_, imgBlue, imgGreen, imgRed, NULL);
		//calculate the histogram, draw it out and store it in imgHistRed
		cvCalcHist(&imgRed, hist, 0, 0);
		IplImage* imgHistRed = DrawHistogram(hist);
		cvClearHist(hist);
        
        cvCalcHist(&imgGreen, hist, 0, 0);
		IplImage* imgHistGreen = DrawHistogram(hist);
		cvClearHist(hist);
        
        cvCalcHist(&imgBlue, hist, 0, 0);
		IplImage* imgHistBlue = DrawHistogram(hist);
		cvClearHist(hist);
	*/
	
		//filter input image

	/*	
		cv::vector<cv::Mat> planes;
		//Apply histogram equlisation to input image 
		//1: convert input to HSV
		//2: separate V
		//3: equlise V 
		// 4: merge and convert back to BGR
		//This will have the effect of contrast stretching
		
		//initilise and image to store v in 
		img_v_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		//convert to HSV
		cv::cvtColor (img_in_, img_hsv_, CV_BGR2HSV);
		//pull out V store in planes[2]
		cv::split(img_hsv_, planes);
		//equlise V
		cv::equalizeHist(planes[2], img_tmp_);
		planes[2] = img_tmp_;
		//recreate img_hsv_ with new values
		cv::merge(planes, img_hsv_);

        //convert back to a BGR image
		cv::cvtColor (img_hsv_, img_in_proc_, CV_HSV2BGR);
	*/	
	
		// output = input
		img_out_ = img_in_.clone ();
		// Convert Input image from BGR to HSV
		cv::cvtColor (img_in_, img_hsv_, CV_BGR2HSV);
		// Zero Matrices
		img_hue_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		img_sat_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		img_bin_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8U);
		// HSV Channel 0 -> img_hue_ & HSV Channel 1 -> img_sat_
		int from_to[] = { 0,0, 1,1};
		cv::Mat img_split[] = { img_hue_, img_sat_};
		cv::mixChannels(&img_hsv_, 3,img_split,2,from_to,2);

		for(i = 0; i < img_out_.rows; i++)
		{
			for(j = 0; j < img_out_.cols; j++)
			{
  				// The output pixel is white if the input pixel
				// hue is yellow and saturation is reasonable
												        	//original values | best found
				if(img_hue_.at<uchar>(i,j) > min_hue &&     //4               | 4
				img_hue_.at<uchar>(i,j) < max_hue &&        //30              | 34
				img_sat_.at<uchar>(i,j) > min_sat){         //126             | 50
					img_bin_.at<uchar>(i,j) = 255;
				} else {
					img_bin_.at<uchar>(i,j) = 0;
					// Clear pixel blue output channel
					img_out_.at<uchar>(i,j*3+0) = 0;
					// Clear pixel green output channel
					img_out_.at<uchar>(i,j*3+1) = 0;
					// Clear pixel red output channel
					img_out_.at<uchar>(i,j*3+2) = 0;
				}
			}
		}

		/*after thresholding the image can still be very 'messy' 
		* with large amounts of noise, this can cause the cetre to be 
		* found incorrectly and cause trouble when we try to fing
		* the pipes angle and estimated size/distance, so it needs to 
		* be cleaned up 
		*/
		//mean filter the binary image
		
		//close the image using erosion and dilation
		img_out_ = img_bin_;
		cv::medianBlur(img_bin_, img_bin_, 9);
		erode_image(&img_bin_, &img_bin_, erode_passes);
		dilate_image(&img_bin_, &img_bin_, dilate_passes);
		//findCentre();





		// Set The Pixel Count
		pixelcount_ = img_bin_.cols * img_bin_.rows;
		
		// Allocate The Correct Amount Of Memory If Required
		if (regions_allocated_ != pixelcount_){
			regions_ = (int*)(malloc(pixelcount_ * sizeof(int)));
			region_lengths_ = (int*)(malloc(pixelcount_ * sizeof(int)));
			largest_regions_ = (int*)(malloc(pixelcount_ * sizeof(int)));
			regions_allocated_ = pixelcount_;
		}

		// Get White Regions
		region_count_ = findRegions(&img_bin_, regions_, region_lengths_, 255);

		// Get Largest White Regions
		img_regions_ = cv::Mat::zeros(img_bin_.rows, img_bin_.cols, CV_8U);
		if (region_count_ > 0){
			k_ = 0;
			m_ = 0;
			for (i_ = 1; i_ <= region_count_; i_++){
				if (region_lengths_[i_ - 1] > k_){
					k_ = region_lengths_[i_ - 1];
					m_ = i_;
				}
			}
			largest_regions_[0] = m_;
			largest_regions_count_ = 1;
			for (i_ = 1; i_ <= region_count_; i_++){
				if (region_lengths_[i_ - 1] >= (k_ * 0.05)){
					largest_regions_[largest_regions_count_] = i_;
					largest_regions_count_ += 1;
				}
			}
			if (m_ > 0){
				pixels_passed_ = 0;
				for (i_ = 0; i_ < img_regions_.rows; i_++){
					for (j_ = 0; j_ < img_regions_.cols; j_++){
						for (k_ = 0; k_ < largest_regions_count_; k_++){
							if (regions_[pixels_passed_ + j_] == largest_regions_[k_]){
								img_regions_.at<uchar>(i_, j_) = 255;
							}
						}
					}
					pixels_passed_ += img_bin_.cols;
				}
			}
		}
		
		// Fill In Enclosed Areas On Largest White Regions
/*		region_count_ = findRegions(&img_regions_, regions_, region_lengths_, 0);
		largest_regions_count_ = 0;
		for (j_ = 0; j_ < 4; j_++){
			if (j_ == 0){
				pixels_passed_ = 0;
				l_ = img_regions_.rows;
			} else if (j_ == 1){
				pixels_passed_ = img_regions_.cols - 1;
				l_ = img_regions_.rows;
			} else if (j_ == 2){
				pixels_passed_ = 1;
				l_ = img_regions_.cols - 1;
			} else if (j_ == 3){
				pixels_passed_ = ((img_regions_.rows - 1) * img_regions_.cols) + 1;
				l_ = img_regions_.cols - 1;
			}
			for (i_ = 0; i_ < l_; i_++){
				m_ = 0;
				for (k_ = 0; k_ < largest_regions_count_; k_++){
					if (regions_[pixels_passed_] == largest_regions_[k_]){
						m_ = 1;
						break;
					}
				}
				if (m_ == 0){
					largest_regions_[largest_regions_count_] = regions_[pixels_passed_];
					largest_regions_count_ += 1;
				}
				if (j_ < 2){
					pixels_passed_ += img_regions_.cols;
				} else {
					pixels_passed_ += 1;
				}
			}
		}
		pixels_passed_ = 0;
		for (i_ = 0; i_ < img_regions_.rows; i_++){
			for (j_ = 0; j_ < img_regions_.cols; j_++){
				if (regions_[pixels_passed_ + j_] != 0){
					m_ = 0;
					for (k_ = 0; k_ < largest_regions_count_; k_++){
						if (regions_[pixels_passed_ + j_] == largest_regions_[k_]){
							m_ = 1;
							break;
						}
					}
					if (m_ == 0){
						img_regions_.at<uchar>(i_, j_) = 255;
					}
				}
			}
			pixels_passed_ += img_regions_.cols;
		}
	*/	
		// Check If The Total Area Is Large Enough To Be Considered A Pipe
		is_pipe_ = 0;
		for (i_ = 0; i_ < img_regions_.rows; i_++){
			for (j_ = 0; j_ < img_regions_.cols; j_++){
				if (img_regions_.at<uchar>(i_, j_) == 255){
					is_pipe_ += 1;
					if (is_pipe_ >= 5000){
						break;
					}
				}
			}
			if (is_pipe_ >= 5000){
				break;
			}
			pixels_passed_ += img_regions_.cols;
		}
		if (is_pipe_ >= 5000){
			is_pipe_ = 1;
		} else{
			is_pipe_ = 0;
		}
		
		// Remove "Cut-Out" Areas

		// Determine The Angle
		if (is_pipe_ == 1){
			n_ = 0;
			sx_ = 0.0;
			sy_ = 0.0;
			sxx_ = 0.0;
			sxy_ = 0.0;
			n2_ = 0;
			sx2_ = 0.0;
			sy2_ = 0.0;
			sxx2_ = 0.0;
			sxy2_ = 0.0;
			for (i_ = 0; i_ < img_regions_.rows; i_++){
				for (j_ = 0; j_ < img_regions_.cols; j_++){
					if (img_regions_.at<uchar>(i_, j_) == 255){
						n_ += 1;
						sx_ += (double)(j_);
						sy_ += (double)(i_);
						sxx_ += (double)(pow(j_, 2.0));
						sxy_ += (double)(i_ * j_);
						break;
					}
				}
				for (j_ = img_regions_.cols - 1; j_ >= 0; j_--){
					if (img_regions_.at<uchar>(i_, j_) == 255){
						n2_ += 1;
						sx2_ += (double)(j_);
						sy2_ += (double)(i_);
						sxx2_ += (double)(pow(j_, 2.0));
						sxy2_ += (double)(i_ * j_);
						break;
					}
				}
			}
			beta_ = (double)(((double)(n_ * sxy_) - (double)(sx_ * sy_)) / ((double)(n_ * sxx_) - (double)(pow(sx_, 2))));
			alpha_ = (double)((double)(sy_ / (double)(n_)) - (double)(beta_ * sx_ / (double)(n_)));
			beta2_ = (double)(((double)(n2_ * sxy2_) - (double)(sx2_ * sy2_)) / ((double)(n2_ * sxx2_) - (double)(pow(sx2_, 2))));
			alpha2_ = (double)((double)(sy2_ / (double)(n2_)) - (double)(beta2_ * sx2_ / (double)(n2_)));
			point1_ = {(-alpha_ / beta_), 0};
			point2_ = {((double)(img_regions_.rows - 1.0 - alpha_) / beta_), img_regions_.rows - 1};
			line(img_regions_, point1_, point2_, cvScalar(128, 128, 128, 0), 1, 8, 0);
			line(img_in_, point1_, point2_, cvScalar(0, 0, 255, 0), 1, 8, 0);
			point1_ = {(-alpha2_ / beta2_), 0};
			point2_ = {((double)(img_regions_.rows - 1.0 - alpha2_) / beta2_), img_regions_.rows - 1};
			line(img_regions_, point1_, point2_, cvScalar(128, 128, 128, 0), 1, 8, 0);
			line(img_in_, point1_, point2_, cvScalar(0, 0, 255, 0), 1, 8, 0);
		}







		// Display Input image
		cv::imshow ("input", img_in_);
		//Display preprocessed input image
	//	cv::imshow ("preprocessed image", img_in_proc_);
		// Display Binary Image
		cv::imshow ("binary image", img_bin_);
		// Display segmented image
		//cv::imshow ("segmented output", img_out_);
		// Display The Largest Regions
		cv::imshow ("largest regions", img_regions_);
		// Needed to  keep the HighGUI window open
		cv::waitKey (3);

	}


	//erode function
	
};



int main(int argc, char **argv)
{
 	// Initialize ROS Node
 	ros::init(argc, argv, "ihr_demo1");
	// Start node and create a Node Handle
	ros::NodeHandle nh;
 	// Instaniate Demo Object
	ROS_INFO("Online");
	Demo d(nh);
	
	//Read threshold values from file
	INIReader reader("config.ini");
	//check the file can be opened
    if (reader.ParseError() < 0) 
    {
        std::cout << "Can't load 'config.ini'\n";
        return 1;
    }
    
	//read the values in, defaults to best values found for the test video
	min_hue = reader.GetInteger("hue", "min_hue", 4);
	std::cout << "min_hue = " << min_hue << "\n";
	max_hue = reader.GetInteger("hue", "max_hue", 34);
	std::cout << "min_hue = " << max_hue << "\n";
	min_sat = reader.GetInteger("saturation", "min_sat", 50);
	std::cout << "min_hue = " << min_sat << "\n";
	erode_passes = reader.GetInteger("erode", "erode_passes", 5);
	std::cout << "erode_passes = " << erode_passes << "\n";
	dilate_passes = reader.GetInteger("dilate", "dilate_passes", 5);
	std::cout << "dilation_passes = " << dilate_passes << "\n";
  	// Spin ...



	/* Leave One Section Uncommented And The Others Commented Out */
	/* Replace Video/Image Filename If Necessary (Files Should Be Stored In The "dwncam2" Folder) */


	/****************** FOR CAMERA INPUT MODE ******************/
  	//ros::spin();
	/***********************************************************/


	/****************** FOR VIDEO INPUT MODE ******************/
	cv::Mat video_frame;
	IplImage video_frame2;
	sensor_msgs::CvBridge bridge2_;
	cv::VideoCapture cap("Test.avi");
    	if(!cap.isOpened()){
        	return -1;
	}
	while(ros::ok()){
		ros::spinOnce();
		cap >> video_frame;
		video_frame2 = video_frame;
		d.imageCallback(bridge2_.cvToImgMsg(&video_frame2, "passthrough"));
	}
	/**********************************************************/


	/****************** FOR IMAGE INPUT MODE ******************/
	/*cv::Mat video_frame;
	IplImage video_frame2;
	sensor_msgs::CvBridge bridge2_;
	while(ros::ok()){
		ros::spinOnce();
		video_frame = cvLoadImage("a.jpg", CV_LOAD_IMAGE_COLOR);
		video_frame2 = video_frame;
		d.imageCallback(bridge2_.cvToImgMsg(&video_frame2, "passthrough"));
	}*/
	/**********************************************************/



	// ... until done
	return 0;
}
