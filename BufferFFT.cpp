#include "stdafx.h"
#include "BufferFFT.h"

BufferFFT::BufferFFT(int n, float spikeLimit)
{
	n_ = n;
	fps = 1.0;
	spike_limit = spikeLimit;
	
	ready = false;
}

//interpolate between vector points 
void BufferFFT::Interpolate(const std::vector<double> &newTimes, const std::deque<time_t> &original, const std::deque<float> &samples )
{
	interpolated.clear();
	interpolated.push_back(samples[0]);

	int last_pos = 0;
	for (int i=1; i < newTimes.size(); i++)
	{
		double current = newTimes[i];
		for(int j = last_pos; j < original.size()-1; j++)
		{
			if( current >= (double)original[j] && current <= (double)original[j+1])
			{
				last_pos = j;

				if (std::abs(current - (double)original[j+1]) < 0.0001f)
				{
					interpolated.push_back(samples[j+1]);
				}
				else if (std::abs(current - (double)original[j]) < 0.0001f)
				{
					interpolated.push_back(samples[j]);
				}
				else
				{
					current = current - original[j];
					double m = ((double)samples[j + 1] - (double)samples[j]) / ((double)original[j + 1] - (double)original[j]);
					double newVal = (m * current) + samples[j];
					interpolated.push_back(newVal);
				}

				break;
			}
		}
	}
}

std::vector<float> BufferFFT::SubtractMean(std::vector<float> &samples)
{
	cv::Scalar tempVal = mean( samples );
	float meanVal = tempVal.val[0];

	float sk0 = 0;
	for (auto f : samples)
	{
		sk0 += (f - meanVal) * (f - meanVal);
	}
	float sk = std::sqrt(sk0 / (samples.size() - 1));

	for(int j = 0; j < samples.size(); j++)
	{
		samples[j] = (samples[j] - meanVal) / sk;
	}
	return samples;
}


std::vector<float> BufferFFT::HammingWindow(int length)
{
	std::vector<float> hamming;
	double omega = 2.0 * M_PI / (length);

    for (int i = 0; i < length; i++)
	{
		hamming.push_back(0.54 - 0.46 * cos(omega * (double)(i))); 
	}
	return hamming;
}

//laplace
std::vector<float> BufferFFT::HighPass(std::vector<float> &samples)
{
	std::vector<float> temp;
	temp.push_back(0.0);

	for(int j = 1; j < samples.size() -1; j++)
	{
		float val = (samples[j+1] + (samples[j] * -2)  +  samples[j+1]);
		temp.push_back(val);
	}
	temp.push_back(0.0);

	return temp;
}


void BufferFFT::GetFFT()
{	
	int N = (int)times.size();
	fps = N / ((difftime(times[times.size() -1], times[0]) / 1000.0f));
		
	even_times.clear();
	double spacing = (difftime(times[times.size() -1], times[0]) / (N - 1));

	for(int i=0; i < N; i++)
	{	
		even_times.push_back(times[0] + i * spacing);
	}

	// linear interpolate
	Interpolate(even_times, times, samples);	

	// (val - mean) / sk
	interpolated= SubtractMean(interpolated);

	//get kernal size depending on input frequency
	int kernal = (int)((fps * 60) / 160);
	if(kernal % 2 == 0) kernal++;

	//low pass to remove high freq
	GaussianBlur(interpolated, interpolated, Size(kernal,1), 3, 1);

	// high pass to remove low freq
	interpolated = HighPass(interpolated);
	
	// Create a vector containing a Hamming Window which filters the signal
	std::vector<float> hamming = HammingWindow(interpolated.size());
	
	for(int i = 0; i < interpolated.size(); i++)
	{
		interpolated[i] = interpolated[i] * hamming[i];
	}
	
	//zero padding
	for(int i = N; i< n_; i++)
	{
		interpolated.push_back(0);
	}

	//get fft
	dft(interpolated, fft);	

	freqs.clear();
	
	// do dft for these freqs
	for(int i = 0; i < interpolated.size(); i++)
	{
		freqs.push_back((i * fps / interpolated.size()) / 2); 
	}
}

//find min and max and see if there is a spike in the signal	
float BufferFFT::FindOffset()
{
	if (offset_strategy_remove_all)
		return samples.size() - 1;
	else
	{
		for (int i = 1; i < samples.size() - 1; i++)
		{
			auto start = samples.begin();
			std::advance(start, i);
			std::vector<float> samplesTemp(start, samples.end());

			auto minmax = std::minmax_element(samplesTemp.begin(), samplesTemp.end());
			if ((*(minmax.second) - *(minmax.first)) < spike_limit)
				return i;
		}
	}
}


void BufferFFT::Reset()
{
	int N = FindOffset();
	ready = false;

	std::deque<float> tempSamples;
	std::deque<time_t> tempTimes;
	//keep all data after spike
	for(int i = N; i < samples.size(); i++)
	{
		tempTimes.push_back(times[i]);
		tempSamples.push_back(samples[i]);
	}
	times = tempTimes;
	samples = tempSamples;
}

//Execute the command
void BufferFFT::Execute(float data_in)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	time_t millis = (time.wHour * 3600000) + (time.wMinute * 60000) + (time.wSecond * 1000) + time.wMilliseconds;
	
	samples.push_back(data_in);
	times.push_back(millis);

    if(samples.size() > n_)
	{
		ready = true;

		if (remove_all_when_full)
		{
			float data = samples[samples.size() - 1];
			time_t time_val = times[times.size() - 1];
			samples.clear();
			times.clear();
			samples.push_back(data);
			times.push_back(time_val);
		}
		else
		{
			samples.pop_front();
			times.pop_front();
		}
	}

	if(samples.size() >= 50)
	{
        GetFFT();

        if (spike_limit != 0)
		{
			auto minmax = std::minmax_element(samples.begin(), samples.end());
            if ((*(minmax.second) - *(minmax.first)) > spike_limit)
			{
				Reset();
			}
		}
	}
}
