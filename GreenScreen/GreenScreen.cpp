#include <GL/glew.h>
#include <gl/glut.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp> 
#include <iostream>
#include <fstream>


#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "freeglut.lib")
#pragma comment(lib, "glfw3.lib")

#include "ShaderUtil.h"

using namespace cv;
using namespace std;

typedef struct {
	float r;       // 0 to 1
	float g;       // 0 to 1
	float b;       // 0 to 1
} rgb;

typedef struct {
	float h;       // angle in degrees
	float s;       // 0 to 1
	float v;       // 0 to 1
} hsv;

static const float Stala = 1.0f / 255.0f;

static VideoCapture kamerka, film;
static Size sizee;

// :: USTAWIENIA KOLORÓW ::

Vec3f colorToDelete = Vec3f(182, 203, 203);
float colorThreshold = 0.2; // 0 to 1
Vec3f l_colorToDelete, h_colorToDelete;


Vec2f mousePosition;
int shader = 0;
static int nextImg = 0;
ShaderUtil shaderUtil;
unsigned char* buff = new unsigned char[1228800];

const int texNum = 3;
GLuint textures[texNum];
Mat frame;
Mat* images = new Mat[texNum - 1];

static hsv rgb2hsv(rgb in)
{
	hsv         out;
	double      min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max > in.b ? max : in.b;

	out.v = max;
	delta = max - min;
	if (delta < 0.00001)
	{
		out.s = 0;
		out.h = 0;
		return out;
	}
	if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
		out.s = (delta / max);                  // s
	}
	else {
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		out.s = 0.0;
		out.h = NAN;                            // its now undefined
		return out;
	}
	if (in.r >= max)                           // > is bogus, just keeps compilor happy
		out.h = (in.g - in.b) / delta;        // between yellow & magenta
	else
		if (in.g >= max)
			out.h = 2.0 + (in.b - in.r) / delta;  // between cyan & yellow
		else
			out.h = 4.0 + (in.r - in.g) / delta;  // between magenta & cyan

	out.h *= 60.0;                              // degrees

	if (out.h < 0.0)
		out.h += 360.0;

	return out;
}

static rgb hsv2rgb(hsv in)
{
	double      hh, p, q, t, ff;
	long        i;
	rgb         out;

	if (in.s <= 0.0) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}

void calculateColor() {
	Vec3f rangeVector = Vec3f(colorThreshold, colorThreshold, colorThreshold);
	l_colorToDelete = colorToDelete / 255;
	l_colorToDelete -= rangeVector;
	h_colorToDelete = colorToDelete / 255;
	h_colorToDelete += rangeVector;
}

void GreenScreenOn() {
	shaderUtil.Load("greenscreen.frag");
	shaderUtil.Use();
	glUniform2f(glGetUniformLocation(shaderUtil.mProgramId, "resolution"), sizee.width, sizee.height);
	glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "tex"), 0);
	glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "img"), 1);
	glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
	glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
}

void printColor() {
	rgb tmpRgb;
	tmpRgb.r = colorToDelete.val[0];
	tmpRgb.g = colorToDelete.val[1];
	tmpRgb.b = colorToDelete.val[2];
	hsv tmpHsv = rgb2hsv(tmpRgb);
	cout << "H: " << tmpHsv.h << " S: " << tmpHsv.s << " V: " << tmpHsv.v << " Prog: " << colorThreshold << "\t\r" << flush;
}

void printInfo() {
	system("cls");
	cout << "STEROWANIE:" << endl;
	cout << "LPM - Przycisniecie - Probkowanie koloru" << endl;
	cout << "PPM - Przycisniecie / Puszczenie - Wybor pozycji prostokata obcinajacego obraz w tle" << endl;
	cout << "E - Przycisniecie - Wlaczenie/Wylaczenie Greenscreena" << endl;
	cout << "S/D - Przytrzymanie - Zwiekszenie/Zmniejszenie wartosci saturacji" << endl;
	cout << "V/B - Przytrzymanie - Zwiekszenie/Zmniejszenie wartosci koloru" << endl;
	cout << "+/- - Przytrzymanie - Zwiekszenie/Zmniejszenie progu wybranego koloru" << endl;
	cout << "LEWA/PRAWA STRZALKA - Zmiana obrazu w tle" << endl;
	cout << "Obecnie kluczowany kolor w przestrzeni HSV oraz prog: " << endl;
	printColor();
}

void usrednij(unsigned char* buff) {
	int R = 0, G = 0, B = 0;
	for (int i = 0; i < 27; i += 3) {
		R += static_cast<int>(buff[i]);
		G += static_cast<int>(buff[i + 1]);
		B += static_cast<int>(buff[i + 2]);
	}
	R /= 9;
	G /= 9;
	B /= 9;

	colorToDelete = Vec3f(R, G, B);
	calculateColor();
	glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
	glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		mousePosition = Vec2f(xpos, ypos);
		usrednij(buff);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glUniform2f(glGetUniformLocation(shaderUtil.mProgramId, "start"), xpos, ypos);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glUniform2f(glGetUniformLocation(shaderUtil.mProgramId, "end"), xpos, ypos);
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (shader == 0) {
			GreenScreenOn();
			shader = 1;
			glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "shader"), shader);
			glUniform2f(glGetUniformLocation(shaderUtil.mProgramId, "start"), 0, 0);
			glUniform2f(glGetUniformLocation(shaderUtil.mProgramId, "end"), 0, 0);
		}
		else {
			shader = 0;
			glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "shader"), shader);

		}
	}
	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		rgb tmpRGB;
		tmpRGB.r = colorToDelete.val[0];
		tmpRGB.g = colorToDelete.val[1];
		tmpRGB.b = colorToDelete.val[2];
		hsv tmp = rgb2hsv(tmpRGB);

		if (tmp.s + Stala > 1.0f) {
			return;
		}
		else {
			tmp.s += Stala;
		}
		tmpRGB = hsv2rgb(tmp);
		colorToDelete = Vec3f(tmpRGB.r, tmpRGB.g, tmpRGB.b);
		calculateColor();
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
	}
	if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		rgb tmpRGB;
		tmpRGB.r = colorToDelete.val[0];
		tmpRGB.g = colorToDelete.val[1];
		tmpRGB.b = colorToDelete.val[2];
		hsv tmp = rgb2hsv(tmpRGB);
		if (tmp.s - Stala < 0.0f) {
			return;
		}
		else {
			tmp.s -= Stala;
		}
		tmpRGB = hsv2rgb(tmp);
		colorToDelete = Vec3f(tmpRGB.r, tmpRGB.g, tmpRGB.b);
		calculateColor();
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
	}
	if (key == GLFW_KEY_V && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		rgb tmpRGB;
		tmpRGB.r = colorToDelete.val[0];
		tmpRGB.g = colorToDelete.val[1];
		tmpRGB.b = colorToDelete.val[2];
		hsv tmp = rgb2hsv(tmpRGB);
		if (tmp.v + Stala * 100.0f > 255.0f) {
			return;
		}
		else {
			tmp.v += Stala * 100.0f;
		}

		tmpRGB = hsv2rgb(tmp);
		colorToDelete = Vec3f(tmpRGB.r, tmpRGB.g, tmpRGB.b);
		calculateColor();
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
	}
	if (key == GLFW_KEY_B && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		rgb tmpRGB;
		tmpRGB.r = colorToDelete.val[0];
		tmpRGB.g = colorToDelete.val[1];
		tmpRGB.b = colorToDelete.val[2];
		hsv tmp = rgb2hsv(tmpRGB);
		if (tmp.v - Stala * 100.0f < 0.0f) {
			return;
		}
		else {
			tmp.v -= Stala * 100.0f;
		}
		tmpRGB = hsv2rgb(tmp);
		colorToDelete = Vec3f(tmpRGB.r, tmpRGB.g, tmpRGB.b);
		calculateColor();
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
	}
	if (key == GLFW_KEY_KP_ADD && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		if (colorThreshold + 0.001 > 1.0f) {
			return;
		}
		colorThreshold += 0.001;
		calculateColor();
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
	}
	if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		if (colorThreshold - 0.001 < 0.0f) {
			return;
		}
		colorThreshold -= 0.001;
		calculateColor();
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
		glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		if (nextImg - 1 < 0) {
			return;
		}
		nextImg--;
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		if (nextImg + 1 >= texNum - 1) {
			return;
		}
		nextImg++;
	}
	printColor();
}

void BindCVMat2GLTexture(cv::Mat& image, GLuint& imageTexture)
{
	if (image.empty()) {
		std::cout << "image empty" << std::endl;
	}
	else {
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		//glGenTextures(1, &imageTexture);
		glBindTexture(GL_TEXTURE_2D, imageTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set texture clamping method
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		cv::cvtColor(image, image, COLOR_RGB2BGR);

		glTexImage2D(GL_TEXTURE_2D,         // Type of texture
			0,                   // Pyramid level (for mip-mapping) - 0 is the top level
			GL_RGB,              // Internal colour format to convert to
			image.cols,          // Image width  i.e. 640 for Kinect in standard mode
			image.rows,          // Image height i.e. 480 for Kinect in standard mode
			0,                   // Border width in pixels (can either be 1 or 0)
			GL_RGB,              // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
			GL_UNSIGNED_BYTE,    // Image data type
			image.ptr());        // The actual image data itself
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

int main() {
	kamerka = VideoCapture(0);
	if (kamerka.isOpened()) {
		kamerka.set(CAP_PROP_FRAME_WIDTH, 1920);
		kamerka.set(CAP_PROP_FRAME_HEIGHT, 1080);
		int width = (int)kamerka.get(CAP_PROP_FRAME_WIDTH);
		int height = (int)kamerka.get(CAP_PROP_FRAME_HEIGHT);
		sizee = Size(width, height);
		kamerka >> frame;
		//obraz
		images[0] = imread("dd.jpg", IMREAD_COLOR);
		flip(images[0], images[0], 0);
		//film
		film = VideoCapture(0);
		film.open("film.mp4");
		film >> images[1];

		for (int i = 0; i < texNum - 1; i++) {
			if (images[i].empty()) {
				cout << "Nie można otworzyć podanego obrazlu tła - " << i << endl;
				return -1;
			}
		}
	}
	else {
		return -1;
	}
	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(sizee.width, sizee.height, "Greenscreen", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	else
	{
		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

		// :: TRYB ORTOGRAFICZNY :: 

		glMatrixMode(GL_PROJECTION);
		glOrtho(0, sizee.width, 0, sizee.height, 1, -1);
		glMatrixMode(GL_MODELVIEW);

		// :: BINDOWANIE TEKSTUR ::

		glGenTextures(texNum, textures);

		glActiveTexture(GL_TEXTURE0);
		BindCVMat2GLTexture(frame, textures[0]);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0);
		BindCVMat2GLTexture(images[0], textures[1]);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);


		
		glActiveTexture(GL_TEXTURE0);
		BindCVMat2GLTexture(images[1], textures[2]);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetKeyCallback(window, key_callback);
		printInfo();

		while (!glfwWindowShouldClose(window))
		{
			// :: CZYSZCZENIE BUFFORA ::
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// :: NOWE DANE DO TEKSTURY ::

			kamerka >> frame;
			flip(frame, frame, 0);

			// :: PRZYPISANIE NOWYCH DANYCH DO ODPOWIEDNICH JEDNOSTEK TEKSTURUJĄCYCH
			glActiveTexture(GL_TEXTURE0);
			BindCVMat2GLTexture(frame, textures[0]);
			if (shader == 1) {
				if (nextImg == 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, textures[1]);
				}
				else if (nextImg == 1) {
					film.read(images[1]);
					if (images[1].empty()) {
						film.set(CAP_PROP_POS_FRAMES, 0);
						film.read(images[1]);
					}
					flip(images[1], images[1], 0);
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, textures[2]);
					BindCVMat2GLTexture(images[1], textures[2]);
				}

			}

			// :: RENDER ::

			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex2i(0, 0);
			glTexCoord2i(0, 1); glVertex2i(0, sizee.height);
			glTexCoord2i(1, 1); glVertex2i(sizee.width, sizee.height);
			glTexCoord2i(1, 0); glVertex2i(sizee.width, 0);
			glEnd();
			glDisable(GL_TEXTURE_2D);

			glfwSwapBuffers(window);
			glReadPixels(mousePosition[0], 480 - mousePosition[1], 3, 3, GL_RGB, GL_UNSIGNED_BYTE, buff);
			glfwPollEvents();
		}
		shaderUtil.Delete();
		film.release();
		kamerka.release();
		destroyAllWindows();
	}
	glfwTerminate();
	return 0;
}