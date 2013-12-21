#include <cv.h>
#include <highgui.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include<opencv2/nonfree/nonfree.hpp>
#include<opencv2/nonfree/features2d.hpp>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <boost/program_options.hpp>
#include "3dms-func.h"

// パノラマ画像の大きさ
#define PANO_W 6000
#define PANO_H 3000

// 合成フレームのFPS
#define TARGET_VIDEO_FPS 30

using namespace std;
using namespace cv;
using boost::program_options::options_description;
using boost::program_options::value;
using boost::program_options::variables_map;
using boost::program_options::store;
using boost::program_options::parse_command_line;
using boost::program_options::notify;

int main(int argc, char** argv) {

	Mat source_image, dest_image;
	Mat hsv_source, hsv_dest;
	vector<Mat> source_hist_channels,dest_hist_channels;

	if (argc != 3) {
		cout << "Usage : " << argv[0] << "source_image " << "destination_image " << endl;
		return -1;
	}

	// パノラマ背景画像の読み込み
	cout << "load twe images" << endl;

	source_image = imread(argv[1]);
	dest_image = imread(argv[2]);
	if (source_image.empty()) {
		cerr << "cannot open source image : " << argv[1] << endl;
		return -1;
	}

	if (dest_image.empty()) {
		cerr << "cannot open destination_image : " << argv[2] << endl;
		return -1;
	}

	cout << "done\n" << endl;

	//convert BGR to hsv
//	cvtColor(source_image,hsv_source,CV_BGR2HSV);
	//cvtColor(dest_image,hsv_dest,CV_BGR2HSV);
	//チャネルごとのヒストグラムを計算
	get_color_hist(source_image, source_hist_channels);
	cout << "calc source_image histgram ...done" << endl;

	imshow("source image", source_image);
	waitKey(30);

	get_color_hist(dest_image,dest_hist_channels);
	cout << "calc near_frame histgram ...done" << endl;
	imshow("dest image", dest_image);
	waitKey(30);

	// 各チャネルに
	// 0.8-1.2間で0.1刻みでバイアスを変えながらヒストグラムの類似度を計算
	uchar lut[256];
	float est_gamma[3] = {1.0};
	double min_hist_distance[3] = {DBL_MAX,DBL_MAX,DBL_MAX};
	double  gamma[3] = {0.5,0.5,0.5};
	vector<Mat> source_channels, dest_channels;
	Mat gray_hist,tmp_channel,result;
	split(source_image, source_channels);
	split(dest_image, dest_channels);

	// source image を dest image に近づけるgammaを計算
	int i,j;
	for (i = 0; i < 3; i++) {

		// gammaを0.001刻みで調整
		for (; gamma[i] < 3; gamma[i] += 0.001) {
			// LUTの生成
			for (int j = 0; j < 256; j++)
					lut[j] = ((j*gamma[i] <= 255) ? j*gamma[i]:255);

			//source_image にLUTで補正をかけてdest_imageにヒストグラムが似ているかどうかを計算する
			LUT(source_channels[i], Mat(Size(256, 1), CV_8U, lut), tmp_channel);
			get_gray_hist(tmp_channel,gray_hist);


			double hist_distance = compareHist(gray_hist, dest_hist_channels[i],CV_COMP_CHISQR);

			if(hist_distance < min_hist_distance[i] ){
				est_gamma[i] = gamma[i];
				min_hist_distance[i] = hist_distance;
			}
		}
	}

	cout << " <est_gamma> " << endl;
	cout << est_gamma[0] << " : " << est_gamma[1] << " : " << est_gamma[2]  << endl;
	vector<Mat> convert_channels;

	for(i = 0 ;i < 3;i ++){
		for (j = 0; j < 256; j++)
				lut[j] = ((j*est_gamma[i] <= 255) ? j*est_gamma[i]:255);

		LUT(source_channels[i], Mat(Size(256,1), CV_8U, lut), result);
		source_channels[i] = result.clone();
	}

	Mat tmp_fix,fix_near_image;
	merge(source_channels,fix_near_image);


	imwrite("fix_test.jpg",fix_near_image);



	return 0;
}
