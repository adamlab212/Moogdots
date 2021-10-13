#include "GLWindow.h"

//global frameCounter
int frameCounter = 0;
extern OculusVR g_oculusVR;
extern Application g_application;
extern RenderContext g_renderContext;
int counter;
thread* oculusThreadLoop;


// Parameter list -- Original declaration can be found in ParameterList.cpp
extern CParameterList g_pList;


/****************************************************************************************/
/*	GLWindow Definitions ****************************************************************/
/****************************************************************************************/
GLWindow::GLWindow(const wxChar *title, int xpos, int ypos, int width, int height, World world , Logger* logger) :
wxFrame((wxFrame *)NULL, -1, title, wxPoint(xpos, ypos), wxSize(width, height), wxSIMPLE_BORDER), m_logger(logger)
{
	GetClientSize(&m_clientX, &m_clientY);

	// Setup the pixel format descriptor.
	int attribList[6];
	attribList[0] = WX_GL_STEREO;
	attribList[1] = WX_GL_DOUBLEBUFFER;
	attribList[2] = WX_GL_RGBA;
	attribList[3] = WX_GL_STENCIL_SIZE; attribList[4] = 8;
	attribList[5] = 0;

	// Create the embedded panel where all the OpenGL stuff will be shown.
	counter = 0;
	m_glpanel = new GLPanel(this, m_clientX, m_clientY, world, attribList , m_logger);
	
}


/****************************************************************************************/
/*	GLPanel Definitions *****************************************************************/
/****************************************************************************************/
BEGIN_EVENT_TABLE(GLPanel, wxGLCanvas)
EVT_PAINT(GLPanel::OnPaint)
EVT_SIZE(GLPanel::OnSize)
EVT_KEY_DOWN(GLPanel::OnKeyboard)
END_EVENT_TABLE()


GLPanel::GLPanel(wxWindow *parent, int width, int height, World world, int *attribList , Logger* logger) :
		 wxGLCanvas(parent, -1, wxPoint(0, 0), wxSize(width, height), 0, "GLCanvas", attribList),
		 m_Heave(0.0), m_Surge(0.0), m_Lateral(0.0), m_frameCount(1), m_logger(logger)
{
	WRITE_LOG(m_logger->m_logger, "GLPanel created....");

	// for moving keyboard
	prow=3; pcol=3; // point at prow and pcol
	key='p'; // for select point
	x_offset=1.0; y_offset=1.0;
	whichGrid = LEFT_EYE;

	renderNow = false;
	clearBuffer = true;

	m_world = world;

	// Create call lists to hold all of our world objects.
	m_starFieldCallList = glGenLists(6);
	m_floorCallList = m_starFieldCallList + 1;
	m_cylindersCallList = m_floorCallList + 1;
	m_textureLeftEyeCallList = m_cylindersCallList + 1;
	m_textureRightEyeCallList = m_textureLeftEyeCallList + 1;
	m_sphereFieldCallList = m_textureRightEyeCallList + 1;

	// Initialize the Star pointers.
	m_starArray = NULL;
	m_world.floorObject.vertices = NULL;
	starFieldVertex3D = NULL;
	sphereFieldVertex3D = NULL;

	InitGL();
	GenerateStarField();
	GenerateSphereField();
	GenerateFloor();
	SetupCallList(STARFIELD | FLOOR | CYLINDERS | TEXTURE | SPHEREFIELD);

	// Rotation defaults.
	m_rotationAngle = 0.0;
	m_rotationVector.x = 0.0; m_rotationVector.y = 0.0; m_rotationVector.z = 1.0;
	SetRotationCenter(0.0, 0.0, 0.0);
	m_doRotation = false;
	m_rotateFP = false;

	curve_screen_space = 10.0;
	enableGrid = 0.0;
	alignmentGrid = &m_world.gridLeft;
	drawingMode = MODE_FLAT_SCREEN;
	enableStereo = g_pList.GetVectorData("ENABLE_STEREO")[0];
	targetColor[0] = g_pList.GetVectorData("FP_COLOR")[0];
	targetColor[1] = g_pList.GetVectorData("FP_COLOR")[1];
	targetColor[2] = g_pList.GetVectorData("FP_COLOR")[2];
	sphereFieldTran[0] = sphereFieldTran[1] = sphereFieldTran[2] = 0.0;
	rotateView90 = g_pList.GetVectorData("ROTATE_VIEW_90").at(0) ? true : false;

	//for the FirstConfig cAll - to config the Oculus configurations.
	firstTimeInLoop = true;
}


GLPanel::~GLPanel()
{
	if (m_starArray != NULL) {
		delete [] m_starArray;
	}

	if (m_world.floorObject.vertices != NULL) {
		delete [] m_world.floorObject.vertices;
	}

	glDeleteTextures(1,&curveScreenTextureL);
	glDeleteTextures(1,&curveScreenTextureR);

	if(starFieldVertex3D != NULL){
		delete [] starFieldVertex3D;
	}

	if(sphereFieldVertex3D != NULL){
		delete [] sphereFieldVertex3D;
	}
	
}

GLvoid GLPanel::Render(ovrQuatf& quaternion)
{
	::InterlockedIncrement((long*) &frameCounter);

	WRITE_LOG_PARAM(m_logger->m_logger, "Rendering frame", frameCounter);

	// If star lifetime is up and we flagged the use of star lifetime, then modify some of
	// the stars.
	if (m_frameCount++ % m_world.starField.lifetime == 0 && m_world.starField.use_lifetime == 1.0) 
	{
		ModifyStarField();
	}

	if (m_frameCount++ % m_world.starField.objectLifetime == 0 && m_world.starField.use_objectLiftime == 1.0) 
	{
		ModifySphereField();
	}

	// Draw the left and right image.
	DrawEyeImage(LEFT_EYE , quaternion);
	if(drawingMode == MODE_CURVE_SCREEN) glCallList(m_textureLeftEyeCallList);
	else if(drawingMode == MODE_ALIGNMENT) TextureMappingGrid(LEFT_EYE);

	DrawEyeImage(RIGHT_EYE);
	if (drawingMode == MODE_CURVE_SCREEN)
	{
		glCallList(m_textureRightEyeCallList);
	}
	else if (drawingMode == MODE_ALIGNMENT){
		TextureMappingGrid(RIGHT_EYE);
	}

	glFlush();
}

GLvoid GLPanel::DrawEyeImage(int whichEye, ovrQuatf& quaternion)
{
	WRITE_LOG_PARAM(m_logger->m_logger, "Drawing eye image", whichEye);

	double eyePolarity = 1.0;

	if (whichEye == LEFT_EYE) 
	{
		if(this->enableStereo == 1.0)
		{
			glDrawBuffer(GL_BACK_LEFT);
		}
		else
		{
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
		eyePolarity = -1.0;
	}
	else
	{
		if(this->enableStereo == 1.0)
		{
			glDrawBuffer(GL_BACK_RIGHT);
		}
		else
		{
			glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
		}
	}

	if (clearBuffer || renderNow == false)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);		// Clears the current scene.
	}

	// Add by Johnny for cutout circle (8/21/07)
	// If we are using the cutout, we need to setup the stencil buffer.
	if (m_world.starField.useCutout == true) 
	{
		glPushMatrix();

		// Setup the projection matrix (IO_DIST=0.0).
		CalculateStereoFrustum(m_world.frustum.screenWidth, m_world.frustum.screenHeight, m_world.frustum.camera2screenDist,
							m_world.frustum.clipNear, m_world.frustum.clipFar, 0.0,
							m_world.frustum.worldOffsetX, m_world.frustum.worldOffsetZ);

		//glViewport(0,0,screenWidth, screenHeight);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Setup the camera (IO_DIST=0.0).
		if (m_world.starField.stayCutout == true) glPushMatrix();

		// for horizontal view
		if(rotateView90) glRotated(90.0, 0.0, 0.0, 1.0);

		gluLookAt(0.0+m_Lateral, 0.0f-m_Heave, m_world.frustum.camera2screenDist-m_Surge,		// Camera origin
				0.0+m_Lateral, 0.0f-m_Heave, m_world.frustum.camera2screenDist-m_Surge-1.0f,	// Camera direction
				0.0, 1.0, 0.0); // Which way is up

		WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Origin Cutout", m_Lateral);
		WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Origin Cutout", m_Heave);
		WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Origin Cutout", m_world.frustum.camera2screenDist - m_Surge);
		WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Direction Cutout", m_Lateral);
		WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Direction Cutout", -m_Heave);
		WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Direction Cutout", m_world.frustum.camera2screenDist - m_Surge - 1.0f);

		// Turn off polygon smoothing otherwise we get weird lines in the
		// triangle fan.
		glDisable(GL_POLYGON_SMOOTH);

		// Use 0 for clear stencil, enable stencil test
		glClearStencil(0);
		glEnable(GL_STENCIL_TEST);

		// All drawing commands fail the stencil test, and are not
		// drawn, but increment the value in the stencil buffer.
		glStencilFunc(GL_NEVER, 0x0, 0x0);
		glStencilOp(GL_INCR, GL_INCR, GL_INCR);

		// Draw a circle.
		glColor3d(1.0, 1.0, 1.0);
		glBegin(GL_TRIANGLE_FAN);
		for (double dAngle = 0; dAngle <= 360.0; dAngle += 2.0)
		{
			glVertex3d(m_world.starField.cutoutRadius * cos(dAngle*DEG2RAD) + m_world.starField.fixationPointLocation[0] + m_Lateral,
				m_world.starField.cutoutRadius * sin(dAngle*DEG2RAD) + m_world.starField.fixationPointLocation[1] - m_Heave,
				m_world.starField.fixationPointLocation[2] - m_Surge);
		}
		glEnd();

		if (m_world.starField.stayCutout == true)
		{
			glPopMatrix();

			// for horizontal view
			if(rotateView90) glRotated(90.0, 0.0, 0.0, 1.0);

			// image doesn't move in cutout circle
			gluLookAt(0.0+glStartLateral, 0.0f-glStartHeave, m_world.frustum.camera2screenDist-glStartSurge,		// Camera origin
			0.0+glStartLateral, 0.0f-glStartHeave, m_world.frustum.camera2screenDist-glStartSurge-1.0f,	// Camera direction
			0.0, 1.0, 0.0); // Which way is up
		}

		if(m_world.starField.drawCutout == true) 
		{
			// Now, allow drawing, where the stencil pattern is equal to 0x1
			glStencilFunc(GL_EQUAL, 0x1, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			// Turn smoothing back on to draw the star field.
			glEnable(GL_POLYGON_SMOOTH);

			// Draw eye image object with IO_DIST=0.0.
			if (m_world.starField.stayCutout == true){
				// image doesn't move in cutout circle
				DrawEyeImageObject(whichEye, quaternion , false, true);
			}
			else DrawEyeImageObject(whichEye, quaternion);
		}
		glPopMatrix();

		// Now, allow drawing, except where the stencil pattern is 0x1
		// and do not make any further changes to the stencil buffer
		glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		
		// Turn smoothing back on to draw the star field.
		glEnable(GL_POLYGON_SMOOTH);
	}
	else 
	{
		glDisable(GL_STENCIL_TEST);
	}
	
	// Setup the projection matrix.
	CalculateStereoFrustum(m_world.frustum.screenWidth, m_world.frustum.screenHeight, m_world.frustum.camera2screenDist,
						   m_world.frustum.clipNear, m_world.frustum.clipFar, static_cast<float>(eyePolarity)*m_world.frustum.eyeSeparation/2.0f,
						   m_world.frustum.worldOffsetX, m_world.frustum.worldOffsetZ);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(rotateView90)
	{// for horizontal view
		glPushMatrix();
		glRotated(90.0, 0.0, 0.0, 1.0);
	}

	// Setup the camera.
	gluLookAt(eyePolarity*m_world.frustum.eyeSeparation/2.0f+m_Lateral, 0.0f-m_Heave, m_world.frustum.camera2screenDist-m_Surge,		// Camera origin
			  eyePolarity*m_world.frustum.eyeSeparation/2.0f+m_Lateral, 0.0f-m_Heave, m_world.frustum.camera2screenDist-m_Surge-1.0f,	// Camera direction
			  0.0, 1.0, 0.0); // Which way is up

	WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Origin", eyePolarity*m_world.frustum.eyeSeparation / 2.0f + m_Lateral);
	WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Origin", -m_Heave);
	WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Origin", m_world.frustum.camera2screenDist - m_Surge);
	WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Direction", eyePolarity*m_world.frustum.eyeSeparation / 2.0f + m_Lateral);
	WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Direction", -m_Heave);
	WRITE_LOG_PARAM(m_logger->m_logger, "Looking at Camera Direction", m_world.frustum.camera2screenDist - m_Surge - 1.0f);
	
	DrawEyeImageObject(whichEye , quaternion);

	if (rotateView90)
	{
		glPopMatrix();
	}
}

GLvoid GLPanel::DrawEyeImageObject(const int whichEye, ovrQuatf& quaternion , const bool rotate, const bool drawStayFP)
{
	WRITE_LOG(m_logger->m_logger, "Drawing eye image object");

	//avi hanged due to the delay in rendering.
	if (whichEye == 1)
	{
		// If we don't want the fixation point rotated, go ahead and draw it at
		// a fixed position in front of the camera.
		if (!m_rotateFP) 
		{
			glDisable(GL_STENCIL_TEST);
			// Fixation point.
			if (m_world.starField.drawFixationPoint == 1.0) 
			{
				glColor3dv(targetColor);
				if (drawStayFP)
				{
					if (FPdrawingMode == 0)
					{ // drawing dot
						glPointSize(FP_DOTSIZE);
						glBegin(GL_POINTS);
						glVertex3d(m_world.starField.fixationPointLocation[0],
							m_world.starField.fixationPointLocation[1],
							m_world.starField.fixationPointLocation[2]);
						glEnd();
					}
					else if (FPdrawingMode == 1)
					{ // drawing cross
						glLineWidth(FPcrossWidth);
						glBegin(GL_LINES);
						// horizontal line
						glVertex3d(m_world.starField.fixationPointLocation[0] + FPcrossLength / 2,
							m_world.starField.fixationPointLocation[1],
							m_world.starField.fixationPointLocation[2]);
						glVertex3d(m_world.starField.fixationPointLocation[0] - FPcrossLength / 2,
							m_world.starField.fixationPointLocation[1],
							m_world.starField.fixationPointLocation[2]);
						// vertical line
						glVertex3d(m_world.starField.fixationPointLocation[0],
							m_world.starField.fixationPointLocation[1] + FPcrossLength / 2,
							m_world.starField.fixationPointLocation[2]);
						glVertex3d(m_world.starField.fixationPointLocation[0],
							m_world.starField.fixationPointLocation[1] - FPcrossLength / 2,
							m_world.starField.fixationPointLocation[2]);
						glEnd();
					}
				}
				else
				{
					if (FPdrawingMode == 0)
					{ // drawing dot
						glPointSize(FP_DOTSIZE);
						glBegin(GL_POINTS);
						glVertex3d(m_world.starField.fixationPointLocation[0] + m_Lateral,
							m_world.starField.fixationPointLocation[1] - m_Heave,
							m_world.starField.fixationPointLocation[2] - m_Surge);
						glEnd();
					}
					else if (FPdrawingMode == 1)
					{ // drawing cross
						glLineWidth(FPcrossWidth);
						glBegin(GL_LINES);
						// horizontal line
						glVertex3d(m_world.starField.fixationPointLocation[0] + m_Lateral + FPcrossLength / 2,
							m_world.starField.fixationPointLocation[1] - m_Heave,
							m_world.starField.fixationPointLocation[2] - m_Surge);
						glVertex3d(m_world.starField.fixationPointLocation[0] + m_Lateral - FPcrossLength / 2,
							m_world.starField.fixationPointLocation[1] - m_Heave,
							m_world.starField.fixationPointLocation[2] - m_Surge);
						// vertical line
						glVertex3d(m_world.starField.fixationPointLocation[0] + m_Lateral,
							m_world.starField.fixationPointLocation[1] - m_Heave + FPcrossLength / 2,
							m_world.starField.fixationPointLocation[2] - m_Surge);
						glVertex3d(m_world.starField.fixationPointLocation[0] + m_Lateral,
							m_world.starField.fixationPointLocation[1] - m_Heave - FPcrossLength / 2,
							m_world.starField.fixationPointLocation[2] - m_Surge);
						glEnd();
					}
				}
			}
			glEnable(GL_STENCIL_TEST);
		}

		// Target 1
		if (m_world.starField.drawTarget1 == 1.0) 
		{
			glColor3dv(targetColor);
			glBegin(GL_POINTS);
			glVertex3d(m_world.starField.targ1Location[0] + m_Lateral,
				m_world.starField.targ1Location[1] - m_Heave,
				m_world.starField.targ1Location[2] - m_Surge);
			glEnd();
		}

		// Target 2
		if (m_world.starField.drawTarget2 == 1.0) 
		{
			glColor3dv(targetColor);
			glBegin(GL_POINTS);
			glVertex3d(m_world.starField.targ2Location[0] + m_Lateral,
				m_world.starField.targ2Location[1] - m_Heave,
				m_world.starField.targ2Location[2] - m_Surge);
			glEnd();
		}

		if (rotate)
		{
			glTranslated(m_centerX, m_centerY, m_centerZ);
			glRotated(m_rotationAngle, m_rotationVector.x, m_rotationVector.y, m_rotationVector.z);
			glTranslated(-m_centerX, -m_centerY, -m_centerZ);
		}

		// Rotate the fixation point.  It will only be rotated if we're flagged to do a
		// rotation transformation.  Otherwise, it's just like the standard fixation point.
		if (m_rotateFP) 
		{
			glPointSize(FP_DOTSIZE);
			// Fixation point.
			if (m_world.starField.drawFixationPoint == 1.0) {
				glColor3dv(targetColor);
				glBegin(GL_POINTS);
				glVertex3d(m_world.starField.fixationPointLocation[0] + m_Lateral,
					m_world.starField.fixationPointLocation[1] - m_Heave,
					m_world.starField.fixationPointLocation[2] - m_Surge);
				glEnd();
			}
		}

		if (!clearBuffer)
		{
			glColor3d(0.0, 0.0, 1.0);
			DrawObjectPositon();
		}

		if (whichEye == LEFT_EYE) 
		{
			glColor3d(m_world.starField.starLeftColor[0] * m_world.starField.luminance,			// Red
				m_world.starField.starLeftColor[1] * m_world.starField.luminance,			// Green
				m_world.starField.starLeftColor[2] * m_world.starField.luminance);		// Blue
		}
		else 
		{
			glColor3d(m_world.starField.starRightColor[0] * m_world.starField.luminance,		// Red
				m_world.starField.starRightColor[1] * m_world.starField.luminance,		// Green
				m_world.starField.starRightColor[2] * m_world.starField.luminance);		// Blue
		}
		double eyePolarity = 1.0;
		// Draw the starfield.
		if (m_world.starField.drawBackground == 1.0 && renderNow) 
		{
			if (m_world.starField.use_lifetime == 1.0 && !drawStayFP)
				DrawStarField(quaternion ,
				/*eyePolarity*m_world.frustum.eyeSeparation / 2.0f + */m_Lateral, 0.0f - m_Heave, m_world.frustum.camera2screenDist - m_Surge,//direction
				/*eyePolarity*m_world.frustum.eyeSeparation / 2.0f + */m_Lateral, 0.0f - m_Heave, m_world.frustum.camera2screenDist - m_Surge - 1.0f,//right vector
				0.0, 1.0, 0.0//up vector
				);
			else glCallList(m_starFieldCallList);
			//glCallList(m_starFieldCallList);
			//DrawStarField();
		}

		//avi:added for the bug with the fixation point (and hour glass) during stimulus types which noe visual included.
		//m_world.starField.drawBackground says if to draw at this trial according to the stimulus type.
		else if (!m_world.starField.drawBackground == 1.0 && renderNow) 
		{
			if (m_world.starField.use_lifetime == 1.0 && !drawStayFP && lastRecordAvailable)
				ThreadLoop4();
		}

		if (m_world.sphereFieldPara.at(0) && renderNow) 
		{
			glTranslated(sphereFieldTran[0], sphereFieldTran[1], sphereFieldTran[2]);
			if (m_world.starField.use_objectLiftime == 1.0 && !drawStayFP)
				DrawSphereField();
			else glCallList(m_sphereFieldCallList);

			if (!clearBuffer)
			{
				glColor3d(0.0, 1.0, 1.0);
				DrawObjectPositon();
			}

			glTranslated(-sphereFieldTran[0], -sphereFieldTran[1], -sphereFieldTran[2]);
		}

		// Draw the floor.
		if (m_world.floorObject.enable) 
		{
			glCallList(m_floorCallList);
		}

		// Draw the cylinders.
		if (m_world.cylinders.enable) 
		{
			glCallList(m_cylindersCallList);
		}

		if (m_world.cube.enable)
		{
			DrawCube();
		}

		if (g_pList.GetVectorData("ENABLE_CALIB_STAR")[0] == 1.0)
		{
			DrawCalibStars();
		}

		glDisable(GL_STENCIL_TEST);
		//glFlush();
	}
}

void GLPanel::DrawObjectPositon()
{
	glPointSize(3.0);
	glBegin(GL_POINTS);
	glVertex3d(g_pList.GetVectorData("OBJECT_POS").at(0),
				g_pList.GetVectorData("OBJECT_POS").at(1),
				g_pList.GetVectorData("OBJECT_POS").at(2));
	glEnd();
}

// If we use curve screen, then we use texture method
// to re-alignment each patch on 2D screen         
// This function will use in alignment mode
void GLPanel::TextureMappingGrid(int whichEye)
{	
	glColor3d(0.0,0.0,0.0);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	if (whichEye == LEFT_EYE){
		glReadBuffer(GL_BACK_LEFT); //set current buffer
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		// using curveScreenTextureL texture object
		glBindTexture(GL_TEXTURE_2D,curveScreenTextureL); 
	}
	else{
		glReadBuffer(GL_BACK_RIGHT); //set current buffer
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		// using curveScreenTextureR texture object
		glBindTexture(GL_TEXTURE_2D,curveScreenTextureR);
	}

	// get screen width and high in pixel
	int screenWidth, screenHeight;
	GetClientSize(&screenWidth, &screenHeight);
	// read a rectangle of pixels from the framebuffer and uses it for a new texture.
	//glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, screenWidth, screenHeight, 0);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenWidth, screenHeight);


	if (whichEye == LEFT_EYE) {
//#if USE_STEREO
		if(this->enableStereo == 1.0){
			glDrawBuffer(GL_BACK_LEFT);
		}
//#else
		else{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
//#endif
		//eyePolarity = -1.0;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clears the current scene.
		//targetColor[0] = 1.0;
	}
	else {
//#if USE_STEREO
		if(this->enableStereo == 1.0){
			glDrawBuffer(GL_BACK_RIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clears the current scene.
		}
//#else
		else{
			glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
		}
//#endif
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clears the current scene.
		//targetColor[1] = 1.0;
	}

	// reset orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0,m_world.frustum.screenWidth,0.0, m_world.frustum.screenHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0,screenWidth, screenHeight); // in pixel
	
	float w=m_world.frustum.screenWidth, h=m_world.frustum.screenHeight; // in world coord.
	int row, col;
	Grid *grid;
	if (whichEye == LEFT_EYE) grid = &m_world.gridLeft;
	else grid = &m_world.gridRight;
	
	row=grid->GetRowNum();
	col=grid->GetColNum();
	int r=0, c=0;
	// mapping from all original quadrilateral to new quadrilateral coordinate
	glBegin(GL_QUADS);
	for( c=0; c<col; c++){ 
        for( r=0; r<row; r++){ 
			//(0,0)
			glTexCoord2f(grid->matrix[r][c].ox/w, grid->matrix[r][c].oy/h); 
			glVertex2f(grid->matrix[r][c].nx, grid->matrix[r][c].ny);

			//(0,1)
			glTexCoord2f(grid->matrix[r][c+1].ox/w, grid->matrix[r][c+1].oy/h); 
			glVertex2f(grid->matrix[r][c+1].nx, grid->matrix[r][c+1].ny);				

			//(1,1)
			glTexCoord2f(grid->matrix[r+1][c+1].ox/w, grid->matrix[r+1][c+1].oy/h); 
			glVertex2f(grid->matrix[r+1][c+1].nx, grid->matrix[r+1][c+1].ny);

			//(1,0)
			glTexCoord2f(grid->matrix[r+1][c].ox/w, grid->matrix[r+1][c].oy/h); 
			glVertex2f(grid->matrix[r+1][c].nx, grid->matrix[r+1][c].ny);
		}
	}
	glEnd();

	//glFlush ();
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	// Draw the grid
	if (enableGrid == 1.0){
		if(whichGrid == LEFT_EYE) DrawGrid(LEFT_EYE);
		else DrawGrid(RIGHT_EYE);
	}
	
}

// This function will help to create glCallList and use in curve screen mode
void GLPanel::TextureMappingEq(int whichEye)
{
	glColor3d(0.0,0.0,0.0);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	if (whichEye == LEFT_EYE){
		glReadBuffer(GL_BACK_LEFT); //set current buffer
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		// using curveScreenTextureL texture object
		glBindTexture(GL_TEXTURE_2D,curveScreenTextureL); 
	}
	else{
		glReadBuffer(GL_BACK_RIGHT); //set current buffer
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		// using curveScreenTextureR texture object
		glBindTexture(GL_TEXTURE_2D,curveScreenTextureR);
	}

	// get screen width and high in pixel
	int screenWidth, screenHeight;
	GetClientSize(&screenWidth, &screenHeight);
	// read a rectangle of pixels from the framebuffer and uses it for a new texture.
	//glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, screenWidth, screenHeight, 0);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenWidth, screenHeight);


	if (whichEye == LEFT_EYE) {
//#if USE_STEREO
		if(this->enableStereo == 1.0){
			glDrawBuffer(GL_BACK_LEFT);
		}
//#else
		else{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
//#endif
		//eyePolarity = -1.0;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clears the current scene.
		//targetColor[0] = 1.0;
	}
	else {
//#if USE_STEREO
		if(this->enableStereo == 1.0){
			glDrawBuffer(GL_BACK_RIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clears the current scene.
		}
//#else
		else{
			glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
		}
//#endif
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clears the current scene.
		//targetColor[1] = 1.0;
	}

	// re-setup orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0,m_world.frustum.screenWidth,0.0, m_world.frustum.screenHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0,screenWidth, screenHeight); // in pixel

	//glCallList(m_textureLeftEyeCallList);
	//DrawTextureMappingEq();

	float w=m_world.frustum.screenWidth, h=m_world.frustum.screenHeight; // in world coord.
	Grid grid = Grid();
	grid.space = curve_screen_space;
	grid.shiftDistance = 0.0;
	if (whichEye == LEFT_EYE) {
		grid.x_offset = m_world.gridLeft.x_offset;
		grid.y_offset = m_world.gridLeft.y_offset;
	}
	else{
		grid.x_offset = m_world.gridRight.x_offset;
		grid.y_offset = m_world.gridRight.y_offset;
	}
	grid.SetupMatrix();

	// use equation to setup mapping location
	int row=grid.GetRowNum();
	int col=grid.GetColNum();
	int r=0, c=0;
	for( c=0; c<=col; c++){ 
        for( r=0; r<=row; r++){ 
			grid.matrix[r][c].nx = CurveScreenCoord(grid.matrix[r][c].ox, grid.matrix[r][c].oy, 'x', whichEye);
			grid.matrix[r][c].ny = CurveScreenCoord(grid.matrix[r][c].ox, grid.matrix[r][c].oy, 'y', whichEye);
		}
	}

	// mapping from all original quadrilateral to new quadrilateral coordinate
	glBegin(GL_QUADS);
	for( c=0; c<col; c++){ 
        for( r=0; r<row; r++){ 
			//(0,0)
			glTexCoord2f(grid.matrix[r][c].ox/w, grid.matrix[r][c].oy/h); 
			glVertex2f(grid.matrix[r][c].nx, grid.matrix[r][c].ny);

			//(0,1)
			glTexCoord2f(grid.matrix[r][c+1].ox/w, grid.matrix[r][c+1].oy/h); 
			glVertex2f(grid.matrix[r][c+1].nx, grid.matrix[r][c+1].ny);

			//(1,1)
			glTexCoord2f(grid.matrix[r+1][c+1].ox/w, grid.matrix[r+1][c+1].oy/h); 
			glVertex2f(grid.matrix[r+1][c+1].nx, grid.matrix[r+1][c+1].ny);

			//(1,0)
			glTexCoord2f(grid.matrix[r+1][c].ox/w, grid.matrix[r+1][c].oy/h);
			glVertex2f(grid.matrix[r+1][c].nx, grid.matrix[r+1][c].ny);
		}
	}
	glEnd();

	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	//end of drawing texture

	// Draw the grid
	if (enableGrid == 1.0){
		glLineWidth(3.0);
		glColor3d(1.0,1.0,1.0);

		glBegin(GL_LINES);
		for(r=0; r<row; r++){
			for(c=0; c<=col; c++){
				glVertex2f(grid.matrix[r][c].nx, grid.matrix[r][c].ny);
				glVertex2f(grid.matrix[r+1][c].nx, grid.matrix[r+1][c].ny);
			}
		}
			
		for(c=0; c<col; c++){
			for(r=0; r<=row; r++){
				glVertex2f(grid.matrix[r][c].nx, grid.matrix[r][c].ny);
				glVertex2f(grid.matrix[r][c+1].nx, grid.matrix[r][c+1].ny);
			}
		}
		
		for(r=0; r<row; r++){
			for(c=0; c<col; c++){
				glVertex2f(grid.matrix[r][c].ox, grid.matrix[r][c].oy);
				glVertex2f(grid.matrix[r][c].nx, grid.matrix[r][c].ny);
			}
		}
		glEnd();

		glColor3d(0.0, 0.0, 0.0);
	}
}



void GLPanel::DoRotation(bool val)
{
	m_doRotation = val;
}


void GLPanel::SetRotationVector(nm3DDatum rotationVector)
{
	m_rotationVector = rotationVector;
}


void GLPanel::SetRotationAngle(double angle)
{
	m_rotationAngle = angle;
}


void GLPanel::SetRotationCenter(double x, double y, double z)
{
	m_centerX = x;
	m_centerY = y;
	m_centerZ = z;
}


GLvoid GLPanel::DrawCylinders()
{
	GLUquadricObj *quadObj = gluNewQuadric();

	for (int i = 0; i < static_cast<int>(m_world.cylinders.x.size()); i++) {
		glPushMatrix();
		glTranslated(m_world.cylinders.x.at(i), m_world.cylinders.y.at(i), m_world.cylinders.z.at(i));
		glRotated(-90.0, 1.0, 0.0, 0.0);
		gluCylinder(quadObj, m_world.cylinders.radius, m_world.cylinders.radius, m_world.cylinders.height,
				    m_world.cylinders.numSlices, m_world.cylinders.numStacks);
		glPopMatrix();
	}

	gluDeleteQuadric(quadObj);
}

GLvoid GLPanel::DrawGrid(int whichEye)
{
	Grid *grid;
	if(whichEye == LEFT_EYE) grid = &m_world.gridLeft;
	else grid = &m_world.gridRight;

	double space = grid->space;
	double x_offset = grid->x_offset;
	double y_offset = grid->y_offset;
	double screenWidth = grid->screenWidth;
	double screenHeight = grid->screenHeight;
	double i=0.0, j=0.0;	
	double length=0.0;
	
	glLineWidth(grid->lineWidth);
	glColor3d(1.0,1.0,1.0);

	int row = grid->GetRowNum();
	int col = grid->GetColNum();
	int r,c;

	
	glBegin(GL_LINES);
		for(r=0; r<row; r++){
			for(c=0; c<=col; c++){
				glVertex2f(grid->matrix[r][c].nx, grid->matrix[r][c].ny);
				glVertex2f(grid->matrix[r+1][c].nx, grid->matrix[r+1][c].ny);
			}
		}
			
		for(c=0; c<col; c++){
			for(r=0; r<=row; r++){
				glVertex2f(grid->matrix[r][c].nx, grid->matrix[r][c].ny);
				glVertex2f(grid->matrix[r][c+1].nx, grid->matrix[r][c+1].ny);
			}
		}
		
		for(r=0; r<row; r++){
			for(c=0; c<col; c++){
				glVertex2f(grid->matrix[r][c].ox, grid->matrix[r][c].oy);
				glVertex2f(grid->matrix[r][c].nx, grid->matrix[r][c].ny);
			}
		}
	glEnd();

	

	glPointSize(10.0);
	glColor3d(0.0, 0.0, 1.0);
	glBegin(GL_POINTS);
		//glVertex2f(m_world.grid.screenWidth/2.0, m_world.grid.screenHeight/2.0);
		glVertex2f(grid->matrix[prow][pcol].nx, grid->matrix[prow][pcol].ny);
	glEnd();

	glColor3d(0.0, 0.0, 0.0);

}


GLvoid GLPanel::DrawFloor()
{
	int i;

	// Make sure we don't access an unallocated array.
	if (m_world.floorObject.vertices == NULL)
	{
		return;
	}

	for (i = 0; i < m_world.floorObject.count; i++)
	{
		switch (m_world.floorObject.drawMode)
		{
		case 0: // Circles
			glBegin(GL_TRIANGLE_FAN);
			glVertex3d(m_world.floorObject.vertices[i].x[0], m_world.floorObject.vertices[i].y[0], m_world.floorObject.vertices[i].z[0]);
				for(double dAngle = 0; dAngle <= 360.0; dAngle += m_world.starField.starInc) {
					glVertex3d(m_world.floorObject.objectSize * cos(dAngle*DEG2RAD) + m_world.floorObject.vertices[i].x[0],
						m_world.floorObject.vertices[i].y[0],
						m_world.floorObject.objectSize * sin(dAngle*DEG2RAD) + m_world.floorObject.vertices[i].z[0]);
				}
			glEnd();
			break;

		case 1: // Squares.
			glBegin(GL_QUADS);
				// Bottom right corner.
				glVertex3d(m_world.floorObject.vertices[i].x[0] + m_world.floorObject.objectSize,
						m_world.floorObject.vertices[i].y[0],
						m_world.floorObject.vertices[i].z[0] + m_world.floorObject.objectSize);

				// Top right corner.
				glVertex3d(m_world.floorObject.vertices[i].x[0] + m_world.floorObject.objectSize,
						m_world.floorObject.vertices[i].y[0],
						m_world.floorObject.vertices[i].z[0] - m_world.floorObject.objectSize);

				// Top left corner.
				glVertex3d(m_world.floorObject.vertices[i].x[0] - m_world.floorObject.objectSize,
						m_world.floorObject.vertices[i].y[0],
						m_world.floorObject.vertices[i].z[0] - m_world.floorObject.objectSize);

				// Bottom left corner.
				glVertex3d(m_world.floorObject.vertices[i].x[0] - m_world.floorObject.objectSize,
						m_world.floorObject.vertices[i].y[0],
						m_world.floorObject.vertices[i].z[0] + m_world.floorObject.objectSize);
			glEnd();
		}
	}
}

GLvoid GLPanel::DrawStarField(ovrQuatf& quaternion , float directionX, float directionY, float directionZ,
								float targetPosX, float targetPosY, float targetPosZ,
								float upPosX, float upPosY, float upPosZ)
{
	WRITE_LOG(m_logger->m_logger, "Drawing star field");

	int i;

	// Don't try to mess with an unallocated array.
	if (m_starArray == NULL) 
	{
		return;
	}

	int j = 0;
	for (i = 0; i < m_world.starField.totalStars; i++) 
	{
		switch (m_world.starField.drawMode)
		{
			// Circles
		case 0:
			
			starFieldVertex3D[j++] = m_starArray[i].x[0]; starFieldVertex3D[j++] = m_starArray[i].y[0]; starFieldVertex3D[j++] = m_starArray[i].z[0];
			for(double dAngle = 0; dAngle <= 360.0; dAngle += m_world.starField.starInc)
			{
				starFieldVertex3D[j++] = m_world.starField.starRadius * cos(dAngle*DEG2RAD) + m_starArray[i].x[0];
				starFieldVertex3D[j++] = m_world.starField.starRadius * sin(dAngle*DEG2RAD) + m_starArray[i].y[0];
				starFieldVertex3D[j++] = m_starArray[i].z[0];
			}
			j = 0;
			break;

			// Triangles
		case 1:
			starFieldVertex3D[j++] = m_starArray[i].x[0]; starFieldVertex3D[j++] = m_starArray[i].y[0]; starFieldVertex3D[j++] = m_starArray[i].z[0];
			starFieldVertex3D[j++] = m_starArray[i].x[1]; starFieldVertex3D[j++] = m_starArray[i].y[1]; starFieldVertex3D[j++] = m_starArray[i].z[1];
			starFieldVertex3D[j++] = m_starArray[i].x[2]; starFieldVertex3D[j++] = m_starArray[i].y[2]; starFieldVertex3D[j++] = m_starArray[i].z[2];
			break;
		
		case 2:
			starFieldVertex3D[j++] = m_starArray[i].x[0]; starFieldVertex3D[j++] = m_starArray[i].y[0]; starFieldVertex3D[j++] = m_starArray[i].z[0];
			break;
		}
	}

	if (firstTimeInLoop && counter>1 )
	{
		FirstConfig(starFieldVertex3D, m_world.starField.totalStars * 3);
	}

	if (counter > 1)
	{
		ThreadLoop(m_world.starField.totalStars, starFieldVertex3D, m_world.starField.totalStars * 3,
			directionX, directionY, directionZ,
			targetPosX, targetPosY, targetPosZ,
			upPosX, upPosY, upPosZ,
			g_pList.GetVectorData("OBJECT_POS").at(0),
			g_pList.GetVectorData("OBJECT_POS").at(1),
			g_pList.GetVectorData("OBJECT_POS").at(2),
			m_world.starField.fixationPointLocation[0],
			m_world.starField.fixationPointLocation[1],
			m_world.starField.fixationPointLocation[2],
			m_world.frustum.camera2screenDist,
			g_pList.GetVectorData("CLIP_PLANES")[0],
			g_pList.GetVectorData("CLIP_PLANES")[1],
			quaternion,
			g_pList.GetVectorData("PHOTODIODE_ON").at(0),
			m_drawFlashSqureInCurrentFrame);
	}

	counter++;
}

void GLPanel::FirstConfig(GLfloat* vertexArray, int numOfVertexes)
{
	// initialize everything
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
	{
	}

	if (!g_oculusVR.InitVR())
	{
		SDL_Quit();
	}

	ovrSizei hmdResolution = g_oculusVR.GetResolution();
	ovrSizei windowSize = { hmdResolution.w / 2, hmdResolution.h / 2 };

	g_renderContext.Init("Oculus Rift Minimum OpenGL", 100, 100, windowSize.w, windowSize.h);
	SDL_ShowCursor(SDL_DISABLE);

	if (glewInit() != GLEW_OK)
	{
		g_oculusVR.DestroyVR();
		g_renderContext.Destroy();
		SDL_Quit();
	}

	if (!g_oculusVR.InitVRBuffers(windowSize.w, windowSize.h))
	{
		g_oculusVR.DestroyVR();
		g_renderContext.Destroy();
		SDL_Quit();
	}

	ShaderManager::GetInstance()->LoadShaders();

	//avi:
	ChangeTrianglesColor(ShaderManager::BasicShader);

	firstTimeInLoop = false;
	g_application.OnStart(vertexArray , numOfVertexes);
}

void GLPanel::ChangeTrianglesColor(ShaderManager::ShaderName shaderName)
{
	WRITE_LOG(m_logger->m_logger, "Changing triangles color");

	//avi:
	float trianglesColor[3];
	trianglesColor[0] = (float)(m_world.starField.starLeftColor[0] * m_world.starField.luminance);
	trianglesColor[1] = (float)(m_world.starField.starLeftColor[1] * m_world.starField.luminance);
	trianglesColor[2] = (float)(m_world.starField.starLeftColor[2] * m_world.starField.luminance);

	ShaderManager::GetInstance()->ShaderColor(trianglesColor, shaderName);
}

void GLPanel::ThreadLoop(int numOfTriangles, GLfloat* vertexArray, int numOfVertexes,
	float directionX, float directionY, float directionZ,
	float targetPosX, float targetPosY, float targetPosZ,
	float upPosX, float upPosY, float upPosZ,
	float starsCenterX, float starsCenterY, float starsCenterZ,
	float fixationPointX, float fixationPointY, float fixationPointZ,
	int zDistanceFromScreen,
	float nearZ,
	float farZ,
	ovrQuatf & resultQuaternion,
	bool drawStaticSensorCube,
	bool drawFlashingSquare)
{
	//Delete this log due to a lot of writing to the log file.
	/*WRITE_LOG(m_logger->m_logger, "Entering Thread Loop");
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop" , numOfTriangles);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", directionX);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", directionY);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", directionZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", targetPosX);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", targetPosY);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", targetPosZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", upPosX);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", upPosY);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", upPosZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", starsCenterX);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", starsCenterY);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", starsCenterZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", upPosX);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", upPosY);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", upPosZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", fixationPointX);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", fixationPointY);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", fixationPointZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", zDistanceFromScreen);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", nearZ);
	WRITE_LOG_PARAM(m_logger->m_logger, "ThreadLoop", farZ);*/

	int x = 20;
	// handle key presses
	processEvents();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	g_oculusVR.OnRenderStart();

	ovrVector3f eyePos;
	ovrVector3f targetPos;
	ovrVector3f upPos;

	for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
	{
		eyePos.x = directionX;
		eyePos.y = directionY;
		eyePos.z = directionZ;

		targetPos.x = targetPosX;
		targetPos.y = targetPosY;
		targetPos.z = targetPosZ;

		upPos.x = upPosX;
		upPos.y = upPosY;
		upPos.z = upPosZ;

		OVR::Matrix4f MVPMatrix = g_oculusVR.OnEyeRender(eyeIndex, eyePos, targetPos, upPos, nearZ, farZ);

		// update MVP in quad shader for the stars motion
		const ShaderProgram &shader = ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader);
		glUniformMatrix4fv(shader.uniforms[ModelViewProjectionMatrix], 1, GL_FALSE, &MVPMatrix.Transposed().M[0][0]);
		//avi:
		ChangeTrianglesColor(ShaderManager::BasicShader);

		//update MVP in quad shader for starts sphere object motion
		const ShaderProgram &shader2 = ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::FontShader);
		glUniformMatrix4fv(shader.uniforms[ModelViewProjectionMatrix], 1, GL_FALSE, &MVPMatrix.Transposed().M[0][0]);

		g_application.OnRender(numOfVertexes, vertexArray, numOfTriangles,
			directionX, directionY, directionZ,
			targetPosX, targetPosY, targetPosZ,
			upPosX, upPosY, upPosZ,
			starsCenterX, starsCenterY, starsCenterZ,
			fixationPointX, fixationPointY, fixationPointZ,
			m_world.starField.drawFixationPoint,
			eyeIndex,
			zDistanceFromScreen,
			drawStaticSensorCube,
			drawFlashingSquare);

		//eye oreirntation to give the Control function in the MogDootsCom the values of the heading eye's tracking which it should send to matlab.
		if (eyeIndex == 1)
			resultQuaternion = g_oculusVR.GetEyeOrientationQuaternion(1);

		if (m_world.sphereFieldPara.at(0) && renderNow)
		{
			OVR::Matrix4f mvpTranslateMatrix = g_oculusVR.OnTranslate(sphereFieldTran[0], sphereFieldTran[1], sphereFieldTran[2]);
			OVR::Matrix4f MVPMatrixSphereField = MVPMatrix * mvpTranslateMatrix;
			const ShaderProgram &shader = ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader);
			glUniformMatrix4fv(shader.uniforms[ModelViewProjectionMatrix], 1, GL_FALSE, &MVPMatrixSphereField.Transposed().M[0][0]);
			//glTranslated(sphereFieldTran[0], sphereFieldTran[1], sphereFieldTran[2]);
			if (m_world.starField.use_objectLiftime == 1.0)
			{
				DrawSphereField();
			}
			//glTranslated(-sphereFieldTran[0], -sphereFieldTran[1], -sphereFieldTran[2]);
		}

		g_oculusVR.OnEyeRenderFinish(eyeIndex);
	}

	g_oculusVR.SubmitFrame();
	g_oculusVR.BlitMirror();
	SDL_GL_SwapWindow(g_renderContext.window);
	x--;

	lastNumOfTriangles = numOfTriangles;
	lastTrianglesVertexrArray = vertexArray;
	lastEyePos = eyePos;
	lastTargetPos = targetPos;
	lastUpPos = upPos;
	lastCenterPoint.x = starsCenterX;
	lastCenterPoint.y = starsCenterY;
	lastCenterPoint.z = starsCenterZ;
	lastFixationPoint.x = fixationPointX;
	lastFixationPoint.y = fixationPointY;
	lastFixationPoint.z = fixationPointZ;
	lastZDistanceFromScreen = zDistanceFromScreen;
	lastNearZ = nearZ;
	lastFarZ = farZ;
	lastRecordAvailable = true;
}

GLvoid GLPanel::DrawSphereField()
{
	double radius = m_world.sphereFieldPara.at(1);
	int totalStars = 0;
	if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
		totalStars = (int)(4/3*PI*radius*radius*radius*m_world.sphereFieldPara.at(4));
	else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
		totalStars = (int) (PI*radius*radius*m_world.sphereFieldPara.at(4));

	if (m_world.sphereFieldPara.at(0) == 2) // points
	{
		g_application.OnRenderSphereField(sphereFieldVertex3D, totalStars * 3, m_world.sphereFieldPara.at(0), m_world.starField.starPointSize);
	}
	else // triangles (default)
	{
		g_application.OnRenderSphereField(sphereFieldVertex3D, totalStars * 3, m_world.sphereFieldPara.at(0));
	}
}

GLvoid GLPanel::ModifyStarField()
{
	WRITE_LOG(m_logger->m_logger, "Modifying star field");

	int i;
	double baseX, baseY, baseZ, prob,
		   sd0, sd1, sd2,
		   ts0, ts1;

	// Grab the starfield dimensions and triangle size.  Pulling this stuff out
	// of the vectors now produces a 20 fold increase in speed in the following
	// loop.
	sd0 = m_world.starField.dimensions[0];
	sd1 = m_world.starField.dimensions[1];
	sd2 = m_world.starField.dimensions[2];
	ts0 = m_world.starField.triangle_size[0];
	ts1 = m_world.starField.triangle_size[1];

	for (i = 0; i < m_world.starField.totalStars; i++) {
		// If a star is in our probability range, we'll modify it.
		prob = (double)rand()/(double)RAND_MAX*100.0;
		//double prob = 100.0;

		// If the coherence factor is higher than a random number chosen between
		// 0 and 100, then we don't do anything to the star.  This means that
		// (100-coherence)% of the total stars will change.
		if (m_world.starField.probability < prob) {
			// Find a random point to base our triangle around.
			baseX = (double)rand()/(double)RAND_MAX*sd0 - sd0/2.0;
			baseY = (double)rand()/(double)RAND_MAX*sd1 - sd1/2.0;
			baseZ = (double)rand()/(double)RAND_MAX*sd2 - sd2/2.0;

			// Vertex 1
			m_starArray[i].x[0] = baseX - ts0/2.0;
			m_starArray[i].y[0] = baseY - ts1/2.0;
			m_starArray[i].z[0] = baseZ;

			// Vertex 2
			m_starArray[i].x[1] = baseX;
			m_starArray[i].y[1] = baseY + ts1/2.0;
			m_starArray[i].z[1] = baseZ;

			// Vertex 3
			m_starArray[i].x[2] = baseX + ts0/2.0;
			m_starArray[i].y[2] = baseY - ts1/2.0;
			m_starArray[i].z[2] = baseZ;
		}
	}	

	//SetupCallList(STARFIELD);
}


GLvoid GLPanel::GenerateFloor()
{
	// Delete the old floor if needed.
	if (m_world.floorObject.vertices != NULL) {
		delete [] m_world.floorObject.vertices;
	}

	// Seed the random number generator.
	srand(static_cast<unsigned int>(time(NULL)));

	// Determine the total number of floor objects.
	m_world.floorObject.count = static_cast<int>(m_world.floorObject.dimensions[0]*m_world.floorObject.dimensions[1]*m_world.floorObject.density);

	// Allocate memory for the new floor vertices.
	m_world.floorObject.vertices = new Star[m_world.floorObject.count];

	// Generate the center point for all the objects.
	for (int i = 0; i < m_world.floorObject.count; i++) {
		// Object center.
		m_world.floorObject.vertices[i].x[0] = (double)rand()/(double)RAND_MAX*m_world.floorObject.dimensions[0] - m_world.floorObject.dimensions[0]/2.0 + m_world.floorObject.origin[0];
		m_world.floorObject.vertices[i].y[0] = m_world.floorObject.origin[1];
		m_world.floorObject.vertices[i].z[0] = (double)rand()/(double)RAND_MAX*m_world.floorObject.dimensions[1] - m_world.floorObject.dimensions[1]/2.0 + m_world.floorObject.origin[2];
	}
}


GLvoid GLPanel::GenerateCylinders()
{
}


GLvoid GLPanel::GenerateStarField()
{
	WRITE_LOG(m_logger->m_logger, "Generating starfield");

	int i;
	double baseX, baseY, baseZ,
		   sd0, sd1, sd2,   
		   ts0, ts1;

	// Delete the old Star array if needed.
	if (m_starArray != NULL) {
		delete [] m_starArray;
	}

	// Seed the random number generator.
	srand((unsigned int)time(NULL));

	// Determine the total number of stars needed to create an average density determined
	// from the StarField structure.
	m_world.starField.totalStars = (int)(m_world.starField.dimensions[0] * m_world.starField.dimensions[1] *
					               m_world.starField.dimensions[2] * m_world.starField.density);

	// Allocate the Star array.
	m_starArray = new Star[m_world.starField.totalStars];

	// Grab the starfield dimensions and triangle size.  Pulling this stuff out
	// of the vectors now produces a 20 fold increase in speed in the following
	// loop.
	sd0 = m_world.starField.dimensions[0];
	sd1 = m_world.starField.dimensions[1];
	sd2 = m_world.starField.dimensions[2];
	ts0 = m_world.starField.triangle_size[0];
	ts1 = m_world.starField.triangle_size[1];

	for (i = 0; i < m_world.starField.totalStars; i++) {
		// Find a random point to base our triangle around.
		baseX = (double)rand()/(double)RAND_MAX*sd0 - sd0/2.0;
		baseY = (double)rand()/(double)RAND_MAX*sd1 - sd1/2.0;
		baseZ = (double)rand()/(double)RAND_MAX*sd2 - sd2/2.0;

		// Vertex 1
		m_starArray[i].x[0] = baseX - ts0/2.0;
		m_starArray[i].y[0] = baseY - ts1/2.0;
		m_starArray[i].z[0] = baseZ;

		// Vertex 2
		m_starArray[i].x[1] = baseX;
		m_starArray[i].y[1] = baseY + ts1/2.0;
		m_starArray[i].z[1] = baseZ;

		// Vertex 3
		m_starArray[i].x[2] = baseX + ts0/2.0;
		m_starArray[i].y[2] = baseY - ts1/2.0;
		m_starArray[i].z[2] = baseZ;
	}

	// allocate memory for arrays; we use it on glDrawArrays to speed up drawing
	int size;
	if(m_world.starField.drawMode == 0) // circles
	{
		//size = ((int)(360/m_world.starField.starInc) + 2) * m_world.starField.totalStars*3;
		// use triangle draw circle. (number of triangle = 360/m_world.starField.starInc)
		size = ((int)(360/m_world.starField.starInc))*3*m_world.starField.totalStars*3;
	}
	else if(m_world.starField.drawMode == 1) // triangles
	{
		size = m_world.starField.totalStars*3*3;
	}
	else if(m_world.starField.drawMode == 2) // dots
	{
		size = m_world.starField.totalStars*3;
	}

	//avi: deallocate the memory , because it's being seleted only when the MoogDoots is being close.
	//so delete it each trial.
	if (starFieldVertex3D != NULL)
		delete[] starFieldVertex3D;
	starFieldVertex3D = new GLfloat[size];
}

GLvoid GLPanel::GenerateSphereField()
{
	int i;
	double baseX, baseY, baseZ, length,
		   ts0, ts1;

	// Delete the old sphere field array if needed.
	if (sphereFieldVertex3D != NULL) {
		delete [] sphereFieldVertex3D;
	}

	// Seed the random number generator.
	srand((unsigned int)time(NULL));

	// Determine the total number of stars in sphere field needed to create an average density determined
	// from the StarField structure (4/3*pi*r^3*density).
	double radius = m_world.sphereFieldPara.at(1);
	int totalStars = 0;
	if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
		totalStars = (int)(4/3*PI*radius*radius*radius*m_world.sphereFieldPara.at(4));
	else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
		totalStars = (int) (PI*radius*radius*m_world.sphereFieldPara.at(4));

	// Allocate the sphere field array (triangle has 3 points and each point has xyz coord).
	int size = totalStars*3*3; // triangle (default)
	if(m_world.starField.drawMode == 2) // dots
		size = totalStars*3;
	sphereFieldVertex3D = new GLfloat[size];

	// Grab the sphere field dimensions and triangle size.  
	ts0 = m_world.sphereFieldPara.at(2);
	ts1 = m_world.sphereFieldPara.at(3);

	int j = 0;
	for (i = 0; i < totalStars; i++) {
		// Find a random point to base our triangle around.
		baseX = (double)rand()/(double)RAND_MAX*radius*2 - radius;
		baseY = (double)rand()/(double)RAND_MAX*radius*2 - radius;
		baseZ = (double)rand()/(double)RAND_MAX*radius*2 - radius;
		if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
			length = baseX*baseX+baseY*baseY+baseZ*baseZ;
		else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
			length = baseX*baseX+baseY*baseY;
		 
		if ( length <= radius*radius){
			baseX += g_pList.GetVectorData("OBJECT_POS").at(0);
			baseY += g_pList.GetVectorData("OBJECT_POS").at(1);
			if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
				baseZ += g_pList.GetVectorData("OBJECT_POS").at(2);
			else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
				baseZ = g_pList.GetVectorData("OBJECT_POS").at(2);
		}
		else{
			i--;
			continue;
		}

		if(m_world.starField.drawMode == 2) // dots
		{
			sphereFieldVertex3D[j++] = baseX;
			sphereFieldVertex3D[j++] = baseY;
			sphereFieldVertex3D[j++] = baseZ;
		}
		else // triangles (default)
		{
			// Vertex 1
			sphereFieldVertex3D[j++] = baseX - ts0/2.0;
			sphereFieldVertex3D[j++] = baseY - ts1/2.0;
			sphereFieldVertex3D[j++] = baseZ;

			// Vertex 2
			sphereFieldVertex3D[j++] = baseX;
			sphereFieldVertex3D[j++] = baseY + ts1/2.0;
			sphereFieldVertex3D[j++] = baseZ;

			// Vertex 3
			sphereFieldVertex3D[j++] = baseX + ts0/2.0;
			sphereFieldVertex3D[j++] = baseY - ts1/2.0;
			sphereFieldVertex3D[j++] = baseZ;
		}
	}

	//setting the moving direction and speed
}

GLvoid GLPanel::ModifySphereField()
{
	int i;
	double baseX, baseY, baseZ, length,
		   ts0, ts1;
	double prob;
	double ox, oy, oz; 

	// Determine the total number of stars in sphere field needed to create an average density determined
	// from the StarField structure (4/3*pi*r^3*density).
	double radius = m_world.sphereFieldPara.at(1);
	int totalStars = 0;
	if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
		totalStars = (int)(4/3*PI*radius*radius*radius*m_world.sphereFieldPara.at(4));
	else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
		totalStars = (int) (PI*radius*radius*m_world.sphereFieldPara.at(4));

	// Grab the sphere field dimensions and triangle size.  
	ts0 = m_world.sphereFieldPara.at(2);
	ts1 = m_world.sphereFieldPara.at(3);

	// get star position of sphere field
	ox = g_pList.GetVectorData("OBJECT_POS").at(0);
	oy = g_pList.GetVectorData("OBJECT_POS").at(1);
	oz = g_pList.GetVectorData("OBJECT_POS").at(2);

	int j = 0;
	for (i = 0; i < totalStars; i++) {
		// If a star is in our probability range, we'll modify it.
		prob = (double)rand()/(double)RAND_MAX*100.0;
		
		// If the coherence factor is higher than a random number chosen between
		// 0 and 100, then we don't do anything to the star.  This means that
		// (100-coherence)% of the total stars will change.
		if (m_world.starField.objectProbability < prob) {
			// Find a random point to base our triangle around.
			baseX = (double)rand()/(double)RAND_MAX*radius*2 - radius;
			baseY = (double)rand()/(double)RAND_MAX*radius*2 - radius;
			baseZ = (double)rand()/(double)RAND_MAX*radius*2 - radius;
			if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
				length = baseX*baseX+baseY*baseY+baseZ*baseZ;
			else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
				length = baseX*baseX+baseY*baseY;
			 
			if ( length <= radius*radius){
				baseX += ox;
				baseY += oy;
				if(m_world.sphereFieldPara.at(0) == 1) // Sphere field
					baseZ += oz;
				else if(m_world.sphereFieldPara.at(0) == 2) // Circle patch
					baseZ = oz;
			}
			else{
				i--;
				continue;
			}

			// Vertex 1
			sphereFieldVertex3D[j++] = baseX - ts0/2.0;
			sphereFieldVertex3D[j++] = baseY - ts1/2.0;
			sphereFieldVertex3D[j++] = baseZ;

			// Vertex 2
			sphereFieldVertex3D[j++] = baseX;
			sphereFieldVertex3D[j++] = baseY + ts1/2.0;
			sphereFieldVertex3D[j++] = baseZ;

			// Vertex 3
			sphereFieldVertex3D[j++] = baseX + ts0/2.0;
			sphereFieldVertex3D[j++] = baseY - ts1/2.0;
			sphereFieldVertex3D[j++] = baseZ;
		}
		else j += 9;
	}

	//setting the moving direction and speed
}

GLvoid GLPanel::SetThreadContext(HGLRC context)
{
	m_threadContext = context;
}


GLvoid GLPanel::UpdateWorld(unsigned int objects)
{
	if (objects & STARFIELD) {
		GenerateStarField();
	}

	if (objects & FLOOR) {
		GenerateFloor();
	}

	if (objects & SPHEREFIELD) {
		GenerateSphereField();
	}

}


GLvoid GLPanel::SetupCallList(unsigned int objects)
{
	if (objects & STARFIELD) {
		// Make the call list for the star field.
		glNewList(m_starFieldCallList, GL_COMPILE);
		//avi : deleted caution - what is that?
		//DrawStarField();
		glEndList();
	}

	/*if (objects & SPHEREFIELD) {
		// Make the call list for the sphere field.
		glNewList(m_sphereFieldCallList, GL_COMPILE);
		DrawSphereField();
		glEndList();
	}

	if (objects & FLOOR) {
		// Make the call list for the floor.
		glNewList(m_floorCallList, GL_COMPILE);
		DrawFloor();
		glEndList();
	}

	if (objects & CYLINDERS) {
		// Make the call list for the cylinders.
		glNewList(m_cylindersCallList, GL_COMPILE);
		DrawCylinders();
		glEndList();
	}

	if (objects & TEXTURE) {
		// Make the call list for the texture of curve screen.
		glNewList(m_textureLeftEyeCallList, GL_COMPILE);
		TextureMappingEq(LEFT_EYE);
		glEndList();

		glNewList(m_textureRightEyeCallList, GL_COMPILE);
		TextureMappingEq(RIGHT_EYE);
		glEndList();
	}*/
}


GLvoid GLPanel::SetFrustum(Frustum frustum)
{
	m_world.frustum = frustum;
}


GLvoid GLPanel::SetStarField(StarField starfield)
{
	m_world.starField = starfield;

	// Regenerate the starfield based on new data.
	GenerateStarField();
}


GLvoid GLPanel::CalculateStereoFrustum(GLfloat screenWidth, GLfloat screenHeight, GLfloat camera2screenDist,
									   GLfloat clipNear, GLfloat clipFar, GLfloat eyeSeparation,
									   GLfloat centerOffsetX, GLfloat centerOffsetY)
{
	/*GLfloat left, right, top, bottom;

	// Go to projection mode.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// We use similar triangles to solve for the left, right, top, and bottom of the clipping
	// plane.
	if(rotateView90){ // put left eye down and right eye up
		top = (clipNear / camera2screenDist) * (screenHeight / 2.0f - eyeSeparation - centerOffsetY);
		bottom = (clipNear / camera2screenDist) * (-screenHeight / 2.0f - eyeSeparation - centerOffsetY);
		right = (clipNear / camera2screenDist) * (screenWidth / 2.0f  - centerOffsetX);
		left = (clipNear / camera2screenDist) * (-screenWidth / 2.0f  - centerOffsetX);
	}
	else{	
		top = (clipNear / camera2screenDist) * (screenHeight / 2.0f - centerOffsetY);
		bottom = (clipNear / camera2screenDist) * (-screenHeight / 2.0f - centerOffsetY);
		right = (clipNear / camera2screenDist) * (screenWidth / 2.0f - eyeSeparation - centerOffsetX);
		left = (clipNear / camera2screenDist) * (-screenWidth / 2.0f - eyeSeparation - centerOffsetX);
	}

	glFrustum(left, right, bottom, top, clipNear, clipFar);	*/
}


void GLPanel::OnSize(wxSizeEvent &event)
{
    // this is also necessary to update the context on some platforms
    wxGLCanvas::OnSize(event);

    // Set GL viewport (not called by wxGLCanvas::OnSize on all platforms...).
    int w, h;
    GetClientSize(&w, &h);
	if (GetContext())
    {
        SetCurrent();
        glViewport(0, 0, (GLint) w, (GLint) h);
    }
}


void GLPanel::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);

	// Make sure that we render the OpenGL stuff.
	SetCurrent();
	//this variable is not necessary at all.
	ovrQuatf quaternion;
	Render(quaternion);
	SwapBuffers();
}


void GLPanel::InitGL(void)
{
	//glClearColor(0.0, 0.0, 0.0, 0.0);					// Set background to black.
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glMatrixMode(GL_MODELVIEW);

//#if USE_STEREO
	if(this->enableStereo == 1.0){
		// Enable depth testing.
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}
//#endif

#if USE_ANTIALIASING
	// Enable Antialiasing
	//glEnable(GL_POINT_SMOOTH);
	//glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	//glEnable(GL_POLYGON_SMOOTH);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
#endif

#if WEIRD_MONITOR
	int w = 782, h = 897;  // New viewport settings.  JWN
	glViewport((SCREEN_WIDTH-w)/2, (SCREEN_HEIGHT-h)/2, w, h);
#endif

	/* Texture setting */
	curveScreenTextureL = EmptyTexture(); // Create Our Empty Texture
	curveScreenTextureR = EmptyTexture(); // Create Our Empty Texture
}

void GLPanel::RotateFixationPoint(bool val)
{
	m_rotateFP = val;
}

// Create An Empty Texture -- for curve screen
GLuint GLPanel::EmptyTexture()
{
	// get glWindow width and high
	int w, h;
    GetClientSize(&w, &h);

	GLuint txtnumber;											// Texture ID
	unsigned int* data;											// Stored Data

	// Create Storage Space For Texture Data 
	data = (unsigned int*)new GLuint[((w * h)* 4 * sizeof(unsigned int))];
	ZeroMemory(data,((w * h)* 4 * sizeof(unsigned int)));	// Clear Storage Memory

	//glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &txtnumber);								// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, txtnumber);					// Bind The Texture
	glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);						// Build Texture Using Information In data
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


	delete [] data;												// Release data

	return txtnumber;											// Return The Texture ID
}

void GLPanel::OnKeyboard(wxKeyEvent& event)
{
	if(drawingMode != MODE_ALIGNMENT) return;
	
	int k = event.GetKeyCode();
	char buffer[20];
	int answer;
	//answer = wxMessageBox(itoa( k, buffer, 10 ), "Confirm", wxYES_NO | wxCANCEL, NULL);

	Grid *grid;
	grid = alignmentGrid;

	double distance = grid->shiftDistance;
	bool setupMatrix = false;
	wxString s = "";
	double space = 0.0;

	switch ( k )
    {
		case 80: //p for point
			key = 'p'; //wxBell();
            break;
        case 77: //m for moving
			key = 'm';
            break;
		case 83: //s for change space in grid
			if(!event.ControlDown()) return;
			s = wxGetTextFromUser("Grid space", "Input size of space. It will reset everything!", _gcvt( grid->space, 10, buffer ), this);
			if (s == "") return; // if user click cancl
			space = atof(s);
			if( space <= 0.0 || space > grid->screenWidth || space > grid->screenHeight)
				wxMessageBox("Input a wrong number!", "Confirm", wxOK, this);
			else{
				grid->space = space;
				setupMatrix = true;
			}
			break;
		case 76: //Alt+l for select left grid
			if(!event.AltDown()) return;
				alignmentGrid = &m_world.gridLeft;
				whichGrid = LEFT_EYE;
				wxMessageBox("Left eye grid.", "Confirm", wxOK, this);
			break;
		case 82: //Alt+r for select right grid
			if(!event.AltDown()) return;
				alignmentGrid = &m_world.gridRight;
				whichGrid = RIGHT_EYE;
				wxMessageBox("Right eye grid.", "Confirm", wxOK, this);
			break;
		case 87: //w for lineWidth
			key = 'w';
			break;
        case 316: // move left
			if(key=='p' && pcol>0){
				pcol--;
			}
			else if (key=='m' && pcol!=grid->FPcol){
				grid->matrix[prow][pcol].nx-=distance;
			}
			/*
			else if (key=='s' && grid->x_offset>0){
				grid->x_offset-=distance;
				setupMatrix = true;
			}
			*/
            break;
		case 317: // move up
			if(key=='p' && prow<grid->GetRowNum()){
				prow++;
			}
			else if (key=='m'){
				grid->matrix[prow][pcol].ny+=distance;
				//m_world.gridRight.matrix[prow][pcol].ny+=distance;
				//m_world.gridLeft.matrix[prow][pcol].ny+=distance;
			}
			else if (key=='s' && grid->y_offset<grid->space){
				grid->y_offset+=distance;
				setupMatrix = true;
			}
			else if (key=='w' && grid->lineWidth<10){
				grid->lineWidth++;
			}
            break;
		case 318: // move right
			if(key=='p' && pcol<grid->GetColNum()){
				pcol++;
			}
			else if (key=='m' && pcol!=grid->FPcol){
				grid->matrix[prow][pcol].nx+=distance;
			}
			/*
			else if (key=='s' && grid->x_offset<grid->space){
				grid->x_offset+=distance;
				setupMatrix = true;
			}
			*/
            break;
		case 319: // move down
			if(key=='p' && prow>0){
				prow--;
			}
			else if (key=='m'){
				grid->matrix[prow][pcol].ny-=distance;
				//m_world.gridRight.matrix[prow][pcol].ny-=distance;
				//m_world.gridLeft.matrix[prow][pcol].ny-=distance;
			}
			else if (key=='s' && grid->y_offset>0){
				grid->y_offset-=distance;
				setupMatrix = true;
			}
			else if (key=='w' && grid->lineWidth>1.0){
				grid->lineWidth--;
			}
            break;
        case 306: //shift key for shift whole grid
			answer = wxMessageBox("Shift whole grid will RESET everything!!!", "Confirm", wxOK | wxCANCEL, this);
			if (answer == wxOK){
				key = 's';
				setupMatrix = true;
			}
            break;
		case 32: // space bar
			wxMessageBox("Manual:\n\nAlt+l: select left grid\nAlt+r: select right grid\np: select point\nm: moving selected point\nw: increase or decrease line width\nCtrl+s: change space of grid\nShift: shift whole grid\nspace bar: manual for alignment", "Confirm", wxOK, this);
			break;
		case 307: //Alt key
			break;
		case 308: //Ctrl key
			break;
        default:
			wxMessageBox("Manual:\n\nAlt+l: select left grid\nAlt+r: select right grid\np: select point\nm: moving selected point\nw: increase or decrease line width\nCtrl+s: change space of grid\nShift: shift whole grid\nspace bar: manual for alignment", "Confirm", wxOK, this);
			break;

    }

	// key=='s'
	if(setupMatrix) grid->SetupMatrix();
	Render();
	SwapBuffers();

}


void GLPanel::DrawCube(void)
{
	glPushMatrix();
		glTranslatef(m_world.cube.tx, m_world.cube.ty, m_world.cube.tz );
		glRotatef(m_world.cube.rotateAngle, m_world.cube.rx, m_world.cube.ry, m_world.cube.rz);
		if(m_world.cube.style == 0.0 ) glutWireCube(m_world.cube.size);
		else glutSolidCube(m_world.cube.size);
	glPopMatrix();
}

double GLPanel::CurveScreenCoord(double x, double y, char coord, int whichEye)
{
	double *c;
	Grid gr;

	if (whichEye == LEFT_EYE) gr = GetWorld()->gridLeft;
	else gr = GetWorld()->gridRight;

	if(coord=='x') c = &gr.cubicEqCoeff_X[0]; 
	else if(coord=='y') c = &gr.cubicEqCoeff_Y[0]; 

	double z = c[0] + c[1]*x + c[2]*y +
		c[3]*x*x + c[4]*x*y + c[5]*y*y +
		c[6]*x*x*x + c[7]*x*x*y + c[8]*x*y*y + c[9]*y*y*y ;

	if (coord=='x') return x+z;
	else if (coord=='y') return y+z;

	return 0.0;
}


int GLPanel::GetCounter(void)
{
	InterlockedExchange((long*)&frameCounter,0);
	int c = InterlockedExchange((long*)&frameCounter,0);
	while(c == 0){
		Sleep(16.667);
		c = InterlockedExchange((long*)&frameCounter,0);
	}
	Sleep(1000);
	return InterlockedExchange((long*)&frameCounter,0);
}

GLvoid GLPanel::DrawCalibStars()
{
	int i,j;
	double baseX, baseY, baseZ,
		   sd0, sd1, sd2,
		   ts0, ts1;
	double starLocation[3][3];

	// Grab the starfield dimensions and triangle size.  Pulling this stuff out
	// of the vectors now produces a 20 fold increase in speed in the following
	// loop.
	sd0 = m_world.starField.dimensions[0]; 
	sd1 = m_world.starField.dimensions[1]; 
	sd2 = m_world.starField.dimensions[2]; 
	ts0 = m_world.starField.triangle_size[0];
	ts1 = m_world.starField.triangle_size[1];

	double distY = g_pList.GetVectorData("ENABLE_CALIB_STAR")[2];
	double distZ = g_pList.GetVectorData("ENABLE_CALIB_STAR")[3];

	// drawing star at (0,0,0) position
	baseX = g_pList.GetVectorData("ENABLE_CALIB_STAR")[1]; 
	baseY = 0; baseZ = 0;

	// Vertex 1
	starLocation[0][0] = baseX - ts0/2.0;
	starLocation[0][1] = baseY - ts1/2.0;
	starLocation[0][2] = baseZ;

	// Vertex 2
	starLocation[1][0] = baseX;
	starLocation[1][1] = baseY + ts1/2.0;
	starLocation[1][2] = baseZ;

	// Vertex 3
	starLocation[2][0] = baseX + ts0/2.0;
	starLocation[2][1] = baseY - ts1/2.0;
	starLocation[2][2] = baseZ;

	glBegin(GL_TRIANGLES);
		glVertex3d(starLocation[0][0], starLocation[0][1], starLocation[0][2]);
		glVertex3d(starLocation[1][0], starLocation[1][1], starLocation[1][2]);
		glVertex3d(starLocation[2][0], starLocation[2][1], starLocation[2][2]);
	glEnd();

	for (i = 0, j=0; i <= sd0; i+=distZ, j+=distY) {
		baseY = j;
		baseZ = i;

		// Vertex 1
		starLocation[0][0] = baseX - ts0/2.0;
		starLocation[0][1] = baseY - ts1/2.0;
		starLocation[0][2] = baseZ;

		// Vertex 2
		starLocation[1][0] = baseX;
		starLocation[1][1] = baseY + ts1/2.0;
		starLocation[1][2] = baseZ;

		// Vertex 3
		starLocation[2][0] = baseX + ts0/2.0;
		starLocation[2][1] = baseY - ts1/2.0;
		starLocation[2][2] = baseZ;

		glBegin(GL_TRIANGLES);
			glVertex3d(starLocation[0][0], starLocation[0][1], starLocation[0][2]);
			glVertex3d(starLocation[1][0], starLocation[1][1], starLocation[1][2]);
			glVertex3d(starLocation[2][0], starLocation[2][1], starLocation[2][2]);
		glEnd();

		baseY = -j;
		baseZ = -i;

		// Vertex 1
		starLocation[0][0] = baseX - ts0/2.0;
		starLocation[0][1] = baseY - ts1/2.0;
		starLocation[0][2] = baseZ;

		// Vertex 2
		starLocation[1][0] = baseX;
		starLocation[1][1] = baseY + ts1/2.0;
		starLocation[1][2] = baseZ;

		// Vertex 3
		starLocation[2][0] = baseX + ts0/2.0;
		starLocation[2][1] = baseY - ts1/2.0;
		starLocation[2][2] = baseZ;

		glBegin(GL_TRIANGLES);
			glVertex3d(starLocation[0][0], starLocation[0][1], starLocation[0][2]);
			glVertex3d(starLocation[1][0], starLocation[1][1], starLocation[1][2]);
			glVertex3d(starLocation[2][0], starLocation[2][1], starLocation[2][2]);
		glEnd();

	}
}

void GLPanel::SetGlStartData(double lateral, double surge, double heave, double rotationAngle)
{
	glStartLateral = lateral;
	glStartSurge = surge;
	glStartHeave = heave;
	glStartRotationAngle = rotationAngle;
}

void GLPanel::SetSphereFieldTran(double x, double y, double z)
{
	sphereFieldTran[0] = x;
	sphereFieldTran[1] = y;
	sphereFieldTran[2] = z;
}
