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

static float a1 = 1.4;
static float a2 = 1.5;

GLuint ShaderProgram;
Vec2f fragCoord;
static VideoCapture kamerka;
static Size sizee;
static Mat img;

Mat frame;
GLuint tex;

Mat greenScreen(VideoCapture kamerka, Size size, Mat img);
void greenScreen2(VideoCapture kamerka, Size size, Mat img);



string LoadTextFile(string fileName) {
	ifstream plix;
	plix.open(fileName, ios::in);
	if (!plix.is_open()) {
		cout << "Open file error: " << fileName << endl;
		system("pause");
		exit(0);
	}
	// Odczyt całości pliku:
	string line, buff;
	while (!plix.eof()) {
		getline(plix, line);
		buff += line + "\n";
	}
	return(buff);
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
	}
}

void display() {  // Display function will draw the image.

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	kamerka >> frame;
	flip(frame, frame, 0);
	//upload to GPU texture
	
	
	glBindTexture(GL_TEXTURE_2D, 0);


	//clear and draw quad with texture (could be in display callback)
	glClear(GL_COLOR_BUFFER_BIT);

	BindCVMat2GLTexture(frame, tex);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2i(0,0);
	glTexCoord2i(0, 1); glVertex2i(0, frame.rows);
	glTexCoord2i(1, 1); glVertex2i(frame.cols, frame.rows);
	glTexCoord2i(1, 0); glVertex2i(frame.cols, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);




	glBindTexture(GL_TEXTURE_2D, 0);
	//glFlush();
	

	glutSwapBuffers(); // Required to copy color buffer onto the screen.

}

void OnIdle() {
	static int last_time;
	int now_time = glutGet(GLUT_ELAPSED_TIME);
	last_time = now_time;
	glutPostRedisplay();
}


int main() {
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
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
			img.convertTo(img, CV_32FC3, 1.0 / 255);
		}


		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

		// TODO: Create and compile shaders here (vertex and fragment shaders)
		// and finally draw something with modern OpenGL!
		ShaderUtil shaderUtil;
		shaderUtil.Load("greenscreen.frag");

		// Points for triangle
		float points[6] = {

			// Left
			-0.8f, -0.5f,

			// Top
			0.0f, 0.9f,

			// Right
			0.5f, -0.7f
		};

		unsigned int buffer;

		// Create a buffer
		glGenBuffers(1, &buffer);

		// Bind the buffer to vertx attributes
		glBindBuffer(GL_ARRAY_BUFFER, buffer);

		// Init buffer
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), points, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);


		shaderUtil.Use();
		GLint loc = glGetUniformLocation(shaderUtil.mProgramId, "tex");
		glUniform1i(loc, 0);

		glMatrixMode(GL_PROJECTION);
		glOrtho(0, 640, 0, 480, -1, 1);
		glMatrixMode(GL_MODELVIEW);

		


	
		/* Loop until the user closes the window */
		while (!glfwWindowShouldClose(window))
		{
			/* Render here */
			glClear(GL_COLOR_BUFFER_BIT);



			kamerka >> frame;
			flip(frame, frame, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			GLint loc = glGetUniformLocation(shaderUtil.mProgramId, "tex");
			glUniform1i(loc, 0);

			//clear and draw quad with texture (could be in display callback)
			glClear(GL_COLOR_BUFFER_BIT);

			BindCVMat2GLTexture(frame, tex);
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex2i(0, 0);
			glTexCoord2i(0, 1); glVertex2i(0, frame.rows);
			glTexCoord2i(1, 1); glVertex2i(frame.cols, frame.rows);
			glTexCoord2i(1, 0); glVertex2i(frame.cols, 0);
			glEnd();
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);




			// Draw triangle
			//glDrawArrays(GL_TRIANGLES, 0, 3);

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

			/* Poll for and process events */
			glfwPollEvents();
		}

		shaderUtil.Delete();
		kamerka.release();
		destroyAllWindows();

	}

	glfwTerminate();
	return 0;
}


/*

int main(int argc, char** argv)
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
		img.convertTo(img, CV_32FC3, 1.0 / 255);

		// :: INIT GL :: 
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_SINGLE);    // Use single color buffer and no depth buffer.
		glutInitWindowSize(640, 480);         // Size of display area, in pixels.
		glutInitWindowPosition(0, 0);     // Location of window in screen coordinates.


		ShaderUtil shaderUtil;
		shaderUtil.Load("greenscreen.frag");
		// Points for triangle
		float points[6] = {

			// Left
			-0.8f, -0.5f,

			// Top
			0.0f, 0.9f,

			// Right
			0.5f, -0.7f
		};

		unsigned int buffer;

		// Create a buffer
		glGenBuffers(1, &buffer);

		// Bind the buffer to vertx attributes
		glBindBuffer(GL_ARRAY_BUFFER, buffer);

		// Init buffer
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), points, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

		shaderUtil.Use();



		glutCreateWindow("GL RGB Triangle"); // Parameter is window title.
		glutDisplayFunc(display);            // Called when the window needs to be redrawn.
		glutIdleFunc(OnIdle);



		// Loop until the user closes the window 

		


		//match projection to window resolution (could be in reshape callback)
		glMatrixMode(GL_PROJECTION);
		glOrtho(0, 640, 0, 480, -1, 1);
		glMatrixMode(GL_MODELVIEW);

		glutMainLoop(); //

	
		shaderUtil.Delete();
		kamerka.release();
		destroyAllWindows();
	}
	else {
		cout << "Nie udało się połączyć z kamerką !" << endl;
	}
	return 0;
}

*/

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