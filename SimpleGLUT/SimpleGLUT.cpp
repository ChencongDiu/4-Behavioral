#include "stdafx.h"

// standard
#include <assert.h>
#include <math.h>
#include <iostream>

// glut
#define GLUT_DISABLE_ATEXIT_HACK
#include <gl/glut.h>

using namespace std;
#define pi 3.1415926

//================================
// global variables
//================================
// screen size
int g_screenWidth = 0;
int g_screenHeight = 0;

// frame index
int g_frameIndex = 0;

// angle for rotation
int g_angle = 0;
double g_xdistance = 0.0;
double g_ydistance = 0.0;

// balls parameters
GLfloat radius = 0.1f; //balls radius
int numberBalls = 50; //number of balls
GLfloat curPositions[50][3] = { 0 }; //positions of balls of current frame
//GLfloat nextPositions[20][3] = { 0 }; //positions of balls of next frame
GLfloat curVelocity[50][3] = { 0 }; //velocities of balls of current frame
//GLfloat nextVelocity[20][3] = { 0 }; //velocities of balls of next frame
GLfloat ballsM[50][16] = { 0 }; //balls matrix
GLfloat maxVelocity = 2.5f; // max velocity
// GLfloat center[3] = { 0 }; // center of balls at a certain time
GLfloat t = 0.01f;
GLfloat w1 = 0.035f; // weight of following leader
GLfloat w2 = 1.0f; // weight of clustering
GLfloat w3 = 1.0f; // weight of keep distance with other balls

// leader parameters
int numberPositions = 6; //number of positions
int index = 0;
GLfloat bodyPositions[6][7] = {
	{ 1.0f, 1.0f, 1.0f, 1.0f, 8.0f, 0.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, -1.0f },
	{ 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 5.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f, 5.0f, -2.0f, 5.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f, 5.0f, 2.0f, 0.0f },
	{ 0.0f, 1.0f, 1.0f, 1.0f, 8.0f, 0.0f, 5.0f }
};
GLfloat leaderM[16] = { 0 };
GLfloat time = 0.0f;
GLfloat dist = 0.0f;

// B-Spline Matrix
GLfloat BSplineM[16] = {
	-1.0f / 6.0f, 3.0f / 6.0f, -3.0f / 6.0f, 1.0f / 6.0f,
	3.0f / 6.0f, -6.0f / 6.0f, 0.0f / 6.0f, 4.0f / 6.0f,
	-3.0f / 6.0f, 3.0f / 6.0f, 3.0f / 6.0f, 1.0f / 6.0f,
	1.0f / 6.0f, 0.0f / 6.0f, 0.0f / 6.0f, 0.0f / 6.0f
};

//================================
// Print String on Screen
//================================
void drawBitmapText(char *string, float x, float y, float z)
{
	char *c;
	glRasterPos3f(x, y, z);
	for (c = string; *c != '\0'; c++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
	}
}

//================================
// initialize balls matrix ballsM with initPositions
//================================
void init(void) {
	//initialize initPositions with random positions
	//and velocities of balls
	for (int i = 0; i < numberBalls; i++) {
		curPositions[i][0] = 4.0f + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2.0f); //x
		curPositions[i][1] = -0.5f + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 1.0f); //y
		curPositions[i][2] = -2.0f + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2.0f); //z
		curVelocity[i][0] = -1.0f + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2.0f); //v_x
		curVelocity[i][1] = -1.0f + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2.0f); //v_y
		curVelocity[i][2] = -1.0f + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2.0f); //v_z
		ballsM[i][15] = 1.0f;
		for (int j = 0; j < 3; j++) {
			ballsM[i][5 * j] = 1.0f;
			ballsM[i][12 + j] = curPositions[i][j];
		}
	}
}


// generate floor
void floorGenerator() {
	glBegin(GL_LINES);
	for (int i = -100;i <= 100;i += 1) {
		glVertex3f(i, 0, -100);
		glVertex3f(i, 0, 100);
		glVertex3f(-100, 0, i);
		glVertex3f(100, 0, i);
	};
	glEnd();
}

// blending function
GLfloat blendFunc(GLfloat T[4], GLfloat M[16], GLfloat P[4]) {
	// TM[4] = T*M
	GLfloat TM[4] = { 0 };
	TM[0] = T[0] * M[0] + T[1] * M[1] + T[2] * M[2] + T[3] * M[3];	 //column 1
	TM[1] = T[0] * M[4] + T[1] * M[5] + T[2] * M[6] + T[3] * M[7];	 //column 2
	TM[2] = T[0] * M[8] + T[1] * M[9] + T[2] * M[10] + T[3] * M[11];  //column 3
	TM[3] = T[0] * M[12] + T[1] * M[13] + T[2] * M[14] + T[3] * M[15];//column 4

	GLfloat res = TM[0] * P[0] + TM[1] * P[1] + TM[2] * P[2] + TM[3] * P[3];

	return res;
}

// normalization
void norm(GLfloat m[4]) {
	GLfloat sumsqrt = m[0] * m[0] + m[1] * m[1] + m[2] * m[2] + m[3] * m[3];
	if (sumsqrt != 0) // avoid being divided by 0
	{
		GLfloat base = sqrt(sumsqrt);
		m[0] = m[0] / base;
		m[1] = m[1] / base;
		m[2] = m[2] / base;
		m[3] = m[3] / base;
	}
}

// interpolator by positions and spline matrix
void bodyInterpolater(GLfloat bodyPositions[6][7], GLfloat splineM[16]) {
	GLfloat tempM[7] = { 0 };
	GLfloat timeM[4] = { time * time * time, time * time, time, 1 };
	// 4 points a group
	for (int i = 0; i < 7; i++) {
		GLfloat fourPos[4] = { bodyPositions[(index + 0) % numberPositions][i],
			bodyPositions[(index + 1) % numberPositions][i],
			bodyPositions[(index + 2) % numberPositions][i],
			bodyPositions[(index + 3) % numberPositions][i] };
		tempM[i] = blendFunc(timeM, splineM, fourPos);
	}

	norm(tempM);

	GLfloat w = tempM[0];
	GLfloat x = tempM[1];
	GLfloat y = tempM[2];
	GLfloat z = tempM[3];

	//interpolation of body matrix
	leaderM[0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
	leaderM[1] = 2.0f * x * y + 2.0f * w * z;
	leaderM[2] = 2.0f * x * z - 2.0f * w * y;
	leaderM[3] = 0.0f;
	leaderM[4] = 2.0f * x * y - 2.0f * w * z; //col2
	leaderM[5] = 1.0f - 2.0f * x * x - 2.0f * z * z;
	leaderM[6] = 2.0f * y * z + 2.0f * w * x;
	leaderM[7] = 0.0f;
	leaderM[8] = 2.0f * x * z + 2.0f * w * y; //col3
	leaderM[9] = 2.0f * y * z - 2.0f * w * x;
	leaderM[10] = 1.0f - 2.0f * x * x - 2.0f * y * y;
	leaderM[11] = 0.0f;
	leaderM[12] = tempM[4]; //col4
	leaderM[13] = tempM[5];
	leaderM[14] = tempM[6];
	leaderM[15] = 1.0f;
}


// generate a leader using quaternion and BSpline
void leaderGenerator() {
	glPushMatrix();
	bodyInterpolater(bodyPositions, BSplineM);
	//glGetFloatv(GL_MODELVIEW_MATRIX, bodyM);
	glMultMatrixf(leaderM);
	glutSolidTeapot(0.25);
	glPopMatrix();
}

//euler distance between two points
GLfloat distance(GLfloat pos1[3], GLfloat pos2[3]) {
	return sqrt((pos1[0] - pos2[0]) * (pos1[0] - pos2[0]) +
		(pos1[1] - pos2[1]) * (pos1[1] - pos2[1]) +
		(pos1[2] - pos2[2]) * (pos1[2] - pos2[2]));
}

// follow the leader, also keep distance with leader
// 1. farer from leader, faster it come closer.
// 2. closer with leader, faster it get away,
// perfect distance is 2
void follow(int index, GLfloat a[3]) {
	GLfloat leaderPos[3] = { 0 };
	GLfloat ballPos[3] = { 0 };
	GLfloat direction = 1;
	for (int i = 0; i < 3; i++) {
		leaderPos[i] = leaderM[12 + i];
		ballPos[i] = ballsM[index][12 + i];
	}
	// calculate acceleration
	GLfloat dis = distance(leaderPos, curPositions[index]);
	if (dis < 5 * radius) direction = -1;
	dis -= 5 * radius;
	for (int i = 0; i < 3; i++) {
		a[i] = direction * (leaderM[12 + i] - ballsM[index][12 + i]) * (dis * dis);
	}
}

// clustering, similar with clustering in ML
void cluster(int index, GLfloat a[3]) {
	GLfloat center[3] = { 0 };
	for (int i = 0; i < numberBalls; i++) {
		for (int j = 0; j < 3; j++) {
			center[j] += (i == index)? 0: ballsM[i][12 + j];
		}
	}
	for (int j = 0; j < 3; j++) {
		center[j] /= (numberBalls - 1);
	}
	// calculate acceleration
	GLfloat dis = distance(center, curPositions[index]);
	for (int i = 0; i < 3; i++) {
		a[i] = (center[i] - ballsM[index][12 + i]) * (dis * dis);
	}
}

// avoid collision, keep distance with others
void collision(int index, GLfloat a[3]) {
	for (int i = 0; i < numberBalls; i++) {
		if (i != index) {
			GLfloat dis = distance(curPositions[index], curPositions[i]);
			if (dis < 4 * radius) {
				//dis -= 4 * radius;
				for (int j = 0; j < 3; j++) {
					a[j] = (curPositions[index][j] - curPositions[i][j]) / (dis * dis);
				}
			}
		}
	}
}

// adjust velocities of balls to simulate behavioral motion control system
void interpolater() {
	// several accelerations following coresponding behavioral motions
	GLfloat a1[3] = { 0 };
	GLfloat a2[3] = { 0 };
	GLfloat a3[3] = { 0 };
	GLfloat center[3] = { 0 };
	GLfloat leaderPos[3] = { 0 };
	for (int i = 0; i < numberBalls; i++) {
		// 1. follow the leader
		follow(i, a1);
		// 2. clustering
		cluster(i, a2);
		// 3. collision
		collision(i, a3);
		// update new velocities and positions
		for (int j = 0; j < 3; j++) {
			curVelocity[i][j] = curVelocity[i][j] + (w1 * a1[j] + w2 * a2[j] + w3 * a3[j]) * t;
			curVelocity[i][j] = (curVelocity[i][j] > maxVelocity) ? maxVelocity : curVelocity[i][j];
			curPositions[i][j] = curPositions[i][j] + curVelocity[i][j] * t;
			ballsM[i][12 + j] = curPositions[i][j];
			center[j] += curPositions[i][j];
		}
	}
	// guide line
	for (int j = 0; j < 3; j++) {
		center[j] /= numberBalls;
		leaderPos[j] = leaderM[12 + j];
	}
	dist = distance(center, leaderPos);
	glBegin(GL_LINES);
	glVertex3f(center[0], center[1], center[2]);
	glVertex3f(leaderPos[0], leaderPos[1], leaderPos[2]);
	glEnd();
}

// contorl all followers' velocities with some adjust functions
void followerGenerator() {
	interpolater();
	for (int i = 0; i < numberBalls; i++) {
		glPushMatrix();
		glMultMatrixf(ballsM[i]);
		glutSolidSphere(radius, 25, 25);
		glPopMatrix();
	}
}

//================================
// render
//================================
void render(void) {
	// clear buffer
	glClearColor(0.5, 0.5, 0.5, 0.7);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render state
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	// modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -5.0f);
	gluLookAt(0.0f, 3.0f, 0.0f, 3.0f, 0.0f, 3.0f, 0.0f, 1.0f, 0.0f);

	//helper and 6 points
	char line[100] = { "Distance from Leader to Center of Follower: "};
	sprintf(line, "Distance from Leader to Center of Follower: %f", dist);
	drawBitmapText("Press Spacebar to Exit", 13.0, 2.5, 0.0);
	drawBitmapText(line, 13.7, 2.0, 0.0);

	// enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// light source attributes
	GLfloat LightAmbient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat LightDiffuse[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat LightSpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat LightPosition[] = { 5.0f, 5.0f, 5.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

	// surface material attributes
	GLfloat material_Ka[] = { 0.11f, 0.06f, 0.11f, 1.0f };
	GLfloat material_Kd[] = { 0.43f, 0.47f, 0.54f, 1.0f };
	GLfloat material_Ks[] = { 0.33f, 0.33f, 0.52f, 1.0f };
	GLfloat material_Ke[] = { 0.1f , 0.0f , 0.1f , 1.0f };
	GLfloat material_Se = 10;

	glMaterialfv(GL_FRONT, GL_AMBIENT, material_Ka);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, material_Kd);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_Ks);
	glMaterialfv(GL_FRONT, GL_EMISSION, material_Ke);
	glMaterialf(GL_FRONT, GL_SHININESS, material_Se);

	// render ground, leader ball and follwer cones
	floorGenerator();
	leaderGenerator();
	followerGenerator();

	// disable lighting
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	// swap back and front buffers
	glutSwapBuffers();
}

//================================
// keyboard input
//================================
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case ' ':
		exit(0);
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

//================================
// reshape : update viewport and projection matrix when the window is resized
//================================
void reshape(int w, int h) {
	// screen size
	g_screenWidth = w;
	g_screenHeight = h;

	// viewport
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	// projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLfloat)w / (GLfloat)h, 1.0, 2000.0);
}

//================================
// timer : triggered every 16ms ( about 60 frames per second )
//================================
void timer(int value) {
	// render
	glutPostRedisplay();

	// increase frame index
	g_frameIndex++;

	// time++
	time += t;
	//index++
	if (time >= 1.0f) {
		time = 0.0f;
		if (index < numberPositions - 1) {
			index++;
		}
		else {
			index = 0;
		}
	}

	// reset timer
	// 16 ms per frame ( about 60 frames per second )
	glutTimerFunc(16, timer, 0);
}

//================================
// main
//================================
int main(int argc, char** argv) {
	// create opengL window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1600, 900);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Lab4");

	// init
	init();

	// set callback functions
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(16, timer, 0);

	// main loop
	glutMainLoop();

	return 0;
}