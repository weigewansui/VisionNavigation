#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/videoio.hpp"
#include "opencv2/video/tracking.hpp"
#include <vector>

#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
// #include "opencv2/nonfree/features2d.hpp"

#include <stdio.h>
#include <stdlib.h> 
#include <iostream>
#include <iomanip> 
#include <vector>



#define PI 3.1415926

using namespace std;
using namespace cv;
const int MAX_FEATURES = 500;
vector<Point2f> pointsToTrack;
vector<Vec2f> linesToTrack;
double thres = 200;
float alpha = 0.04;
int blockSize = 3;

void drawLine(Mat& imgOutput, vector<Vec2f> lines) {

  for (size_t i = 0; i <lines.size(); i++) {
    float theta = lines[i][1];
    float rho = lines[i][0];
    double a = cos(theta), b =sin(theta);
    double x0 = a*rho, y0 = b*rho;
    Point pt1(cvRound(x0 + 1000*(-b)), cvRound(y0 + 1000*(a)));

    Point pt2(cvRound(x0 - 1000*(-b)), cvRound(y0 - 1000*(a)));
    line(imgOutput, pt1, pt2, Scalar(0,0,255), 1, 8);

  }

}

void drawPoint(Mat& imgOut, vector<Point2f> mypoints) {
int myradius=3;
for (int i=0;i<mypoints.size();i++)
    circle(imgOut,cvPoint(mypoints[i].x,mypoints[i].y),myradius,CV_RGB(100,0,0),-1,8,0);
}

void showIntensity(Mat&img) {

  Scalar intensity = img.at<uchar>(20,20);
  double res = (double) intensity.val[0];
  cout <<res<<endl;
}

vector<float> getPixelMatrix(Mat& img, int x, int y) {
  // critiria number
  // x, y returns the pixel position
  // alpha, coefficient number

  vector<float> ret(2);

  // cout<<__LINE__<<endl;
  float Crit=0.0;
  double surround[4];
  double intensity_val;
  double dx,dy;
  Scalar intensity;
  Scalar SurrIntense[4];

  // for(int i = 1; i < img.rows-1; i++) {
  //   for(int j = 1; j < img.cols-1; j++) {
  //     intensity = img.at<uchar>(i,j);
  //     SurrIntense[0] = img.at<uchar>(i-1,j-1);
  //     SurrIntense[1] = img.at<uchar>(i-1,j+1);
  //     SurrIntense[2] = img.at<uchar>(i+1,j-1);
  //     SurrIntense[3] = img.at<uchar>(i+1,j+1);
  //   }
  // }

  intensity = img.at<uchar>(y,x);
  SurrIntense[0] = img.at<uchar>(y,x-1);
  SurrIntense[1] = img.at<uchar>(y,x+1);
  SurrIntense[2] = img.at<uchar>(y-1,x);
  SurrIntense[3] = img.at<uchar>(y+1,x);
// cout<<__LINE__<<endl;

  for(int k=0; k<4; k++)
    //get the intense value in double type
    surround[k] = (double) SurrIntense[k].val[0];

  dx = (surround[1]-surround[0])/2;
  dy = (surround[3]-surround[2])/2;

  // cout<<__LINE__<<endl;

  // ret.assign(0,dx);
  // ret.assign(1,dy);

  ret.at(0) = dx;
  ret.at(1) = dy;

  // cout<<ret.at(0)<<endl;
  // cout<<__LINE__<<endl;

  // cout<<dx<<" ,"<<dy<<endl;
  return ret;

}

float getCriteria(Mat& frame, int x, int y) {
  // get the criteria number of a block

  //if the block size in near the corner of the image
  //return 0

  if(x < blockSize-1 || y < blockSize-1) return 0.0;

  float Matrix11 = 0;
  float Matrix22 = 0;
  float Matrix12 = 0;

  vector<float>** blockIntense = new vector<float>*[blockSize];
  for(int i = 0; i < blockSize; i++) {
    blockIntense[i] = new vector<float>[blockSize];
  }

  for(int i = 0; i < blockSize; i++) {
    for(int j = 0; j < blockSize; j++) {
      blockIntense[i][j] = getPixelMatrix(frame, x-i, y-j);
      Matrix11 += (blockIntense[i][j].at(0))*(blockIntense[i][j].at(0));
      Matrix22 += (blockIntense[i][j].at(1))*(blockIntense[i][j].at(1));
      Matrix12 += (blockIntense[i][j].at(1))*(blockIntense[i][j].at(0));
    }
  }

  return ((Matrix11*Matrix22-Matrix12*Matrix12)-alpha*(Matrix11+Matrix22));

}

void processFirstFrameCorner(Mat& first_frame) {

	//process the first frame to get the feature points 
	// and feature lines

    float lineDev = 0.05;
	int blockSize = 2;
	int apertureSize = 3;
  	int pointNum;
  	int minPointNum = 2;
	double k = 0.04;

	vector<Vec2f> lines;
	vector<Point2f> featurePoints;
	vector<Point2f> pointsToTrackTmp;
	Mat first_canny;
  //extract points for the first frame
  	goodFeaturesToTrack(first_frame, featurePoints, MAX_FEATURES, 0.1, 0.2 );

  //extract lines for the first frame
  	Canny(first_frame,first_canny, 0.4*thres, thres);
  	HoughLines(first_canny, lines, 1,CV_PI/180, 20);


  // fit featurePoints into the line
  	for (size_t i = 0; i <lines.size(); i++) {
    //search every line, if there is points fitting in the line


	    pointNum = 0;
	    // clear the tmp vector for the next search;
	    pointsToTrackTmp.clear();
	    float theta = lines[i][1];
	    float rho = lines[i][0];


	    for(int j = 0; j < featurePoints.size(); j++) {
	    	if(featurePoints[j].x*cos(theta) + featurePoints[j].y*sin(theta) < (rho+lineDev) &&
	       	featurePoints[j].x*cos(theta) + featurePoints[j].y*sin(theta) > (rho-lineDev)){
	        //if fit the line, cout the pointNum
	        pointNum ++;
	        //store these points that are in a line
	        pointsToTrackTmp.push_back(featurePoints[j]);
	      }
    }

    // if pointNum greater than a certain number, keep the line;
    if(pointNum > minPointNum) {
      linesToTrack.push_back(lines[i]);
      pointsToTrack.insert(pointsToTrack.end(), pointsToTrackTmp.begin(), pointsToTrackTmp.end());
    }

  }
}

void processFirstFrame(Mat &first_frame) {

	vector<Vec2f> lines;
	Mat first_canny;
	 //extract lines for the first frame
  	Canny(first_frame,first_canny, 0.4*thres, thres);
  	HoughLines(first_canny, lines, 1,CV_PI/180, 20);

    for (size_t i = 0; i <lines.size(); i++) {

	    float theta = lines[i][1];
	    float rho = lines[i][0];
	    double a = cos(theta), b =sin(theta);
	    for(int x = 0; x < first_frame.cols(); x++) {
	    	for(int y = 0; y < first_frame.row(); y++) {
	    		if(x*a + y*b < (rho+lineDev) || x*a + y*b > (rho-lineDev)){

	    			if()
	    		}
	    	}
	    }
  	}


}

int main()
{
	 

	VideoCapture capture(0);

    // Check if video camera is opened
    if(!capture.isOpened()) return 1;

    bool finish = false;
    Mat frame;
    Mat OutCountour;
    Mat imgOutput;
    Mat first_frame, first_canny;
    namedWindow("Video Camera");


	vector<Vec2f> lines;
  //line deviation
 	float lineDev = 0.05;

	int blockSize = 2;
	int apertureSize = 3;
	double k = 0.04;

  //deal with the first frame
  	if(!capture.read(first_frame)) return 1;
  	cvtColor(first_frame,first_frame,COLOR_BGR2GRAY);

  	processFirstFrameCorner(first_frame);

    while(!finish){

      //if didn't get feature points, get again
    
        // Read each frame, if possible
        if(!capture.read(frame)) return 1;
        imgOutput = frame;
        // Convernto to gray image
        cvtColor(frame,frame,COLOR_BGR2GRAY);

        // Detect Maximum Movement with Lucas-Kanade Method
        // maxMovementLK(prev_frame, frame);
        Canny(frame,OutCountour, 0.4*thres, thres);
        HoughLines(OutCountour, lines, 1,CV_PI/180, 80);

        // vector<float> grad = getPixelMatrix(frame, 20, 20);
        // cout<<grad.at(0)<<", ";
        // cout<<grad.at(1)<<endl;

        drawLine(imgOutput, linesToTrack);
        drawLine(imgOutput, lines);
        drawPoint(imgOutput, pointsToTrack);
        // drawPoint(imgOutput, featurePoints);

        // imshow("Video Camera", OutCountour);
        imshow("Video Camera", imgOutput);

        // Press Esc to finish
        if(waitKey(1)==27) finish = true;

        // prev_frame = frame;

    }
    // Release the video camera
    capture.release();
    return 0;

	// Houghlines()
}