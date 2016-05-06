#define _USE_MATH_DEFINES
#define KEY_UP 0
#define KEY_DOWN 1
#define KEY_USED 2
#define RESOLUTION 100
#define CLICK_RADIUS 0.1
#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// Needed on MsWindows
#include <windows.h>
#endif // Win32 platform

#include <GL/gl.h>
#include <GL/glu.h>
// Download glut from: http://www.opengl.org/resources/libraries/glut/
#include <GL/freeglut.h>
#include "stb_image.h"

#include <stdlib.h>
#include <vector>

const float PI = 3.1415926535;
const float POINT_SIZE = 40.0;

bool dragging;
bool drawing = false;
bool robots = false;

class float2
{
public:
	//members
	float x;
	float y;

	//constructors
	float2() :x(0.0f), y(0.0f) {}
	float2(float x, float y) :x(x), y(y) {}

	// more methods here

	// invoke this as:
	//	 float2 myRandomPosition = float2::random();
	static float2 random()
	{
		return float2(
			((float)rand() / RAND_MAX) * 2 - 1,
			((float)rand() / RAND_MAX) * 2 - 1);
	}

	float2 operator-() const
	{
		return float2(-x, -y);
	}

	float2 operator*(float s) const
	{
		return float2(x * s, y * s);
	}

	float2 operator+(float2 b) const
	{
		return float2(x + b.x, y + b.y);
	}

	float2 operator+=(float2 b) //no const
	{
		x += b.x;
		y += b.y;
		return *this;
	}

	float2 operator-=(float2 b) //no const
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}

	float2 operator/=(float b) //no const
	{
		x /= b;
		y /= b;
	}

	float lengthSquared() const
	{
		return x*x + y*y;
	}

	float length() const
	{
		return sqrtf(lengthSquared());
	}

	float2 normalize()
	{
		*this /= length();
		return *this;
	}

};

//class Object
//{
//public:
//	void draw()
//	{
//		// apply scaling, translation and orientation
//		glPushMatrix();
//		glTranslatef(position.x, position.y, 0.0f);
//		glRotatef(orientation, 0.0f, 0.0f, 1.0f);
//		glScalef(scaleFactor.x, scaleFactor.y, 1.0f);
//		drawModel();
//		glPopMatrix();
//	}
//};

class Texture
{
	// OpenGL texture identifier
	unsigned int textureId;
public:
	// creates OpenGL texture from image file
	Texture(const std::string& inputFileName) {
		unsigned char* data;
		int width; int height; int nComponents = 4;
		data = stbi_load(inputFileName.c_str(), &width, &height, &nComponents, 0);
		if (data == NULL) return;
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		//gluBuild2DMipmaps(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, data);
		delete data;
	}

	// sets this as the current OpenGL texture
	void bind()
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
	}
};

float distance(float2 p1, float2 p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

class Curve
{
public:

	bool selected;
	// return curve point at parameter t in [0,1]
	virtual float2 getPoint(float t) = 0;
	virtual float2 getDerivative(float t) = 0;
	virtual int getControlPointCount() = 0;
	virtual void addControlPoint(float2) = 0;
	virtual void drawControlPoints() = 0;
	virtual void deleteCloseControlPoint(float2) = 0;
	virtual void drag(float2) = 0;
	
	virtual std::vector<float2> getControlPoints() {
		std::vector<float2> f;
		return f;
	}

	virtual void draw()
	{
		printf("drawing\n");
		// draw a line strip of small segments along the curve
		glLineWidth(3);
		selected ? glColor3f(0, 0, 1) : glColor3f(1, 1, 1);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= RESOLUTION; i++)
		{
			float2 point = getPoint((float)i / RESOLUTION);
			glVertex2fv((float*)&point);
		}
		glEnd();
	}

	void drawTracker(float t)
	{
		if (robots) {
			//Texture bot("bot.bmp");
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);
			try {
				Texture bot("bot.bmp");
				bot.bind();
			} catch (...) {}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexEnvi(GL_TEXTURE_ENV,	GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0); glVertex2d(getPoint(t).x-.1, getPoint(t).y-.1);
			glTexCoord2d(1.0, 0.0); glVertex2d(getPoint(t).x+.1, getPoint(t).y-.1);
			glTexCoord2d(1.0, 1.0); glVertex2d(getPoint(t).x+.1, getPoint(t).y+.1);
			glTexCoord2d(0.0, 1.0); glVertex2d(getPoint(t).x-.1, getPoint(t).y+.1);
			glEnd();
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
		}

		//glBegin(GL_LINES);
		//float2 point0 = getPoint(t);
		//float2 point1 = point0 + getDerivative(t);
		//glVertex2fv((float*)&point0);
		//glVertex2fv((float*)&point1);
		//glEnd();
	}
};

class Freeform : public Curve 
{
protected:
	//list of points to draw line through
	std::vector<float2> controlPoints;

public:
	std::vector<float2> getControlPoints() {
		return controlPoints;
	}

	int getControlPointCount()
	{
		return controlPoints.size();
	}

	//virtual float2 getPoint(float) = 0;

	virtual void addControlPoint(float2 p) 
	{
		controlPoints.push_back(p);
	}

	void drawControlPoints()
	{
		for (float2 t : controlPoints)
		{
			glPointSize(10);
			selected ? glColor3f(1, 0, 0) : glColor3f(0,1,1);
			glBegin(GL_POINTS);
			//float2 point = getPoint(t);
			glVertex2fv((float*)&t);
			glEnd();
		}
	}

	void deleteCloseControlPoint(float2 t) {
		float minDistance = 2;
		int point;
		for (int i = 0; i < controlPoints.size(); i++) {
			if (distance(t, controlPoints[i]) < minDistance) {
				minDistance = distance(t, controlPoints[i]);
				point = i;
			}
		}
		if (minDistance <= CLICK_RADIUS) {
			controlPoints.erase(controlPoints.begin() + point);
		}
	}

	void drag(float2 t) {
		float minDistance = 2;
		int point;
		for (int i = 0; i < controlPoints.size(); i++) {
			if (distance(t, controlPoints[i]) < minDistance) {
				minDistance = distance(t, controlPoints[i]);
				point = i;
			}
		}
		if (minDistance <= CLICK_RADIUS) {
			controlPoints[point] = t;
		}
	}
};

class PolyLine : public Freeform
{
public:

	float2 getPoint(float t)
	{
		float length = 0;
		for (int i = 1; i < controlPoints.size(); i++) {
			length += distance(controlPoints[i], controlPoints[i - 1]);
		}
		if (t == 0.0) {
			return controlPoints[0];
		}
		return controlPoints[t*controlPoints.size() - 1];// +((controlPoints[t*controlPoints.size() - .01] + (-controlPoints[t*controlPoints.size() - 1]))*t);
	}

	float2 getDerivative(float t)
	{
		return float2(0.0, 0.0);
	}
};

class Bezier : public Freeform
{
public:
	static double bernstein(int i, int n, double t) {
		if (n == 1) {
			if (i == 0) 
				return 1 - t;
			if (i == 1) 
				return t;
			return 0;
		}
		if (i < 0 || i > n) 
			return 0;
		return (1 - t) * bernstein(i, n - 1, t) + t  * bernstein(i - 1, n - 1, t);
	}

	float2 getPoint(float t)
	{
		float2 r(0.0, 0.0);
		// for every control point
		// compute weight using the Bernstein formula
		// add control point to r, weighted
		for (int i = 0; i < controlPoints.size();i++)
		{
			r+=controlPoints[i]*bernstein(i, controlPoints.size()-1, t);
		}
		return r;
	}

	float2 getDerivative(float t)
	{
		// scale t to [0, 2 pi]
		// evaluate parametric circle formula
		// return result;
		float alpha = t * 2 * PI;
		return float2(-sin(alpha), cos(alpha));
	}
};

class Lagrange : public Freeform
{
	std::vector<float> knots;
public:
	void addControlPoint(float2 p)
	{
		controlPoints.push_back(p);
		knots.clear();
		for (int i = 0; i<controlPoints.size(); i++)
			knots.push_back((float)i / (controlPoints.size() - 1));
	}

	void deleteCloseControlPoint(float2 t) {
		float minDistance = 2;
		int point;
		for (int i = 0; i < controlPoints.size(); i++) {
			if (distance(t, controlPoints[i]) < minDistance) {
				minDistance = distance(t, controlPoints[i]);
				point = i;
			}
		}
		if (minDistance <= CLICK_RADIUS) {
			controlPoints.erase(controlPoints.begin() + point);
		}
		knots.clear();
		for (int i = 0; i<controlPoints.size(); i++)
			knots.push_back((float)i / (controlPoints.size() - 1));
	}

	float weight(int i, float t) {
		float num = 1, denom = 1;
		for (int j = 0; j < knots.size(); j++) {
			if (j != i) {
				num *= t - knots[j];
			}
		}
		for (int j = 0; j < knots.size(); j++) {
			if (j != i) {
				denom *= knots[i] - knots[j];
			}
		}
		return num / denom;
	}

	float2 getPoint(float t)
	{
		float2 r(0.0, 0.0);
		// for every control point
		// compute weight using the Lagrange formula
		// add control point to r, weighted
		for (int i = 0; i < controlPoints.size(); i++)
		{
			r += controlPoints[i] * weight(i, t);
		}
		return r;
	}

	float2 getDerivative(float t)
	{
		// scale t to [0, 2 pi]
		// evaluate parametric circle formula
		// return result;
		float alpha = t * 2 * PI;
		return float2(-sin(alpha), cos(alpha));
	}
};

class Circle : public Curve
{
	// center and radius stored in members

	float2 center;
	float radius;

public:
	// constructor to initialize members
	Circle(float2 c, float r)
	{
		center = c; radius = r;
	}

	float2 getPoint(float t)
	{
		// scale t to [0, 2 pi]
		// evaluate parametric circle formula
		// return result;

		float alpha = t * 2 * PI;
		return float2(radius * cos(alpha), radius * sin(alpha));
	}

	float2 getDerivative(float t)
	{
		// scale t to [0, 2 pi]
		// evaluate parametric circle formula
		// return result;
		float alpha = t * 2 * PI;
		return float2(-sin(alpha), cos(alpha));
	}
};

std::vector<Curve*> curves;

unsigned char keys[255];
unsigned char mouse[3];

float trackPosition;
int selected = 0;
int click;
float2 cursor;

int changeSelectedTo(int i) {
	if (dragging) {
		return 2;
	}
	curves[selected % curves.size()]->selected = false;
	curves[i % curves.size()]->selected = true;
	if (selected == i) {
		return 2;
	}
	selected = i;
	return 1;
}

void changeSelected() {
	curves[selected++ % curves.size()]->selected = false;
	curves[selected % curves.size()]->selected = true;
}

int selectCloseLine(float2 t) // 0 if no curve selected, 1 if curve changed, 2 if same curve
{
	float minDistance = 2;
	int curve;
	for (int i = 0; i < curves.size(); i++)
	{
		for (int j = 0; j <= RESOLUTION; j++)
		{
			if (distance(t, (*curves[i]).getPoint((float)j / RESOLUTION)) < minDistance) {
				minDistance = distance(t, (*curves[i]).getPoint((float)j / RESOLUTION));
				curve = i;
			}
		}
		for (int j = 0; j < (*curves[i]).getControlPoints().size(); j++) {
			if (distance(t, (*curves[i]).getControlPoints()[j]) < minDistance) {
				minDistance = distance(t, (*curves[i]).getControlPoints()[j]);
				curve = i;
			}
		}
	}
	if (minDistance <= CLICK_RADIUS) {
		return changeSelectedTo(curve);
	}
	return 0;
}

//--------------------------------------------------------
// Display callback that is responsible for updating the screen
//--------------------------------------------------------

void onDisplay() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);        	// clear color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen


	for (int i = 0; i < curves.size(); i++)
	{
		printf("%i : i \n%i : num curves\n", i, curves.size());
		if ((*curves[i]).getControlPointCount() < 2 && !drawing) {
			curves.erase(curves.begin()+i);
		}
		else {
			(*curves[i]).draw();			
			(*curves[i]).drawTracker(trackPosition);
			(*curves[i]).drawControlPoints();
		}
	}

	//dealing with the keyboard
	if (keys[' '] == KEY_DOWN) {
		changeSelected();
		keys[' '] = KEY_USED;
	}
	else if (keys['a'] == KEY_DOWN) {
		if (click == KEY_DOWN) {
			(*curves[selected % curves.size()]).addControlPoint(cursor);
			click = KEY_USED;
		}
	}
	else if (keys['d'] == KEY_DOWN) {
		if (click == KEY_DOWN) {
			(*curves[selected % curves.size()]).deleteCloseControlPoint(cursor);
			click = KEY_USED;
		}
	}
	else if (keys['b'] == KEY_DOWN) {
		if (click == KEY_DOWN) {
			drawing = true;
			Bezier* temp = new Bezier();
			(*temp).addControlPoint(cursor);
			curves.push_back(temp);
			click = KEY_USED;
			keys['b'] = KEY_USED;
		}
	}
	else if (keys['b'] == KEY_USED) {
		if (click == KEY_DOWN) {
			(*curves[curves.size() - 1]).addControlPoint(cursor);
			click = KEY_USED;
		}
	}
	else if (keys['l'] == KEY_DOWN) {
		if (click == KEY_DOWN) {
			drawing = true;
			Lagrange* temp = new Lagrange();
			(*temp).addControlPoint(cursor);
			curves.push_back(temp);
			click = KEY_USED;
			keys['l'] = KEY_USED;
		}
	}
	else if (keys['l'] == KEY_USED) {
		if (click == KEY_DOWN) {
			(*curves[curves.size() - 1]).addControlPoint(cursor);
			click = KEY_USED;
		}
	}
	else if (keys['p'] == KEY_DOWN) {
		if (click == KEY_DOWN) {
			drawing = true;
			PolyLine* temp = new PolyLine();
			(*temp).addControlPoint(cursor);
			curves.push_back(temp);
			click = KEY_USED;
			keys['p'] = KEY_USED;
		}
	}
	else if (keys['p'] == KEY_USED) {
		if (click == KEY_DOWN) {
			(*curves[curves.size() - 1]).addControlPoint(cursor);
			click = KEY_USED;
		}
	}
	else if (keys['q'] == KEY_DOWN) {
		robots = !robots;
		keys['q'] = KEY_USED;
	}
	else if (click == KEY_DOWN) {
		if (selectCloseLine(cursor) == 2) {
			dragging = true;
			(*curves[selected % curves.size()]).drag(cursor);
		}
		else {
			dragging = false;
			selectCloseLine(cursor);
			click = KEY_USED;
		}
	}
	else if (click != KEY_DOWN) {
		dragging = false;
	}
	if (keys['b'] == KEY_UP && keys['p'] == KEY_UP && keys['l'] == KEY_UP) {
		drawing = false;
	}
	glutSwapBuffers();								// Swap buffers for double buffering
}

void onReshape(int winWidth0, int winHeight0)
{
	glViewport(0, 0, winWidth0, winHeight0);
}

void onIdle()
{
	// time elapsed since program started, in seconds 
	double time = glutGet(GLUT_ELAPSED_TIME) * 0.001;

	// change position
	trackPosition = time * 0.1 - floor(time* 0.1);

	// show result
	glutPostRedisplay();
}

void onKeyboard(unsigned char key, int x, int y) {
	if (keys[key] != KEY_USED) {
		keys[key] = KEY_DOWN;
	}
}

void onKeyboardUp(unsigned char key, int x, int y) {
	keys[key] = KEY_UP;
}

void onMouseMove(int x, int y) {
	int viewportRect[4];
	glGetIntegerv(GL_VIEWPORT, viewportRect);
	cursor = float2(x * 2.0 / viewportRect[2] - 1.0, -y * 2.0 / viewportRect[3] + 1.0);
}

void onMouseClick(int button, int state, int x, int y) {
	if (state == GLUT_DOWN) {
		if (click != KEY_USED) {
			click = KEY_DOWN;
		}
	}
	else {
		click = KEY_UP;
	}
}

//--------------------------------------------------------
// The entry point of the application 
//--------------------------------------------------------

int main(int argc, char *argv[]) {
	//Circle circle0(float2(0, 0), 0.8);
	//Circle circle1(float2(0, 0), 0.6);

	Bezier curve0;
	curve0.addControlPoint(float2(1, 1));
	curve0.addControlPoint(float2(0, 1));
	curve0.addControlPoint(float2(0, -1));
	curve0.addControlPoint(float2(-1, -1));
	curves.push_back(&curve0);

	Lagrange curve1;
	curve1.addControlPoint(float2(-1, 1));
	curve1.addControlPoint(float2(0, -1));
	curve1.addControlPoint(float2(0, 1));
	curve1.addControlPoint(float2(1, -1));
	curves.push_back(&curve1);

	PolyLine curve2;
	curve2.addControlPoint(float2(1, -1));
	curve2.addControlPoint(float2(.75, -.75));
	curves.push_back(&curve2);

	//circles.push_back(circle0);
	//circles.push_back(circle1);

	Texture bot("bot.bmp");
	bot.bind();

	glutInit(&argc, argv);                 		// GLUT initialization
	glutInitWindowSize(512, 512);				// Initial resolution of the MsWindows Window is 600x600 pixels 
	glutInitWindowPosition(100, 100);            // Initial location of the MsWindows window
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);    // Image = 8 bit R,G,B + double buffer + depth buffer

	glutCreateWindow("Curve Editor");        	// Window is born

	glutDisplayFunc(onDisplay);                	// Register event handlers
	glutReshapeFunc(onReshape);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMouseFunc(onMouseClick);
	glutPassiveMotionFunc(onMouseMove);
	glutMotionFunc(onMouseMove);

	glutIgnoreKeyRepeat(1);

	glutMainLoop();                    			// Event loop  
	return 0;
}