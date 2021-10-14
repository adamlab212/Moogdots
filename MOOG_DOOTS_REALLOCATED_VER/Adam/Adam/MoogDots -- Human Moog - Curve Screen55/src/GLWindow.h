#pragma once

#ifndef GLWINDOW
#define GLWINDOW

#include "StdAfx.h"

#include "GlobalDefs.h"
#include "Grid.h"
#include "ParameterList.h"
#include "Logger.h"
//
#include <wx\msw\glcanvas.h>
#include <wx/numdlg.h>

#include <thread>




#define USE_ANTIALIASING 1		// 1 = use anti-aliasing, 0 = don't
#define FP_DOTSIZE 5			// Fixation point size
#define LEFT_EYE 1
#define RIGHT_EYE 2
#define RED 1.0, 0.0, 0.0
#define GREEN 0.0, 1.0, 0.0

#define CYLINDERS 0x1
#define FLOOR 0x10
#define STARFIELD 0x100
#define TEXTURE 0x1000
#define SPHEREFIELD 0x10000


using namespace std;

class GLPanel;


// Defines a stereo glFrustum.
typedef struct FRUSTUM_STRUCT
{
	GLfloat screenWidth,		// Width of the screen.
		    screenHeight,		// Height of the screen.
			camera2screenDist,	// Distance from the camera to the screen.
			clipNear,			// Distance from camera to near clipping plane.
			clipFar,			// Distance from camera to far clipping plane.
			eyeSeparation,		// Interocular distance
			worldOffsetX,		// Shifts the entire world horizontally.
			worldOffsetZ;		// Shifts the entire world vertically.
} Frustum;

// Defines a 3D field of stars.
typedef struct STARFIELD_STRUCT
{
	vector<double> dimensions,				// Width, height, depth dimensions of starfield.
		triangle_size,			// Base width, height for each star.
		fixationPointLocation,	// (x,y,z) origin of fixation point.
		targ1Location,			// (x,y,z) origin of target 1.
		targ2Location,			// (x,y,z) origin of target 2.
		starLeftColor,			// (r,g,b) value of left eye starfield.
		starRightColor;			// (r,g,b) value of right eye starfield.
	double density,
		drawTarget,
		drawFixationPoint,
		drawFlashingFixationPoint,
		drawTarget1,
		drawTarget2,
		drawBackground,
		targetSize,
		luminance,
		probability,
		objectProbability,
		use_lifetime,
		use_objectLiftime,
		starRadius,
		starPointSize,
		starInc,
		cutoutRadius;
	int totalStars,
		lifetime,
		objectLifetime,
		drawMode;
	bool useCutout,
		drawCutout,
		stayCutout;
	//vector<double> sphereFieldPara;		// add special cirle patch of star field inside
} StarField;

// Represents a single star.
typedef struct STAR_STRUCT
{
	// Defines the 3 vertexes necessary to form a triangle.
	GLdouble x[3], y[3], z[3];
} Star;

// Represents the floor object.
typedef struct FLOOR_STRUCT
{
	double dimensions[2],
		   origin[3],
		   objectSize,
		   density;
	int count,				// Number of floor objects.
		drawMode;
	bool enable;			// Turns the floor on/off.
	Star *vertices;
} Floor;

// Represents the cylinders.
typedef struct CYLINDERS_STRUCT
{
	vector<double> x, y, z;	// x, y, z locations of the base center of the cylinders.
	double height,
		   radius;
	int numSlices,
		numStacks;
	bool enable;
} Cylinders;

typedef struct CUBE_STRUCT
{
	double style,
		size,
		rotateAngle,
		rx, ry, rz,
		tx, ty, tz;
	bool enable;
} Cube;


// Represents all the objects in the world.
typedef struct WORLD_STRUCT
{
	Frustum frustum;
	StarField starField;
	Floor floorObject;
	Cylinders cylinders;
	Grid gridLeft, gridRight;
	Cube cube;
	vector<double> sphereFieldPara;		// add special cirle patch of star field inside
} World;

// Defines a window that contains a wxGLCanvas to display OpenGL stuff.
class GLWindow : public wxFrame
{
private:
	GLPanel *m_glpanel;
	int m_clientX, m_clientY;
	Logger* m_logger;

public:
	GLWindow(const wxChar *title, int xpos, int ypos, int width, int height, World world , Logger* logger);

	// Returns a pointer to the embedded wxGLCanvas.
	GLPanel* GetGLPanel() const;
};


class GLPanel : public wxGLCanvas
{
private:
	World m_world;				// Holds everything we need for the OpenGL scene.
	GLfloat m_Heave,			//Holds everythin we need for the current rendered frame.
			m_Surge,
			m_Lateral;
	bool m_drawFlashSqureInCurrentFrame;
	Star *m_starArray;			// Holds all the vertices for the star field.
	int m_frameCount;
	nm3DDatum m_rotationVector;
	double m_rotationAngle,
		   m_centerX,
		   m_centerY,
		   m_centerZ;
	bool m_doRotation,
		 m_rotateFP;
	GLuint m_starFieldCallList,
		   m_floorCallList,
		   m_cylindersCallList,
		   m_textureLeftEyeCallList,
		   m_textureRightEyeCallList,
		   m_sphereFieldCallList;
	HGLRC m_threadContext;
	unsigned int texture_id;
	GLuint curveScreenTextureL, curveScreenTextureR;
	double sphereFieldTran[3];

	// for keyboard control - grid
	int prow, pcol;
	char key;
	double x_offset, y_offset;
	int whichGrid;
	Grid *alignmentGrid;

	//indicate if it is the first time in the program when trying to render something to the Oculus.
	bool firstTimeInLoop;

	//members for the ThreadLoop2 - for remember waht are the parameters in the last loop before the break between the rounds.
	int lastNumOfTriangles;
	float* lastTrianglesVertexrArray;
	ovrVector3f lastEyePos;
	ovrVector3f lastTargetPos;
	ovrVector3f lastUpPos;
	ovrVector3f lastCenterPoint;
	ovrVector3f lastFixationPoint;
	int lastZDistanceFromScreen;
	float lastNearZ;
	float lastFarZ;
	bool lastRecordAvailable = false;	//for flag if there was a last render already.
	//End of the members of the ThreadLoop2.

	Logger* m_logger;					//the program main logger.

public:
	GLPanel(wxWindow *parent, int width, int height, World world, int *attribList , Logger* logger);
	~GLPanel();

	void OnPaint(wxPaintEvent &event);
	void OnSize(wxSizeEvent &event);
	void SetHeave(GLdouble heave);
	void SetLateral(GLdouble lateral);
	void SetSurge(GLdouble surge);
	/**
	 * \Determins if to draw the flahing square at the current frame. 
	 * \drawFlashSquare - Wheter to draw or not. 
	 */
	void SetDrawFlashSquareAtCurrentFrame(bool drawFlashSquare);
	void SetSphereFieldTran(double x, double y, double z);
	// keyboard control
	void OnKeyboard(wxKeyEvent& event);

	// This is the main function that draws the OpenGL scene.
	GLvoid Render(ovrQuatf& quaternion = ovrQuatf());

	// Does any one time OpenGL initializations.
	GLvoid InitGL();

	// Gets a pointer to the world object.
	World* GetWorld();

	// Gets/Sets the frustum for the GL scene.
	Frustum* GetFrustum();
	GLvoid SetFrustum(Frustum frustum);

	// Gets/Sets the starfield data for the GL scene and recreates the
	// individual star information.
	StarField* GetStarField();
	GLvoid SetStarField(StarField starfield);

	// Sets the rotation vector.
	void SetRotationVector(nm3DDatum rotationVector);

	// Sets the rotation angle in degrees.
	void SetRotationAngle(double angle);

	// Sets whether or not we should do any rotation.
	void DoRotation(bool val);

	// Sets whether or not we rotate the fixation point.
	void RotateFixationPoint(bool val);

	// Sets the center of rotation.
	void SetRotationCenter(double x, double y, double z);

	// Draws all the stars into a call list.
	GLvoid SetupCallList(unsigned int objects);

	// Sets the thread's OpenGL context.
	GLvoid SetThreadContext(HGLRC context);

	// Updates the world objects.
	GLvoid UpdateWorld(unsigned int objects);

	// Curve screen texture mapping square size. (cm)
	double curve_screen_space;

	//Enable/Disable drawing grid
	double enableGrid;

	//ThreadLoop for keep rendering the last frame of the last round between the rounds - for no black screen at the pauses between the rounds.
	void ThreadLoopStartField()
	{
		//no need fore this quaternion
		ovrQuatf quaternion;
		ThreadLoop(lastNumOfTriangles,
			lastTrianglesVertexrArray,
			lastNumOfTriangles * 3,
			lastEyePos.x,
			lastEyePos.y,
			lastEyePos.z,
			lastTargetPos.x,
			lastTargetPos.y,
			lastTargetPos.z,
			lastUpPos.x,
			lastUpPos.y,
			lastUpPos.z,
			lastCenterPoint.x,
			lastCenterPoint.y,
			lastCenterPoint.z,
			lastFixationPoint.x,
			lastFixationPoint.y,
			lastFixationPoint.z,
			lastZDistanceFromScreen,
			lastNearZ,
			lastFarZ,
			quaternion,
			false,
			m_world.starField.drawFlashingFixationPoint);
	}

	//ThreadLoop for keep rendering the fixation point (can move by the head) between the rounds - for no black screen at the pauses between the rounds.
	void ThreadLoopFixationPoint()
	{
		//no need fore this quaternion
		ovrQuatf quaternion;
		ThreadLoop(0,
			NULL,
			0 * 3,
			lastEyePos.x,
			lastEyePos.y,
			lastEyePos.z,
			lastTargetPos.x,
			lastTargetPos.y,
			lastTargetPos.z,
			lastUpPos.x,
			lastUpPos.y,
			lastUpPos.z,
			lastCenterPoint.x,
			lastCenterPoint.y,
			lastCenterPoint.z,
			lastFixationPoint.x,
			lastFixationPoint.y,
			lastFixationPoint.z,
			lastZDistanceFromScreen,
			lastNearZ,
			lastFarZ,
			quaternion,
			false,
			m_world.starField.drawFlashingFixationPoint
			);
	}
	// matan - Thread Loop for displaying flashing square without background
	void ThreadLoopFlashingSquare()
	{
		//no need fore this quaternion
		ovrQuatf quaternion;
		ThreadLoop(0,
			NULL,
			0 * 3,
			lastEyePos.x,
			lastEyePos.y,
			lastEyePos.z,
			lastTargetPos.x,
			lastTargetPos.y,
			lastTargetPos.z,
			lastUpPos.x,
			lastUpPos.y,
			lastUpPos.z,
			lastCenterPoint.x,
			lastCenterPoint.y,
			lastCenterPoint.z,
			lastFixationPoint.x,
			lastFixationPoint.y,
			lastFixationPoint.z,
			lastZDistanceFromScreen,
			lastNearZ,
			lastFarZ,
			quaternion,
			false,
			m_drawFlashSqureInCurrentFrame
		);
	}

	//clearing the world of the starfield to be nothing (if between trials we want a cleared world).
	void ClearStarFieldData()
	{
		m_world.starField.totalStars = 0;
		m_world.starField.density = 0;
		starFieldVertex3D = NULL;
	}
	
private:
	// Calcultates the glFrustum for a stereo scene.
	GLvoid CalculateStereoFrustum(GLfloat screenWidth, GLfloat screenHeight, GLfloat camera2screenDist,
								  GLfloat clipNear, GLfloat clipFar, GLfloat eyeSeparation,
								  GLfloat centerOffsetX, GLfloat centerOffsetY);

	// Generates the starfield.
	GLvoid GenerateStarField();

	// Generates a circle patch in the starfield.
	GLvoid GenerateSphereField();

	// Generates the floor.
	GLvoid GenerateFloor();

	// Generates the cylinders.
	GLvoid GenerateCylinders();

	// Draws the generated starfield.
	GLvoid DrawStarField(ovrQuatf& quaternion = ovrQuatf() , float directionX = 0, float directionY = 0, float directionZ = 0,
		float targetPosX = 0, float targetPosY = 0, float targetPosZ = 0,
		float upPosX = 0, float upPosY = 0, float upPosZ = 0);

	// Draws the generated sphere field.
	GLvoid DrawSphereField();

	void ChangeTrianglesColor(ShaderManager::ShaderName shaderName);

	/***	Function : ThreadLoop - to render all the world.
	****	numOfTriangles - the num of triangles in the vertexArray pointer.
	****	vertexArray - the vertexes of the triangles to be rendered as the stars field.
	****	numOfVetexes - the num of the vertexex in the array (the numOfTriangles*3).
	****	direction - the point vector of the origin of the camera.
	****	targetPos - the point vector target to look at.\
	****	starsCenter - the point vector of the center of the star field(may be always (0,0,0) at the begin).
	****	fixationPoint - the fixation point of the camera (the one that is between the camera place and the camera target).
	****	zDistanceFromScreen - the distance of the camera from the screen.
	****	nearZ - the near clip plane.
	****	farZ - the far clip plane.
	****	drawFlashingSquare - the flashin square in the middle (where the fixation point is) for the prior flashing protocol.
	****/
	void ThreadLoop(int numOfTriangles, GLfloat* vertexArray, int numOfVetexes,
		float directionX , float directionY, float directionZ,
		float targetPosX, float targetPosY, float targetPosZ,
		float upPosX, float upPosY, float upPosZ,
		float starsCenterX, float starsCenterY, float starsCenterZ,
		float fixationPointX, float fixationPointY, float fixationPointZ,
		int zDistanceFromScreen,
		float nearZ,
		float farZ,
		ovrQuatf & resultQuaternion,
		bool drawStaticSensorCube,
		bool drawFlashingSquare);

	/***	Function : FirstConfig - to initialize the oculus and the vertex array buffer.
	****	vertexArray - the vertex array of the stars field.
	****	numOfVetexes - the num of the vertexex in the array (the numOfTriangles*3).
	****/
	void FirstConfig(GLfloat* vertexArray, int numOfVertexes);

	// Draws the floor.
	GLvoid DrawFloor();

	// Draws the cylinders.
	GLvoid DrawCylinders();

	// Draws the cylinders.
	GLvoid DrawGrid(int whichEye);

	// Used to alter star locations due to their lifetimes.
	GLvoid ModifyStarField();

	// Used to alter star locations in sphere field due to their lifetimes.
	GLvoid ModifySphereField();

	// Renders one eye's image.
	GLvoid DrawEyeImage(int whichEye, ovrQuatf& quaternion = ovrQuatf());

	// Really drawing starfield, floor,...
	GLvoid DrawEyeImageObject(int whichEye, ovrQuatf& quaternion = ovrQuatf() , bool rotate = true, bool drawStayFP = false);

	// Create An Empty Texture -- for curve screen
	GLuint EmptyTexture();

	// If we use curve screen, then we use texture method 
	// to re-alignment each patch on 2D screen             
	// This function is called when use alignment mode
	void TextureMappingGrid(int whichEye);

	// If we use curve screen mode, we use equation to mapping.
	void TextureMappingEq(int whichEye);

	// Drawing calibration stars
	void DrawCalibStars();

	// For test the angle between object motion and local optical flow of starfield.
	void DrawObjectPositon();

private:
	DECLARE_EVENT_TABLE()

public:
	//Indicated if there was something already rendered to the screen.
	bool FirstTimeInLoop(){ return firstTimeInLoop; }
	int GetLastNumOfTriangles(){ return lastNumOfTriangles; }
	void CopyLastCameraViewVectors()
	{

	}
	float* GetLastTrianglesVertexArray(){ return lastTrianglesVertexrArray; }
	void DrawCube(void);
	double CurveScreenCoord(double x, double y, char coord, int whichEye);
	int GetCounter(void);
	void SetGlStartData(double lateral, double surge, double heave, double rotationAngle);

	int drawingMode;
	GLfloat *starFieldVertex3D, *sphereFieldVertex3D;
	double enableStereo;
	double targetColor[3];
	int FPdrawingMode;
	double FPcrossLength;
	int FPcrossWidth;
	GLfloat glStartHeave,
			glStartSurge,
			glStartLateral,
			glStartRotationAngle;
	bool renderNow, // when we start the trajectories, we show up the background.
		clearBuffer, // Sometime we don't want to clear buffer, then we will know the object movement path.
		rotateView90; // Rotate View in countclockwise 90 degree

	enum
	{
		MODE_FLAT_SCREEN,
		MODE_CURVE_SCREEN,
		MODE_ALIGNMENT
	};
};

#include "GLWindow.inl"

#endif
