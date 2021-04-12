#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <unistd.h>

#include "opencv.hpp"

// #define DEBUG

using namespace cv;
/**
 * Mat类的data属性是一个uchar的指针
 * */
void CannyEdge(AVFrame *iAvFrame)
{
	 Mat black(800, 480, CV_8UC4);
	 Mat iRGB(800, 480, CV_8UC4);
	 Mat iRGB1;
	 Mat dst, edge, gray;

	 black = Scalar::all(0);

	 iRGB.data = (unsigned char *)iAvFrame->data[0];
	
	 iRGB.convertTo(iRGB1, CV_8UC1);
	 iRGB1.convertTo(iRGB, CV_8UC4);

	 dst.create(iRGB.size(), iRGB.type());
	 dst = Scalar::all(0);
	
	 cvtColor(iRGB1, gray, COLOR_BGRA2GRAY);
	 blur(gray, edge, Size(3, 3));
	 Canny(edge, edge, 3, 9, 3);
	
}

void CannyEdgeByYUVV(unsigned char *input)
{
	frame_rgb32 *temp = (frame_rgb32 *)malloc(640*480*4);

	/* opencv Mat (cols, rows) */
	Mat yuvImg(480, 640, CV_8UC2);
	Mat srcRgbImg(480, 640, CV_8UC4);
	Mat rgbImg(480, 640, CV_8UC4);

	memcpy(yuvImg.data, input, 640*480*2);
	cvtColor(yuvImg, srcRgbImg, CV_YUV2BGRA_YUYV);
	
	//转成多通道BGR
	unsigned char *ch = srcRgbImg.data;
	
	for (int j = 0; j < 480; j++) {
		for (int i = 0;i < 640; i++) {
			// if (i > 640 && *(ch))
			rgbImg.at<Vec4b>(j,i)[0] = (unsigned char)(*(ch));
			rgbImg.at<Vec4b>(j,i)[1] = (unsigned char)(*(ch + 1));
			rgbImg.at<Vec4b>(j,i)[2] = (unsigned char)(*(ch + 2));
			rgbImg.at<Vec4b>(j,i)[3] = (unsigned char)(*(ch + 3));

			ch += 4;
		}
	}

#ifdef DEBUG
	imwrite("1.jpg", rgbImg);
	printf("save ok!\r\n");
#endif // DEBUG

	Mat dst, edge, gray;

	cvtColor(rgbImg, gray, COLOR_BGRA2GRAY);

	dst.create(rgbImg.size(), rgbImg.type());
	dst = Scalar::all(0);
	
	// cvtColor(yuvImg, gray, CV_YUV2GRAY_YUYV);

	blur(gray, edge, Size(3, 3));
	Canny(edge, edge, 3, 9, 3);

	rgbImg.copyTo(dst, edge);
	// rgbImg.copyTo(dst);

	//Laplacian 
	// GaussianBlur()
	// Laplacian(gray, dst, CV_16S, 3, 1, 0, BORDER_DEFAULT);
	// convertScaleAbs(dst, edge);

	//重新转换成单通道RGBA的格式
	for (int i = 0,j = 0;i < 640*480;i++) {
		j = i/640;
		temp[i].b = (unsigned char)dst.at<Vec4b>(j,i%640)[0];
		temp[i].g = (unsigned char)dst.at<Vec4b>(j,i%640)[1];
		temp[i].r = (unsigned char)dst.at<Vec4b>(j,i%640)[2];
		temp[i].a = (unsigned char)dst.at<Vec4b>(j,i%640)[3];
		// temp[i].a = 0;
	}

	resolutionChange((unsigned char *)temp, 640, 480);
	free(temp);

#ifdef DEBUG
	while(1);
#endif // DEBUG
}
