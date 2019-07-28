//------------------------------------------------------------------------------


#ifndef _CARDIAC
#define _CARDIAC

#define _USE_MATH_DEFINES

#include <math.h>
#include <opencv2\opencv.hpp>
#include "BandProcess.h"

using namespace cv;

class Cardiac
{
public:
	Cardiac();
		
	void Execute(std::vector<float> &freqsIn, Mat &fftIn, 
		float lowerLimit = 50.0, float upperLimit = 160, bool makeFiltered = true, 
                 char* operation = "pass");
	int bufferThresh;
	float bpm;
	float phase;
	std::vector<float> filtered;
	std::vector<float> freqs;
    
	std::vector<float> magnitude;

protected:
	BandProcess bandProcess;
	 std::vector<float> pulseBuffer;

};

#endif