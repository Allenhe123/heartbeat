#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
#include <thread>
#include <opencv2\opencv.hpp>
#include "DetectPulse.h"
#include "GraphUtils.h"

std::vector<float>* filtered = nullptr;
std::vector<float>* samples = nullptr;
std::vector<float>* mag = nullptr;
DetectPulse pulse_detector;


int detect_bpm()
{
	cv::VideoCapture cap(CV_CAP_DSHOW);
	cap.open(0);
	if (cap.isOpened())
	{
		auto start = std::chrono::system_clock::now();
		int frame_count = 0;
		double fps = 0.0f;

		cv::Mat frame;
		while (true)
		{
			cap >> frame;
			if (frame.empty()) break;

			Rect face;
			pulse_detector.Execute(frame, face, true, fps);

			filtered = pulse_detector.filtered;
			samples = pulse_detector.samples;
			mag = pulse_detector.mag;

			imshow("bpm", frame);

			char k = waitKey(1);

			frame_count++;

			auto end = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			double seconds = (duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
			if (seconds >= 1.0f)
			{
				fps = frame_count * 1.0f / seconds;
				start = end;
				frame_count = 0;
			}

			if (k == 27) //ESC
				exit(0);
			if (k == 13) pulse_detector.PauseStart(); // Enter pause or start

			if (k == '=' || k == '+')pulse_detector.IncrEye();
			if (k == '-' || k == '_')pulse_detector.DecrEye();

			if (k == 'a') pulse_detector.LeftMove();
			if (k == 'w') pulse_detector.UpMove();
			if (k == 'd') pulse_detector.RightMove();
			if (k == 's') pulse_detector.DownMove();
			if (k == 'h') pulse_detector.Help();
			if (k == 'r') pulse_detector.Reset();
		}
	}

	return 0;
}


int run_graphs()
{
	while (true)
	{
		if (filtered != nullptr && !filtered->empty())
			showFloatGraph("Pulse", &(*filtered)[0], filtered->size(), 1,  0, 0, 425);
		if (samples != nullptr && !samples->empty())
			showFloatGraph("Samples", &(*samples)[0], samples->size(), 1,  0, 0, 425);
		if (mag != nullptr && !mag->empty())
			showFloatGraph("FFT Magnitude", &(*mag)[0], mag->size(), 1, 0, 0, 425);
	}
	return 0;
}

void Start()
{
	std::thread t1(detect_bpm);
	std::thread t2(run_graphs);

	t1.join();
	t2.join();
}

int APIENTRY wWinMain(    
	_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
    )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

   Start();
}

