#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp> 
#include <iostream>

using namespace cv;
using namespace std;

static float a1 = 1.4;
static float a2 = 1.5;

void init(VideoCapture kamerka, Size *size);
Mat greenScreen(VideoCapture kamerka, Size size, Mat img);
void greenScreen2(VideoCapture kamerka, Size size, Mat img);
int main()
{
	VideoCapture kamerka = VideoCapture(0);
	if (kamerka.isOpened()) {
		Size size;
		init(kamerka,&size);
		Mat img = imread("obraz.jpg", IMREAD_COLOR);
		if (img.empty()) {
			cout << "Nie można otworzyć podanego obrazlu tła!" << endl;
			return -1;
		}
		img.convertTo(img, CV_32FC3, 1.0 / 255);
		while (1) {
			Mat frame;
			//frame = greenScreen(kamerka, size, img);
			//imshow("Frame", frame);

			greenScreen2(kamerka, size, img);
			char c = (char)waitKey(25);
			if (c == 27)
				break;
		}
		kamerka.release();
		destroyAllWindows();
	}
	else {
		cout << "Nie udało się połączyć z kamerką !" << endl;
	}
	return 0;
}

void init(VideoCapture kamerka, Size *size) {
	int width = (int)kamerka.get(CAP_PROP_FRAME_WIDTH);
	int height = (int)kamerka.get(CAP_PROP_FRAME_HEIGHT);
	*size = Size(width, height);
}

Mat greenScreen(VideoCapture kamerka, Size size, Mat img) {
	Mat frame;
	kamerka >> frame;
	vector<Mat> channels, img_channels;;
	frame.convertTo(frame, CV_32F, 1.0 / 255);
	split(frame, channels);
	split(img, img_channels);

	Mat alpha = Mat::zeros(size, CV_32F);
	alpha = Scalar::all(1.0) - a1 * (channels[1] - a2 * channels[0]);
	threshold(alpha, alpha, 1, 1, THRESH_TRUNC);
	threshold(-1 * alpha, alpha, 0, 0, THRESH_TRUNC);
	alpha = -1 * alpha;

	for (int i = 0; i < 3; ++i) {
		multiply(alpha, channels[i], channels[i]);
		multiply(Scalar::all(1.0) - alpha, img_channels[i], img_channels[i]);
	}

	
	Mat out,front,back;
	merge(channels, front);
	merge(img_channels, back);
	out = front + back;
	out.convertTo(out, CV_8UC3, 255);
	return out;
}

void greenScreen2(VideoCapture kamerka, Size size, Mat img) {
	Mat u_green(size, CV_8UC3, Scalar(200, 223, 224));
	Mat l_green(size, CV_8UC3, Scalar(120, 120, 120));
	Mat mask, res, out, frame;
	kamerka >> frame;
	inRange(frame, l_green, u_green, mask);
	bitwise_and(frame, frame, res, mask);

	out = frame - res;

	imshow("chuj", out);
}