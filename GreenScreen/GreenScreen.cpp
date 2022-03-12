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

static VideoCapture kamerka;
static Size sizee;
static Mat img;

// :: USTAWIENIA KOLORÓW ::

Vec3f colorToDelete = Vec3f(182, 203, 203);
float colorThreshold = 0.1; // 0 to 1
Vec3f l_colorToDelete, h_colorToDelete;
Vec2f mousePosition;
int shader = 0;
ShaderUtil shaderUtil;
unsigned char* buff = new unsigned char[1228800];

const int texNum = 2;
GLuint textures[texNum];

Mat frame;

void calculateColor() {
	Vec3f rangeVector = Vec3f(colorThreshold,colorThreshold,colorThreshold);
	l_colorToDelete = colorToDelete/255;
	l_colorToDelete -= rangeVector;	
	h_colorToDelete = colorToDelete/255;
	h_colorToDelete += rangeVector;
}

void GreenScreenON() {
	shaderUtil.Load("greenscreen.frag");
	shaderUtil.Use();
	glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "tex"), 0);
	glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "img"), 1);
	glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "low"), l_colorToDelete[0], l_colorToDelete[1], l_colorToDelete[2]);
	glUniform3f(glGetUniformLocation(shaderUtil.mProgramId, "high"), h_colorToDelete[0], h_colorToDelete[1], h_colorToDelete[2]);
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
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (shader == 0) {
			GreenScreenON();
			shader = 1;
			glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "shader"), shader);
		}
		else {
			shader = 0;
			glUniform1i(glGetUniformLocation(shaderUtil.mProgramId, "shader"), shader);

		}
	}
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
	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
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
		kamerka = VideoCapture(0);
		if (kamerka.isOpened()) {

			int width = (int)kamerka.get(CAP_PROP_FRAME_WIDTH);
			int height = (int)kamerka.get(CAP_PROP_FRAME_HEIGHT);
			sizee = Size(width, height);
			Mat img = imread("obraz.jpg", IMREAD_COLOR);
			if (img.empty()) {
				cout << "Nie można otworzyć podanego obrazlu tła!" << endl;
				return -1;
			}
		}

		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
		
		calculateColor();

		// :: WCZYTANIE POCZĄTKOWYCH WARTOŚCI TEKSTUR ::

		img = imread("obraz.jpg", IMREAD_COLOR);
		flip(img, img, 0);

		kamerka >> frame;

		// :: TRYB ORTOGRAFICZNY :: 

		glMatrixMode(GL_PROJECTION);
		glOrtho(0, 640, 0, 480, 1, -1);
		

		// :: BINDOWANIE TEKSTUR ::

		glGenTextures(texNum, textures);
		
		glActiveTexture(GL_TEXTURE0);
		BindCVMat2GLTexture(frame, textures[0]);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
			// Tutaj nic nie dzieje sie w jednym momencie, dlatego można zrobić na jednej jednostce shadującej

		glActiveTexture(GL_TEXTURE0);
		BindCVMat2GLTexture(img, textures[1]);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		
		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		
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
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, textures[1]);
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
			
			//cout << mousePosition[0] << " " << mousePosition[1] << " k " << static_cast<int>(buff[0]) << " " << static_cast<int>(buff[1]) << " " << static_cast<int>(buff[2]) << endl;
			//cout << mousePosition[0] << " " << mousePosition[1] << " k " << cos[0] << " " << cos[1] << " " << cos[2] << endl;
			glfwPollEvents();
		}
		shaderUtil.Delete();
		kamerka.release();
		destroyAllWindows();
	}

	glfwTerminate();
	return 0;
}


