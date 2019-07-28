

#include "stdafx.h"
#include "DetectPulse.h"
#include <ostream>


DetectPulse::DetectPulse()
{ 
	buffer_fft = BufferFFT(400, 13.0);
	phase_controller = PhaseController(1.0, 0.0, true);

	cardiac_ = Cardiac();
	forehead_mean = 0;
	running = true;
	eyeW = 60;
	eyeH = 30;
	eyeMW = 0;
	eyeMH = 0;
	help = false;
}


//Draw the face rectangle and the text
void DetectPulse::DrawForehead(Mat& frame, Mat &forehead_img, Rect &forehead)
{
	Mat roi = frame(forehead);
	std::vector<Mat> combined;
    combined.push_back(Mat(forehead_img.rows, forehead_img.cols, forehead_img.type(), Scalar(0)));
    combined.push_back(forehead_img);
	combined.push_back(Mat(forehead_img.rows, forehead_img.cols, forehead_img.type(), Scalar(0)));
	merge(combined, roi);
}

float DetectPulse::GetSkinPixelsMean(Mat &face, float upperBound, float lowerBound)
{
	//std::wostringstream wos;
	//wos << L"size:" << face.size.p[0] << " p1: " << face.size.p[1]
	//	<< " vec4b channels: " << Vec4b::channels 
	//	<< " mat channels: " << face.channels()
	//	<< std::endl;
	//::OutputDebugString(wos.str().c_str());

	float cumulative = 0;
	int count = 0;
	
	for(int x= 0; x<face.rows; x++)
	{	
		for(int y=0; y<face.cols ; y++)
		{	
			float val = face.at<Vec3b>(x,y)[1]; 
			if(val > lowerBound && val < upperBound)
			{
				cumulative += val;
				count++;
				face.at<Vec3b>(x,y)[0] = 0;
				face.at<Vec3b>(x,y)[2] = 0;
			}			
		}
	}
	return cumulative/count;
}

void DetectPulse::ClampRect(Rect &face, Rect &range)
{
	if (face.x < 0) face.x = 0;
	if (face.y < 0) face.y = 0;
	if (face.width < 0) face.width = 1;
	if (face.height < 0) face.height = 1;
	if (face.x + face.width>range.width) face.x = range.width - face.width;
	if (face.y + face.height>range.height) face.y = range.height - face.height;

}

void DetectPulse::Execute(Mat &image, Rect &face, bool faceFound, double fps)
{
	//Get forehead
	float x = face.x + face.width * 0.5;
    float y = face.y + face.height * 0.2;
    
    x -= face.width / 5.0;
    y -= face.height / 5.0;

	float iw = image.cols;
	float ih = image.rows;
	if (iw == 0 || ih == 0) return;

	Rect imRct(0, 0, iw, ih);
	float w = eyeW;              // forehead width
	float h = eyeH;              // forehead height
	x = iw / 2 - w/2 + eyeMW;    // forehead center x
	y = ih / 2 - h/2 + eyeMH;    // forehead center y

	Rect forehead(x, y, w, h);

	ClampRect(forehead, imRct);

	float zoomx = 3.0f;
	float zoomy = 8.5f;

	face.width = static_cast<int>(w * zoomx);    // should adjust by distacne between camera  and face
	face.height = static_cast<int>(h * zoomy);   // should adjust by distacne between camera  and face
	face.x = x + w/2 - face.width/2;
	face.y = y + h/2 - face.height/4;
	//face.x = x + face.width / 2;
	//face.y = y - face.height / 4;

	ClampRect(face, imRct);

	Mat forehead_image = image(forehead);

	std::vector<Mat> channels(4);
	// split img
	split(forehead_image, channels);
	// get the green channel
	Mat forehead_green = channels[1];
	
	if(faceFound)
	{	
		cv::Scalar tempVal = mean( forehead_green );
		forehead_mean = tempVal.val[0];

		//std::wostringstream wos;
		//wos << L"forceMean:" << forehead_mean << std::endl;
		//::OutputDebugString(wos.str().c_str());
	}

	int pixelRange = 10;

	float skinMean = GetSkinPixelsMean(image(face), forehead_mean + pixelRange, forehead_mean - pixelRange);

	char text[50];
	char helpstr[100];
	sprintf(helpstr, "Move: s<,d>,e^,x.   Zoom: + or -, Enter: pause or start, R:reset ! ", 1);
	if (running)
	{
		buffer_fft.Execute(skinMean);

		//Measure heart
		cardiac_.Execute(buffer_fft.freqs, buffer_fft.fft);

		sprintf(text, "%0.0f bpm", cardiac_.bpm);

		if (buffer_fft.ready)
		{
			phase_controller.On();
		}

		filtered = &(cardiac_.filtered);
		samples = &(buffer_fft.interpolated);
		mag = &(cardiac_.magnitude);
	}
	else
	{
		sprintf(text, "Not Working",1);
		filtered->clear();
		filtered->push_back(0);
		samples->clear();
		samples->push_back(0);
		mag->clear();
		mag->push_back(0);
	}
	if (help)
	{
		putText(image, helpstr, Point(21, 41), FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0));
	}

	putText(image,text, Point(face.x,face.y - 20), FONT_HERSHEY_PLAIN,2,Scalar(0,0,255));

	sprintf(text, "%4.2f(%d)", fps, buffer_fft.samples.size());
	putText(image, text, Point(face.x + face.width - 10, face.y - 20), FONT_HERSHEY_PLAIN, 2, Scalar(0, 0, 255));
	
	equalizeHist( forehead_green, forehead_green );

	DrawForehead(image, forehead_green, forehead);

	cv::Point pt1(face.x, face.y);
	cv::Point pt2(face.x + face.width, face.y + face.height);
	cv::rectangle(image, pt1, pt2, cv::Scalar(0, 0, 255), 1, 8, 0);
}

void DetectPulse::PauseStart()
{
	if (running)
	{
		buffer_fft.Reset();
	}

	running = !running;
}

void DetectPulse::IncrEye()
{
	if (eyeW <= 160 && eyeH <= 80)
	{
		eyeW *= 1.25f;
		eyeH *= 1.25f;
	}
}

void DetectPulse::DecrEye()
{
	if (eyeW >= 40 && eyeH >= 20)
	{
		eyeW *= 0.25f;
		eyeH *= 0.25f;
	}
}

void DetectPulse::LeftMove()
{
	eyeMW -= 10;
}
void DetectPulse::UpMove()
{
	eyeMH -= 10;
}
void DetectPulse::RightMove()
{
	eyeMW += 10;
}
void DetectPulse::DownMove()
{
	eyeMH += 10;
}

void DetectPulse::Help()
{
	help = !help;
}

void DetectPulse::Reset()
{
	running = true;
	eyeW = 60;
	eyeH = 30;
	eyeMW = 0;
	eyeMH = 0;
	help = false;
}

