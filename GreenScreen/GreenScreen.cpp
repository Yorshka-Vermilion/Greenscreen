#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main()
{
	VideoCapture kamerka = VideoCapture(0);
	if (kamerka.isOpened()) {
		while (1) {
			Mat frame;
			kamerka >> frame;
			imshow("Frame", frame);

			char c = (char)waitKey(25);
			if (c == 27)
				break;
		}
	}
	else {
		cout << "Nie udało się połączyć z kamerką !" << endl;
	}
	return 0;
}