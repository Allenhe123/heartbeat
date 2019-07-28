//------------------------------------------------------------------------------


#ifndef _DETECT_PULSE
#define _DETECT_PULSE

#define _USE_MATH_DEFINES

#include <math.h>
#include <opencv2\opencv.hpp>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <cmath>

#include "BandProcess.h"
#include "BufferFFT.h"
#include  "Cardiac.h"
#include  "PhaseController.h"
#include "GraphUtils.h"

using namespace cv;


class DetectPulse
{
public:

	std::vector<float>* filtered;
	std::vector<float>* samples;
	std::vector<float>* mag;

	DetectPulse();

	void Execute(Mat &Image, Rect &face, bool faceFound, double fps);

protected:

	void DrawForehead(Mat& frame, Mat &forehead_img, Rect &forehead);
	float GetSkinPixelsMean(Mat &face, float upperBound, float lowerBound);
	void ClampRect(Rect &face, Rect &range);

	Cardiac cardiac_;
	PhaseController phase_controller;
	BufferFFT buffer_fft;

	std::vector<float>freqs;
	std::vector<float>fft;
	float forehead_mean;

	float eyeW;
	float eyeH;
	float eyeMW;
	float eyeMH;
	bool running;
	bool help;
public:
	void PauseStart();
	void IncrEye();
	void DecrEye();
	void LeftMove();
	void UpMove();
	void RightMove();
	void DownMove();
	void Help();
	void Reset();
};


#endif