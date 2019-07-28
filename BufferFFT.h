
#ifndef _BUFFER_FFT
#define _BUFFER_FFT


#define _USE_MATH_DEFINES

#include <math.h>
#include <opencv2\opencv.hpp>
#include "BandProcess.h"
#include <time.h> 

#include "windows.h"

#include <deque>


class BufferFFT
{
public:

	BufferFFT(int n = 322, float spikeLimit = 5.0);

	void GetFFT();
    float FindOffset();
	void Reset();
    void Execute(float data_in);
	
	
	float ready;
	int size;
    int n_;
	float fps;
	Mat fft;
	std::vector<float> freqs;
	std::deque<time_t> times;
	std::deque<float> samples;
	
	std::vector<float> interpolated;
private:
	void Interpolate(const std::vector<double> &newTimes, const std::deque<time_t> &original, const std::deque<float> &samples );
	std::vector<float> HammingWindow(int length);
	std::vector<float> HighPass(std::vector<float> &samples);
	std::vector<float> SubtractMean(std::vector<float> &samples);
	float spike_limit;
	
	std::vector<double> even_times;

	bool offset_strategy_remove_all = true;
	bool remove_all_when_full = false;
};

#endif