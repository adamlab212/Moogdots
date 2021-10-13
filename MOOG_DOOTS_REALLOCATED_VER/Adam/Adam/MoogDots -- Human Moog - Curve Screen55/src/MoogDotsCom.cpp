#include "MoogDotsCom.h"


using namespace libxl;

extern OculusVR g_oculusVR;
extern RenderContext g_renderContext;
extern Application g_application;

// function pointer typdefs
typedef void (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int(*PFNWGLEXTGETSWAPINTERVALPROC) (void);

// declare functions
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT = NULL;

// Parameter list -- Original declaration can be found in ParameterList.cpp
extern CParameterList g_pList;

extern double FLUTE_C_SOUND[];
extern double REED_ORGAN_LOW_G[];
extern double FLUTE_HIGH_G[];
extern double FLUTE_LOW_G[];
extern double REED_ORGAN_C[];
extern double REED_ORGAN_HIGH_G[];


int startClk = 0;
int finishClk = 0;
#include <ctime>

MoogDotsCom::MoogDotsCom(char *mbcIP, int mbcPort, char *localIP, int localPort, Logger* logger, bool useCustomTimer, wxWindow* parent) :
	CORE_CONSTRUCTOR, m_glWindowExists(false), m_parentWindow(parent), m_isLibLoaded(false), m_messageConsole(NULL),
	m_tempoHandle(-1), m_listenMode(false), m_drawRegularFeedback(true), m_logger(logger),
	/* m_previousLateral(0.0), m_previousSurge(0.0), m_previousHeave(MOTION_BASE_CENTER), */
	m_previousBitLow(true)
{
	// Create the world object.
	World world;
	world.floorObject = createFloor();
	world.frustum = createFrustum();
	world.starField = createStarField();
	world.cylinders = createCylinders();
	world.cube = createCube();
	world.gridLeft.SetupMatrix();
	world.gridRight.SetupMatrix();
	world.sphereFieldPara = g_pList.GetVectorData("SPHERE_FIELD_PARAMS");


	// Create the OpenGL display window.
#if DUAL_MONITORS
#if FLIP_MONITORS
	m_glWindow = new GLWindow("GL Window", SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT, createFrustum(), createStarField());
#else
	m_glWindow = new GLWindow("GL Window", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, world, m_logger);

	//avioculus

	//avioculus
#endif
#else
	m_glWindow = new GLWindow("GL Window", 450, 100, 800, 870, world);
#endif

#if SHOW_GL_WINDOW
	m_glWindow->Show(true);
#endif

#if CUSTOM_TIMER
	m_doSyncPulse = false;
#endif;

	if (g_pList.GetVectorData("MOOG_CTRL_TIME").at(0)) UseMoogCtrlTimer(true);
	else UseMoogCtrlTimer(false);

	// Initialize the previous position data.
	m_previousPosition.heave = MOTION_BASE_CENTER; m_previousPosition.lateral = 0.0f;
	m_previousPosition.surge = 0.0f; m_previousPosition.roll = 0.0f;
	m_previousPosition.yaw = 0.0f; m_previousPosition.pitch = 0.0f;

	m_glWindowExists = true;
	m_verboseMode = false;
	m_continuousMode = false;
	m_reloadCallLists = false;

	QueryPerformanceFrequency(&m_freq);

	m_delay = 0.0;

	// Set the packet rate.
	SetPacketRate(16.594);

#if !CUSTOM_TIMER
	// Setup the vsync functions and, by default, turn off vsync.
	//InitVSync();
	//SetVSyncState(false);
#endif

	m_matlabRDX = NULL;
	/*m_matlabRDXHeave = NULL;
	m_matlabRDXLat = NULL;
	m_matlabRDXRoll = NULL;
	m_matlabRDXSurge = NULL;
	m_matlabRDXYaw = NULL;*/

	//GLPanel *g = m_glWindow->GetGLPanel();
	//g->curve_screen = g_pList.GetVectorData("CURVE_SCREEN_ON")[0];
	//g->curve_screen_space = g_pList.GetVectorData("CURVE_SCREEN_SPACE")[0];
	//g->enableGrid = g_pList.GetVectorData("ENABLE_GRID")[0];
	//g->SetupCallList(TEXTURE);

	//init the LPT controller for the EEG.
	m_EEGLptContoller = new LPTCOntroller();
	m_EEGLptContoller->Connect();

	redrawTexture = false;

	InitializeCriticalSection(&m_CS);
	InitializeCriticalSection(&m_TrialAborted);
	InitializeCriticalSection(&m_matlabInterpolation);

	WRITE_LOG(m_logger->m_logger, "MoogDotsCom created....");
}

double const MoogDotsCom::m_speedBuffer[SPEED_BUFFER_SIZE] =
{
	EXP_BASE, pow(EXP_BASE, 2), pow(EXP_BASE, 3), pow(EXP_BASE, 4), pow(EXP_BASE, 5),
	pow(EXP_BASE, 6), pow(EXP_BASE, 7), pow(EXP_BASE, 8), pow(EXP_BASE, 9), pow(EXP_BASE, 10),
	pow(EXP_BASE, 11), pow(EXP_BASE, 12), pow(EXP_BASE, 13), pow(EXP_BASE, 14), pow(EXP_BASE, 15),
	pow(EXP_BASE, 16), pow(EXP_BASE, 17), pow(EXP_BASE, 18), pow(EXP_BASE, 19), pow(EXP_BASE, 20),
	pow(EXP_BASE, 21), pow(EXP_BASE, 22), pow(EXP_BASE, 23), pow(EXP_BASE, 24), pow(EXP_BASE, 25),
	pow(EXP_BASE, 26), pow(EXP_BASE, 27), pow(EXP_BASE, 28), pow(EXP_BASE, 29), pow(EXP_BASE, 30)
	//pow(EXP_BASE, 31), pow(EXP_BASE, 32), pow(EXP_BASE, 33), pow(EXP_BASE, 34), pow(EXP_BASE, 35),
	//pow(EXP_BASE, 36), pow(EXP_BASE, 37), pow(EXP_BASE, 38), pow(EXP_BASE, 39), pow(EXP_BASE, 40),
	//pow(EXP_BASE, 41), pow(EXP_BASE, 42), pow(EXP_BASE, 43), pow(EXP_BASE, 44), pow(EXP_BASE, 45),
	//pow(EXP_BASE, 46), pow(EXP_BASE, 47), pow(EXP_BASE, 48), pow(EXP_BASE, 49), pow(EXP_BASE, 50),
	//pow(EXP_BASE, 51), pow(EXP_BASE, 52), pow(EXP_BASE, 53), pow(EXP_BASE, 54), pow(EXP_BASE, 55),
	//pow(EXP_BASE, 56), pow(EXP_BASE, 57), pow(EXP_BASE, 58), pow(EXP_BASE, 59), pow(EXP_BASE, 60)
};

double const MoogDotsCom::m_speedBuffer2[SPEED_BUFFER_SIZE] =
{
	0.9988, 0.9966, 0.9930, 0.9872, 0.9782, 0.9649, 0.9459, 0.9197,
	0.8852, 0.8415, 0.7882, 0.7257, 0.6554, 0.5792, 0.5000, 0.4208,

	446, 0.2743, 0.2118, 0.1585, 0.1148, 0.0803, 0.0541, 0.0351,
	0.0218, 0.0128, 0.0070, 0.0034, 0.0012, 0.0
};

MoogDotsCom::~MoogDotsCom()
{
	// reset portB
	cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 0);

	if (m_glWindowExists)
	{
		m_glWindow->Destroy();
	}

	// Deallocate memory used for analog scans.
	if (m_memHandle > 0)
	{
		cbWinBufFree(m_memHandle);
	}
}


void MoogDotsCom::ReloadCallLists(unsigned int objects)
{
	m_objects2change = objects;
	m_reloadCallLists = true;
}


#if !CUSTOM_TIMER
void MoogDotsCom::InitVSync()
{
	//get extensions of graphics card
	char* extensions = (char*)glGetString(GL_EXTENSIONS);

	// Is WGL_EXT_swap_control in the string? VSync switch possible?
	if (strstr(extensions, "WGL_EXT_swap_control"))
	{
		//get address's of both functions and save them
		wglSwapIntervalEXT = (PFNWGLEXTSWAPCONTROLPROC)
			wglGetProcAddress("wglSwapIntervalEXT");
		wglGetSwapIntervalEXT = (PFNWGLEXTGETSWAPINTERVALPROC)
			wglGetProcAddress("wglGetSwapIntervalEXT");
	}
}


void MoogDotsCom::SetVSyncState(bool enable)
{
	if (enable) {
		wglSwapIntervalEXT(1);
	}
	else {
		wglSwapIntervalEXT = 0;
	}
}


bool MoogDotsCom::VSyncEnabled()
{
	return (wglGetSwapIntervalEXT() > 0);
}
#endif


void MoogDotsCom::ListenMode(bool value)
{
	m_listenMode = value;
}


void MoogDotsCom::ShowGLWindow(bool value)
{
	if (m_glWindowExists) {
		m_glWindow->Show(value);
	}
}



void MoogDotsCom::InitTempo()
{
	int errCode;
	vector<wxString> errorStrings;

#if PCI_DIO_24H_PRESENT
	//m_PCI_DIO24_Object.DIO_board_num = m_PCI_DIO24_Object.GetBoardNum("pci-dio24/S");
	m_PCI_DIO24_Object.DIO_board_num = m_PCI_DIO24_Object.GetBoardNum("pci-dio48h");
#endif

#if USE_ANALOG_OUT_BOARD
	m_PCI_DIO48H_Object.DIO_board_num = m_PCI_DIO24_Object.GetBoardNum("pci-dda02/12");
	m_USB_3101FS_AO_Object.DIO_board_num = m_USB_3101FS_AO_Object.GetBoardNum("USB-3101FS");
#else
	m_PCI_DIO48H_Object.DIO_board_num = m_PCI_DIO24_Object.GetBoardNum("pci-dio48h");
#endif
	m_PCI_DIO48H_Object.DIO_base_address = m_PCI_DIO48H_Object.Get8255BaseAddr(m_PCI_DIO48H_Object.DIO_board_num, 1) + 4;
	m_USB_3101FS_AO_Object.DIO_base_address = m_USB_3101FS_AO_Object.Get8255BaseAddr(m_USB_3101FS_AO_Object.DIO_board_num, 1) + 4;

	//RDX is handled either via chip #1 on the pci-dio48h (rig #3) or via
	//an ISA version of the DIO24 board (base address set to 0x300
	//The logic here makes this code portable across different configurations
	if (m_PCI_DIO48H_Object.DIO_board_num == -1) { //board not found
		m_RDX_base_address = 0x300;  //default for the ISA board
	}
	else {
		m_RDX_base_address = m_PCI_DIO48H_Object.DIO_base_address;
	}

#if PCI_DIO_24H_PRESENT
	//Configure the PCI DIO board for digital input on the first port and output on second
	errCode = cbDConfigPort(m_PCI_DIO24_Object.DIO_board_num, FIRSTPORTA, DIGITALIN);
	if (errCode != 0) {
		errorStrings.push_back("Error setting up PCI-DIO24 port A!");
	}
	errCode = cbDConfigPort(m_PCI_DIO24_Object.DIO_board_num, FIRSTPORTB, DIGITALOUT);
	if (errCode != 0) {
		errorStrings.push_back("Error setting up PCI-DIO24 port B!");
	}
	// Initial B2, B1 and B0 are OFF, NO and OFF (010) = 2;
	cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 2);
#else
	// Configure the 48H board for ouput on the first chip, port B.
	errCode = cbDConfigPort(m_PCI_DIO48H_Object.DIO_board_num, FIRSTPORTB, DIGITALOUT);
	if (errCode != 0) {
		errorStrings.push_back("Error setting up PCI-DIO48H port B!");
	}

	// Configure the 48H board for input on the first chip, port A.
	errCode = cbDConfigPort(m_PCI_DIO48H_Object.DIO_board_num, FIRSTPORTA, DIGITALIN);
	if (errCode != 0) {
		errorStrings.push_back("Error setting up PCI-DIO48H port A!");
	}
#endif

#if USE_ANALOG_OUT_BOARD
	// Zero out PCI-DDA02/16 board.
	for (int i = 0; i<8; i++) cbAOut(m_PCI_DIO48H_Object.DIO_board_num, i, BIP10VOLTS, DASCALE(0));
#endif

	// Print info out to the message console.
	if (m_messageConsole != NULL) {
		m_messageConsole->Append(wxString::Format("Tempo Handle: %d", m_tempoHandle));
		m_messageConsole->Append("--------------------------------------------------------------------------------");
#if PCI_DIO_24H_PRESENT
		m_messageConsole->Append(wxString::Format("PCI_DIO24 board # = %d", m_PCI_DIO24_Object.DIO_board_num));
#endif
		m_messageConsole->Append(wxString::Format("PCI_DIO48H board # = %d", m_PCI_DIO48H_Object.DIO_board_num));
		m_messageConsole->Append(wxString::Format("PCI_DIO48H base address (chip 1) = 0x%04X",
			m_PCI_DIO48H_Object.DIO_base_address));
		m_messageConsole->Append(wxString::Format("RDX base address = 0x%04X", m_RDX_base_address));

		// Spit out any errors from setting up the digital ports.
		vector <wxString>::iterator iter;
		for (iter = errorStrings.begin(); iter != errorStrings.end(); iter++) {
			m_messageConsole->Append(*iter);
		}

		//m_messageConsole->Append("--------------------------------------------------------------------------------");
	}
}


void MoogDotsCom::SetConsolePointer(wxListBox *messageConsole)
{
	m_messageConsole = messageConsole;
}


void MoogDotsCom::SetVerbosity(bool value)
{
	m_verboseMode = value;
}


StarField MoogDotsCom::createStarField()
{
	WRITE_LOG(m_logger->m_logger, "Creating star field");

	// Create a StarField structure that describes the GL starfield.
	StarField s;
	s.dimensions = g_pList.GetVectorData("STAR_VOLUME");
	s.density = g_pList.GetVectorData("STAR_DENSITY")[0];
	s.triangle_size = g_pList.GetVectorData("STAR_SIZE");
	s.drawTarget = g_pList.GetVectorData("TARGET_ON")[0];
	s.drawFixationPoint = g_pList.GetVectorData("FP_ON")[0];
	s.drawFlashingFixationPoint = g_pList.GetVectorData("FP_FLASH_ON")[0];
	s.drawTarget1 = g_pList.GetVectorData("TARG1_ON")[0];
	s.drawTarget2 = g_pList.GetVectorData("TARG2_ON")[0];
	s.drawBackground = g_pList.GetVectorData("BACKGROUND_ON")[0];
	s.targetSize = g_pList.GetVectorData("TARGET_SIZE")[0];
	s.starLeftColor = g_pList.GetVectorData("STAR_LEYE_COLOR");
	s.starRightColor = g_pList.GetVectorData("STAR_REYE_COLOR");
	s.luminance = g_pList.GetVectorData("STAR_LUM_MULT")[0];
	s.lifetime = (int)g_pList.GetVectorData("STAR_LIFETIME")[0];
	s.objectLifetime = (int)g_pList.GetVectorData("OBJECT_LIFETIME")[0];
	s.probability = g_pList.GetVectorData("STAR_MOTION_COHERENCE")[0];
	s.objectProbability = g_pList.GetVectorData("OBJECT_MOTION_COHERENCE")[0];
	s.use_lifetime = g_pList.GetVectorData("STAR_LIFETIME_ON")[0];
	s.use_objectLiftime = g_pList.GetVectorData("OBJECT_LIFETIME_ON")[0];
	s.drawMode = (int)(g_pList.GetVectorData("DRAW_MODE").at(0));
	s.starRadius = g_pList.GetVectorData("STAR_RADIUS").at(0);
	s.starPointSize = g_pList.GetVectorData("STAR_POINT_SIZE").at(0);
	s.starInc = g_pList.GetVectorData("STAR_INC").at(0);
	s.useCutout = g_pList.GetVectorData("ENABLE_CUTOUT").at(0) ? true : false;
	s.drawCutout = g_pList.GetVectorData("ENABLE_CUTOUT").at(1) ? true : false;
	s.stayCutout = g_pList.GetVectorData("ENABLE_CUTOUT").at(2) ? true : false;
	s.cutoutRadius = g_pList.GetVectorData("CUTOUT_RADIUS").at(0);

	// Fixation point.
	vector<double> a;
	a.push_back(g_pList.GetVectorData("TARG_XCTR")[0]);
	a.push_back(g_pList.GetVectorData("TARG_YCTR")[0]);
	a.push_back(g_pList.GetVectorData("TARG_ZCTR")[0]);
	s.fixationPointLocation = a;

	// Target 1.
	vector<double> b;
	b.push_back(g_pList.GetVectorData("TARG_XCTR")[1]);
	b.push_back(g_pList.GetVectorData("TARG_YCTR")[1]);
	b.push_back(g_pList.GetVectorData("TARG_ZCTR")[1]);
	s.targ1Location = b;

	// Target  2.
	vector<double> c;
	c.push_back(g_pList.GetVectorData("TARG_XCTR")[2]);
	c.push_back(g_pList.GetVectorData("TARG_YCTR")[2]);
	c.push_back(g_pList.GetVectorData("TARG_ZCTR")[2]);
	s.targ2Location = c;

	//have to setup the totalStars
	s.totalStars = (int)(s.dimensions[0] * s.dimensions[1] * s.dimensions[2] * s.density);

	return s;
}


Cylinders MoogDotsCom::createCylinders()
{
	Cylinders c;

	c.enable = g_pList.GetVectorData("ENABLE_CYLINDERS").at(0) ? true : false;
	c.height = g_pList.GetVectorData("CYLINDER_HEIGHT").at(0);
	c.numSlices = g_pList.GetVectorData("CYLINDER_SLICES").at(0);
	c.numStacks = g_pList.GetVectorData("CYLINDER_STACKS").at(0);
	c.radius = g_pList.GetVectorData("CYLINDER_RADIUS").at(0);
	c.x = g_pList.GetVectorData("CYLINDERS_XPOS");

	// The y and z axes are flipped and negated between the OpenGL and
	// Moog axes.
	vector<double> z = g_pList.GetVectorData("CYLINDERS_ZPOS"),
		y = g_pList.GetVectorData("CYLINDERS_YPOS");
	for (int i = 0; i < static_cast<int>(z.size()); i++) {
		c.y.push_back(-z.at(i));
		c.z.push_back(-y.at(i));
	}

	return c;
}


Floor MoogDotsCom::createFloor()
{
	Floor f;

	f.density = g_pList.GetVectorData("FLOOR_DENSITY").at(0);
	for (int i = 0; i < 2; i++) {
		f.dimensions[i] = g_pList.GetVectorData("FLOOR_DIMS").at(i);
	}
	f.enable = g_pList.GetVectorData("ENABLE_FLOOR").at(0) ? true : false;
	f.objectSize = g_pList.GetVectorData("FLOOR_OBJ_SIZE").at(0);
	f.drawMode = static_cast<int>(g_pList.GetVectorData("FLOOR_DRAW_MODE").at(0));

	// For the origin, we need to swap the y and z because the Moog and OpenGL
	// have a different axes orientation.  y and z also need to be flipped.
	vector<double> o = g_pList.GetVectorData("FLOOR_ORIGIN");
	f.origin[0] = o.at(0);
	f.origin[1] = -o.at(2);
	f.origin[2] = -o.at(1);

	// have to setup count
	f.count = static_cast<int>(f.dimensions[0] * f.dimensions[1] * f.density);

	return f;
}

void MoogDotsCom::createGrid(Grid& gr)
{

	//gr.enable =	g_pList.GetVectorData("ENABLE_GRID").at(0) ? true : false;

	Grid grid;
	grid.screenWidth = g_pList.GetVectorData("SCREEN_DIMS")[0];
	grid.screenHeight = g_pList.GetVectorData("SCREEN_DIMS")[1];

	// set up new matrix if need
	if (!compareGrid(grid, gr)) {
		//gr.space = g_pList.GetVectorData("GRID_SETUP").at(1);
		//gr.x_offset = g_pList.GetVectorData("GRID_SETUP").at(2);
		//gr.y_offset = g_pList.GetVectorData("GRID_SETUP").at(3);
		gr.screenWidth = g_pList.GetVectorData("SCREEN_DIMS")[0];
		gr.screenHeight = g_pList.GetVectorData("SCREEN_DIMS")[1];
		gr.SetupMatrix();
	}

}



Frustum MoogDotsCom::createFrustum()
{
	WRITE_LOG(m_logger->m_logger, "Creating frustrum");

	vector<double> eyeOffsets = g_pList.GetVectorData("EYE_OFFSETS");
	vector<double> headCenter = g_pList.GetVectorData("HEAD_CENTER");

	eyeOffsets.at(2) += FP_ORIGIN_ADJUST;

	// Create a new Frustum structure that describes the GL space we'll be working with.
	Frustum f;
	f.screenWidth = g_pList.GetVectorData("SCREEN_DIMS")[0];					// Screen width
	f.screenHeight = g_pList.GetVectorData("SCREEN_DIMS")[1];					// Screen height
	f.clipNear = g_pList.GetVectorData("CLIP_PLANES")[0];						// Near clipping plane.
	f.clipFar = g_pList.GetVectorData("CLIP_PLANES")[1];						// Far clipping plane.
	f.eyeSeparation = g_pList.GetVectorData("IO_DIST")[0];						// Distance between eyes.
	f.camera2screenDist = CENTER2SCREEN - eyeOffsets.at(1) - headCenter.at(1);	// Distance from monkey to screen.
	f.worldOffsetX = eyeOffsets.at(0) + headCenter.at(0);						// Horizontal world offset.
	f.worldOffsetZ = eyeOffsets.at(2) + headCenter.at(2);						// Vertical world offset.

	return f;
}


void MoogDotsCom::UpdateGLScene(bool doSwapBuffers)
{
	WRITE_LOG(m_logger->m_logger, "Updating GL scene");

	// Make sure that the glWindow actually has been created before we start
	// messing with it.
	if (m_glWindowExists == false) {
		return;
	}

	bool drawTarget = true, drawBackground = true;
	GLPanel *g = m_glWindow->GetGLPanel();

	// Create new World objects based on the parameter list.
	Frustum f = createFrustum();
	StarField s = createStarField();
	Floor fl = createFloor();
	Cylinders c = createCylinders();
	Cube cube = createCube();
	vector<double> sphereFieldPara = g_pList.GetVectorData("SPHERE_FIELD_PARAMS");

	//g->curve_screen = g_pList.GetVectorData("CURVE_SCREEN_ON")[0];
	//g->curve_screen_space = g_pList.GetVectorData("CURVE_SCREEN_SPACE")[0];
	if (g->enableStereo != g_pList.GetVectorData("ENABLE_STEREO")[0]) {
		g->enableStereo = g_pList.GetVectorData("ENABLE_STEREO")[0];
		ThreadInit();
	}

	// Check to see if things changed such that I need to regenerate any
	// objects.
	bool worldChanged = false;
	unsigned int objects2change = 0x0;
	World *world = g->GetWorld();
	if (compareStarFields(s, world->starField) == false || newRandomStars == true) {
		objects2change |= STARFIELD;
		worldChanged = true;
		newRandomStars = false;
	}
	if (compareFloors(fl, world->floorObject) == false) {
		objects2change |= FLOOR;
		worldChanged = true;
	}
	if (compareCylinders(c, world->cylinders) == false) {
		objects2change |= CYLINDERS;
		worldChanged = true;
	}
	//for(int i=0; i<(int)world->sphereFieldPara.size(); i++){;
	//	if (world->sphereFieldPara.at(i) != sphereFieldPara.at(i)){
	objects2change |= SPHEREFIELD;
	worldChanged = true;
	//		break;
	//	}
	//}

	double space = g_pList.GetVectorData("CURVE_SCREEN_SPACE")[0];
	double enableGrid = g_pList.GetVectorData("ENABLE_GRID")[0];
	if (g->curve_screen_space != space || g->enableGrid != enableGrid) {
		g->curve_screen_space = space;
		g->enableGrid = g_pList.GetVectorData("ENABLE_GRID")[0];
		objects2change |= TEXTURE;
		worldChanged = true;
	}
	else if (redrawTexture) {
		objects2change |= TEXTURE;
		worldChanged = true;
		redrawTexture = false;
	}

	// Copy over the newly generated world objects to enact any changes
	// to the parameter list.
	world->starField = s;
	world->frustum = f;
	world->cylinders = c;
	world->cube = cube;
	world->sphereFieldPara = sphereFieldPara;
	createGrid(world->gridLeft);
	createGrid(world->gridRight);

	// Make sure we don't trash the vertices pointer.  Otherwise
	// the dynamically allocated pointer won't be cleaned up later.
	fl.vertices = world->floorObject.vertices;
	world->floorObject = fl;

	if (worldChanged) {
		g->UpdateWorld(objects2change);		// Regenerate any vertices.
		g->SetupCallList(objects2change);	// Draws the lists.
	}

	// setup the color for FP and 2 targets
	g->targetColor[0] = g_pList.GetVectorData("FP_COLOR")[0];
	g->targetColor[1] = g_pList.GetVectorData("FP_COLOR")[1];
	g->targetColor[2] = g_pList.GetVectorData("FP_COLOR")[2];

	// Drawing FP dots or cross
	g->FPdrawingMode = (int)g_pList.GetVectorData("FP_DRAW_MODE").at(0);
	g->FPcrossLength = g_pList.GetVectorData("FP_CROSS_LW").at(0);
	g->FPcrossWidth = (int)g_pList.GetVectorData("FP_CROSS_LW").at(1);

	g->clearBuffer = g_pList.GetVectorData("BUFFER_CLEAR").at(0) ? true : false;
	g->rotateView90 = g_pList.GetVectorData("ROTATE_VIEW_90").at(0) ? true : false;

	//avi : warning - that was deleted because it renders at the time of receiving data.
	// Re-Render the scene.
	//g->Render();
	//glFlush();

	if (doSwapBuffers) {
		// If we're in the main thread then call the GLPanel's SwapBuffers() function
		// because that function references the context made in the main thread.
		// Otherwise, we need to swap the buffers based on the context made in the
		// communications thread.
		if (wxThread::IsMain() == true) {
			g->SwapBuffers();

			if (worldChanged) {
				ReloadCallLists(objects2change);
			}
		}
		else {
			HDC hdc = static_cast<HDC>(g->GetContext()->GetHDC());
			//wglMakeCurrent(hdc, m_threadGLContext);
			SwapBuffers(hdc);
		}
	}

	if (g_pList.GetVectorData("MOOG_CTRL_TIME").at(0)) UseMoogCtrlTimer(true);
	else UseMoogCtrlTimer(false);
}


bool MoogDotsCom::compareFrustums(Frustum a, Frustum b) const
{
	// Compare every element in the two Frustums.
	bool equalFrustums = a.camera2screenDist == b.camera2screenDist &&
		a.clipNear == b.clipNear &&
		a.clipFar == b.clipFar &&
		a.eyeSeparation == b.eyeSeparation &&
		a.screenHeight == b.screenHeight &&
		a.screenWidth == b.screenWidth &&
		a.worldOffsetX == b.worldOffsetX &&
		a.worldOffsetZ == b.worldOffsetZ;

	return equalFrustums;
}

bool MoogDotsCom::compareStarFields(StarField a, StarField b) const
{
	bool equalStarFields;

	// Compare every element in the two StarFields.
	equalStarFields = a.density == b.density &&
		a.dimensions == b.dimensions &&
		a.triangle_size == b.triangle_size &&
		a.drawMode == b.drawMode &&
		a.starRadius == b.starRadius &&
		a.starPointSize == b.starPointSize &&
		a.starInc == b.starInc;

	return equalStarFields;
}

bool MoogDotsCom::compareFloors(Floor a, Floor b) const
{
	bool equalFloors;

	equalFloors = a.density == b.density &&
		a.dimensions[0] == b.dimensions[0] &&
		a.dimensions[1] == b.dimensions[1] &&
		a.objectSize == b.objectSize &&
		a.origin[0] == b.origin[0] &&
		a.origin[1] == b.origin[1] &&
		a.origin[2] == b.origin[2] &&
		a.drawMode == b.drawMode;

	return equalFloors;
}

bool MoogDotsCom::compareCylinders(Cylinders a, Cylinders b) const
{
	bool equalCylinders, equalVertices = true;;

	equalCylinders = a.height == b.height &&
		a.numSlices == b.numSlices &&
		a.numStacks == b.numStacks &&
		a.radius == b.radius;

	// Now make sure the vertices are the same.
	if (static_cast<int>(a.x.size()) == static_cast<int>(b.x.size())) {
		for (int i = 0; i < static_cast<int>(a.x.size()); i++) {
			if (a.x.at(i) != b.x.at(i) || a.y.at(i) != b.y.at(i) || a.z.at(i) != b.z.at(i)) {
				equalVertices = false;
				break;
			}
		}
	}
	else {
		equalVertices = false;
	}

	return equalCylinders && equalVertices;
}

bool MoogDotsCom::compareGrid(Grid a, Grid b) const
{
	bool equalGrid;

	equalGrid = //a.space == b.space &&
				//a.lineWidth == b.lineWidth &&
				//a.x_offset == b.x_offset &&
				//a.y_offset == b.y_offset &&
		a.screenWidth == b.screenWidth &&
		a.screenHeight == b.screenHeight;

	return equalGrid;
}


void MoogDotsCom::Sync()
{
	// Sync to a SwapBuffers() call.
	SetVSyncState(true);
	//wglMakeCurrent((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC(), m_threadGLContext);
	SwapBuffers((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC());
	SetVSyncState(false);
	Delay(m_delay);
}


void MoogDotsCom::ThreadInit(void)
{
	// Setup the rendering context for the thread.  Every thread has to
	// have its own rendering context.
	if (m_glWindowExists) {
		m_threadGLContext = wglCreateContext((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC());

		// Make sure that we got a valid handle.
		if (m_threadGLContext == NULL) {
			wxMessageDialog d(NULL, "ThreadInit: Couldn't create GL Context.", "GL Error");
			d.ShowModal();
		}

		/*if (wglMakeCurrent((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC(), m_threadGLContext) == FALSE) {
		wxMessageDialog d(NULL, "ThreadInit: Couldn't MakeCurrent.", "GL ERROR");
		d.ShowModal();
		}*/

		m_glWindow->GetGLPanel()->SetThreadContext(m_threadGLContext);

		// Initialize the GL Session.
		m_glWindow->GetGLPanel()->InitGL();

		// Setup which objects need to be rendered.
		unsigned int objects2render = 0x0;
		World *w = m_glWindow->GetGLPanel()->GetWorld();
		if (w->starField.drawBackground) {
			objects2render |= STARFIELD;
		}
		if (w->floorObject.enable) {
			objects2render |= FLOOR;
		}
		if (w->cylinders.enable) {
			objects2render |= CYLINDERS;
		}
		if (m_glWindow->GetGLPanel()->drawingMode == GLPanel::MODE_CURVE_SCREEN) {
			objects2render |= TEXTURE;
		}
		if (w->sphereFieldPara.at(0)) {
			objects2render |= SPHEREFIELD;
		}
		// Setup the call list within the thread.
		m_glWindow->GetGLPanel()->SetupCallList(objects2render);
	}

	m_data.index = 0;
	m_recordOffset = 0;
	m_recordIndex = 0;

	// Initialize the variables that record previous Analog input values.
	m_previousAnalogPosition = 0.0;
	m_previousAnalogVelocity = 0.0;

	// Create the Matlab RDX communication object if it doesn't exist.
	if (m_matlabRDX == NULL) {
		m_matlabRDX = new CMatlabRDX(m_PCI_DIO48H_Object.DIO_board_num);
	}
	m_matlabRDX->InitClient(FIRSTPORTB, FIRSTPORTA, SECONDPORTA);
}


#if USE_MATLAB | USE_MATLAB_INTERPOLATION

void MoogDotsCom::StartMatlab()
{
	WRITE_LOG(m_logger->m_logger, "Starting Matlab");

	//m_engine = engOpen(NULL);
	if (!(m_engine = engOpen(NULL))) {
		MessageBox((HWND)NULL, (LPSTR)"Can't start MATLAB engine",
			(LPSTR) "Warning", MB_OK);
	}

	/*
	avi : matlab interpolation version:
	This will allow all the thread (the other threads) to connect to this particular matlab engine even if they have a pointer to others engine.
	It should be said that eacj thred should have it's own matlab engime (pointer) for using the matlab , ,but because rhis line , they will use the same engine either
	that the pointer of each thread is for different matlab engine (but should to is thread safe).
	*/
	engEvalString(m_engine, "enableservice('AutomationServer' , true)");
}

void MoogDotsCom::PlotTrajectoryGraph()
{
#if USE_MATLAB_DEBUG_GRAPHS //avi : disable this because it makes the hourglass to appear at that time.
	if (m_debugFrameTime.size() > 0)
	{
		int minLength = m_debugPlaceTime.size();
		Engine* m_engine2 = engOpen(NULL);
		int x = engEvalString(m_engine2, "format long");

		mxArray* debugPlacemxArray = mxCreateDoubleMatrix(minLength, 1, mxREAL);
		memcpy((void*)mxGetPr(debugPlacemxArray), (void*)&m_debugPlace.at(0), (int)(sizeof(double) * minLength));
		engPutVariable(m_engine2, "debugPlacemxArray", debugPlacemxArray);

		mxArray* debugPlaceTimemxArray = mxCreateDoubleMatrix(minLength, 1, mxREAL);
		memcpy((void*)mxGetPr(debugPlaceTimemxArray), (void*)&m_debugPlaceTime.at(0), (int)(sizeof(double) * minLength));
		engPutVariable(m_engine2, "debugPlaceTimemxArray", debugPlaceTimemxArray);

		m_glWindow->GetGLPanel()->ThreadLoop3();

		minLength = m_debugFramePlace.size();
		mxArray* debugFrameTimemxArray = mxCreateDoubleMatrix(minLength, 1, mxREAL);
		memcpy((void*)mxGetPr(debugFrameTimemxArray), (void*)&m_debugFrameTime.at(0), sizeof(double) * minLength);
		engPutVariable(m_engine2, "debugFrameTimemxArray", debugFrameTimemxArray);

		mxArray* debugFramePlacemxArray = mxCreateDoubleMatrix(minLength, 1, mxREAL);
		memcpy((void*)mxGetPr(debugFramePlacemxArray), (void*)&m_debugFramePlace.at(0), sizeof(double) * minLength);
		engPutVariable(m_engine2, "debugFramePlacemxArray", debugFramePlacemxArray);

		engEvalString(m_engine2, "figure");
		engEvalString(m_engine2, "plot(debugFrameTimemxArray , debugFramePlacemxArray , 'color' , 'r'); hold on");
		engEvalString(m_engine2, "plot(debugPlaceTimemxArray , debugPlacemxArray , 'color' , 'b')");
		engEvalString(m_engine2, "title('(framePlaceVSmbcPlace , time')");

		m_debugFrameTime.clear();
		m_debugPlace.clear();
		m_debugPlaceTime.clear();
		m_debugFramePlace.clear();
	}
#endif//USE_MATLAB_DEBUG_GRAPHS
}

void MoogDotsCom::CloseMatlab()
{
	engClose(m_engine);
}

#if USE_MATLAB
void MoogDotsCom::StuffMatlab()
{
	stuffDoubleVector(m_sendStamp, "sendTimes");
	stuffDoubleVector(m_recordedHeave, "rHeave");
	stuffDoubleVector(m_recordedLateral, "rLateral");
	stuffDoubleVector(m_recordedSurge, "rSurge");
	stuffDoubleVector(m_recordedYaw, "rYaw");
	stuffDoubleVector(m_recordedPitch, "rPitch");
	stuffDoubleVector(m_recordedRoll, "rRoll");
	stuffDoubleVector(m_receiveStamp, "receiveTimes");

	// Stuff the command data into Matlab.
	stuffDoubleVector(m_data.X, "dataX");
	stuffDoubleVector(m_data.Y, "dataY");
	stuffDoubleVector(m_data.Z, "dataZ");

	// Stuff the interpolated, icted data into Matlab.
	stuffDoubleVector(m_interpHeave, "iHeave");
	stuffDoubleVector(m_interpSurge, "iSurge");
	stuffDoubleVector(m_interpLateral, "iLateral");

	// Stuff the noise data.
	stuffDoubleVector(m_noise.X, "noiseX");
	stuffDoubleVector(m_noise.Y, "noiseY");
	stuffDoubleVector(m_noise.Z, "noiseZ");
	stuffDoubleVector(m_filteredNoise.X, "fnoiseX");
	stuffDoubleVector(m_filteredNoise.Y, "fnoiseY");
	stuffDoubleVector(m_filteredNoise.Z, "fnoiseZ");

	// Stuff rotation data.
	stuffDoubleVector(m_rotData.X, "yaw");
	stuffDoubleVector(m_rotData.Y, "pitch");
	stuffDoubleVector(m_rotData.Z, "roll");

	stuffDoubleVector(m_fpRotData.X, "fpA");
	stuffDoubleVector(m_fpRotData.Y, "fpE");

	stuffDoubleVector(m_recordedYaw, "ayaw");
	stuffDoubleVector(m_recordedYawVel, "ayawv");



#if SWAP_TIMER
	stuffDoubleVector(m_swapStamp, "swapTimes");
#endif
}

void MoogDotsCom::stuffDoubleVector(vector<double> data, const char *variable)
{
	int i;
	mxArray *matData;

	// Create an mxArray large enough to hold the contents of the data vector.
	matData = mxCreateDoubleMatrix(1, data.size(), mxREAL);

	// Clear the variable if it exists.
	string s = "clear "; s += variable;
	engEvalString(m_engine, s.c_str());

	// Stuff the mxArray with the vector data.
	for (i = 0; i < (int)data.size(); i++) {
		mxGetPr(matData)[i] = data[i];
	}

	engPutVariable(m_engine, variable, matData);

	mxDestroyArray(matData);
}
#endif				//USE_MATLAB
#endif // USE_MATLAB _ USE_MATLAB_INTERPOLATION


bool MoogDotsCom::CheckForEStop()
{
	WRITE_LOG(m_logger->m_logger, "Checking for ESTOP");

	unsigned short digIn;			// Stores the read in estop bit.
	bool eStopActivated = false;	// Indicates if the estop sequence was activated.

									// Read the digital bit.
	cbDIn(ESTOP_IN_BOARDNUM, FIRSTPORTA, &digIn);

	// If it's currently high, but previously low, then a stop command has been issued.
	if ((digIn & 2) && m_previousBitLow == true) {
		eStopActivated = true;

		vector<double> stopVal, zeroCode;
		stopVal.push_back(2.0);
		zeroCode.push_back(0.0);

		m_previousBitLow = false;

		if (m_verboseMode) {
			m_messageConsole->InsertItems(1, &wxString("***** Fixation Break!"), 0);
		}

		// Turn off a bunch of visual stuff along with the movement stop.
		g_pList.SetVectorData("DO_MOVEMENT", stopVal);
		g_pList.SetVectorData("BACKGROUND_ON", zeroCode);
		g_pList.SetVectorData("FP_ON", zeroCode);
		g_pList.SetVectorData("TARG1_ON", zeroCode);
		g_pList.SetVectorData("TARG2_ON", zeroCode);

#if CUSTOM_TIMER
		// Turn off sync pulse.
		m_doSyncPulse = false;
#endif

#if USE_ANALOG_OUT_BOARD
		// Zero the analog out board.
		for (int i = 0; i<8; i++)
			cbAOut(m_PCI_DIO48H_Object.DIO_board_num, i, BIP10VOLTS, DASCALE(0.0));
#endif
	}

	// Reset the previous bit setting to low if the current bit state
	// is low.
	if ((digIn & 2) == 0)
	{
		m_previousBitLow = true;
	}

	return eStopActivated;
}


void MoogDotsCom::Control()
{
	string command;
	LARGE_INTEGER st, fi;
	double start, finish;
	bool stuffChanged = false;

	// Reload the call lists if told to do so.  This is only really used when I need
	// to update the star field from the main thread.  The stars are pre-rendered in a
	// call list and thus are specific to a rendering context.  Since the thread has its
	// own, I can't recreate the call list inside the main thread and expect it to be
	// rendered correctly in the communication thread.
	if (m_reloadCallLists == true)
	{
		m_glWindow->GetGLPanel()->SetupCallList(m_objects2change);
		m_reloadCallLists = false;
	}

	// Don't do anything if listen mode isn't enabled or the connection type is set to none.
	if (m_listenMode == false)
	{
		return;
	}

#if ESTOP
	// Check to see if the estop bit has been set.  If it has, then CheckForEStop will
	// take care of turning the display off and setting the trajectory to be a buffered stop.
	stuffChanged = CheckForEStop();
#endif

	QueryPerformanceCounter(&st);
	start = static_cast<double>(st.QuadPart) / static_cast<double>(m_freq.QuadPart) * 1000.0;

	try
	{
		// Loop for a maximum of CONTROL_LOOP_TIME to get as much stuff from the Tempo buffer as possible.
		do
		{
			// Do the RDX stuff if we have a valid tempo handle and we actually received
			// something on the buffer.
			if (m_matlabRDX->ReadString(1.0, 64, &command, FIRSTPORTB, FIRSTPORTA, SECONDPORTA) > 0)
			{
				string keyword;
				vector<double> commandParams;

				stuffChanged = true;

				//clear the message console to have the maximun numbers of items in the console.
				ClearMessageConsoleMaxItems();

				// Put the command in the message console.
				if (m_verboseMode)
				{
					AddItemToConsole(command);
				}

				//Grab the command with key and parameters for the given key - if the key is not recognized returned "invalid key".
				GrabCommand(command, commandParams, keyword);

				// Set the parameter data if it's supposed to be in the parameter list.
				CommandRecognitionType commandRecognitionType = AddCommandParamsToCommandsList(keyword, commandParams);
				if (m_verboseMode)
				{
					//show command validation and parameters if necessary for the console box.
					ShowCommandStatusValidation(command, keyword, commandRecognitionType);
				}
			}
			else
			{
				start = 0.0;
			}

			QueryPerformanceCounter(&fi);
			finish = static_cast<double>(fi.QuadPart) / static_cast<double>(m_freq.QuadPart) * 1000.0;

		} while ((finish - start) < CONTROL_LOOP_TIME);
	}
	catch (exception &e)
	{
		stuffChanged = false;
		m_messageConsole->InsertItems(1, &wxString("Serious screwup detected!"), 0);
	}

	//Check Matlab ready to receive Oculus Motion and sending it.
	SendOculusHeadTrackingIfAckedTo();

	// Only waste time updating stuff if we actually received valid data from Tempo.
	if (stuffChanged)
	{
		// Updates the GL scene.
		UpdateGLScene(true);

		// Updates basic Moog trajectory movement, including whether or not to move.
		UpdateMovement();

		//just for removing the hourglass when the moog receive the data from the matlab, so draw the world with nothing but the fixation point.
		if (m_glWindow)
		{
			if (m_glWindow->GetGLPanel()->FirstTimeInLoop() == false)
			{

				double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
				WRITE_LOG_PARAM(m_logger->m_logger, "Start ThreadLoop3(2) for fixation point rendering [ms]", time);

				m_glWindow->GetGLPanel()->ThreadLoop3();

				time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
				WRITE_LOG_PARAM(m_logger->m_logger, "Stop ThreadLoop3(2) for fixation point rendering [ms]", time);
			}
		}
	}

	//avi: if data was not changed or not received any data because the timer waiting to the press buttom in the matlab make a thread running the last world image.
	else
	{
		// Grab a pointer to the GLPanel.
		if (m_glWindow)
		{
			GLPanel *glPanel = m_glWindow->GetGLPanel();

			if (glPanel->FirstTimeInLoop() == false && m_trial_finished)
			{
				if (DRAWING_BETWEEN_TRIALS)
				{
					if (glPanel->GetLastNumOfTriangles() > 0 && glPanel->GetLastTrianglesVertexArray() != NULL)
					{
						m_glWindow->GetGLPanel()->renderNow = true;

						double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
						WRITE_LOG_PARAM(m_logger->m_logger, "Start ThreadLoop3(1) for fixation point rendering [ms]", time);

						glPanel->ThreadLoop2();

						time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
						WRITE_LOG_PARAM(m_logger->m_logger, "Stop ThreadLoop3(1) for fixation point rendering [ms]", time);
					}
				}
				else
				{
					//if the waiting time is also finished, than render nothing but the fixation point.
					if (m_waiting_a_little_after_finished)
					{
						m_glWindow->GetGLPanel()->renderNow = true;

						double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
						WRITE_LOG_PARAM(m_logger->m_logger, "Start ThreadLoop3 for fixation point rendering [ms]", time);

						//thread loop3 is for rendering only the fixation point.
						glPanel->ThreadLoop3();

						time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
						WRITE_LOG_PARAM(m_logger->m_logger, "Stop ThreadLoop3 for fixation point rendering [ms]", time);
					}

					//if the waiting time is not over but the render time is over, than render the freeze world include the starfield.
					else
					{//avi : error!!!!!!!!!! - should make it as a thread or remove it because the mbc waits for it to be over.
						double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
						WRITE_LOG_PARAM(m_logger->m_logger, "Start ThreadLoop2 for fixation point rendering and freezing world of stars [ms]", time);

						glPanel->ThreadLoop2();

						time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
						WRITE_LOG_PARAM(m_logger->m_logger, "End ThreadLoop2 for fixation point rendering and freezing world of stars [ms]", time);
					}
				}
			}
		}
	}
} // End void MoogDotsCom::Control()

void MoogDotsCom::Compute()
{
	//avi : for the eyes orientation trace.
	//turn to a a member : unsigned short* orientationsBytesArray;

	//avi : make that for random new vertexes to the scene every repetition
	//startClk = clock();//This remark to see if the randomization does not waste the reaction time

	if (m_startButtonGoToOriginCommand &&  m_data.index == 0)
	{
		WRITE_LOG(m_logger->m_logger, "Calling Compute from m_startButtonGoToOriginCommand.");
		//Add logic when trhe data length is zero because there is no offset from the origin to the current moog position(at start vutton experiment click)
		//if the m_startButtonGoToOriginCommand is true , nut the m_data size if 0, it means that the robot is already at the origin.
		if (m_data.X.size() > 0)
		{
			WRITE_LOG(m_logger->m_logger, "MoveMBCThread from  m_startButtonGoToOriginCommand .");
			_movingMBCThread = MoveMBCThread(m_moveByMoogdotsTrajectory);

			//for the data index not influence in the Computr function. (there is no enter to the next logic loop which incraemant the index).
			m_data.index = m_data.X.size();

			_movingMBCThread.join();
		}
		else
		{
			m_startButtonGoToOriginCommand = false;
		}

		m_forwardMovement = true;
	}
	else if (!m_startButtonGoToOriginCommand)
	{
		WRITE_LOG(m_logger->m_logger, "Calling Compute from normal .");
		if (m_data.index == 0 && m_data.X.size() > 0)
		{
			if (m_forwardMovement)
			{
				_freezeFrameIndex = g_pList.GetVectorData("FREEZE_FRAME").at(0);
			}
			else
			{
				//todo:also -1 for the FREEZE_FRAME (for ensurance).
				_freezeFrameIndex = -1;
				wxString s = wxString::Format("&&&&&&&&&& Tagging _trialAborted as false. &&&&&&&&&&&&");
				m_messageConsole->InsertItems(1, &s, 0);
				//critical section for the changing whem movePlatformToOrigin is coming and _trialAborted is being reset.
				//instead of that can be done by adding delay with matkab command to MOVE_FREEZE 4.0 for trial aborted , and between matlab command for MOVE_TO_ORIGIN.
				EnterCriticalSection(&m_TrialAborted);
				_trialAborted = false;
				LeaveCriticalSection(&m_TrialAborted);
			}

			//if not at the correct place return and show the error window.
			if (!CheckMoogAtCorrectPosition(MAX_DIFFERENT_DISTANCE))
			{
				ThreadDoCompute(RECEIVE_COMPUTE);

				//for not sending then null oculus transformation headings (because at the 1st time there is no data).
				m_oculusIsOn = false;

				//exiting the COMPUTE and transfering to RECEIVE_COMPTE.
				if (m_glData.index < static_cast<int>(m_glData.X.size()))
				{
					m_trial_finished = false;
					m_waiting_a_little_after_finished = false;
					m_glData.index = static_cast<int>(m_glData.X.size());
				}
				UpdateStatusesMembers();

				//todo:add a function call.
				//Disconnect from the MBC , chenged also the state to be parked.
				this->ForceDisconnect();

				//Clos the system window.
				this->m_glWindow->Destroy();
				this->m_parentWindow->Destroy();

				return;
			}

			newRandomStars = true;

			// Updates the GL scene with all new parameters (in the else - the rendering function is called which use that paams and make transorms...)
			UpdateGLScene(true);

			m_roundStartTime = clock();
			WRITE_LOG(m_logger->m_logger, "Starting the round now at t = 0");

			PlotTrajectoryGraph();

			if (m_moveByMoogdotsTrajectory)
			{
				thread soundThread(&MoogDotsCom::PlaySoundThread, this, m_soundData);
				soundThread.detach();
			}
			WRITE_LOG(m_logger->m_logger, "m_forwardMovement is " << m_forwardMovement);
			if (!m_forwardMovement)
			{
				int stimType = g_pList.GetVectorData("STIMULUS_TYPE").at(0);
				if (stimType == 100 ||
					stimType == 110 ||
					stimType == 114 ||
					stimType == 115 ||
					stimType == 120 ||
					stimType == 124 ||
					stimType == 125 ||
					stimType == 130)
					//if (g_pList.GetVectorData("MOOG_CREATE_TRAJ").at(0))
				{
					thread soundThread(&MoogDotsCom::PlaySoundThreadInverse, this, m_soundDataInverse);
					soundThread.detach();
				}
			}

			//Move MBC thread starting.
			//reset it immediately after that because the Matlab may not reset it in the next trial (if the trial is not a one that moog should create it own trajectory).
			g_pList.SetVectorData("MOOG_CREATE_TRAJ", vector <double>(1, 0));
			WRITE_LOG(m_logger->m_logger, "MoveMBCThread from normal .");
			_movingMBCThread = MoveMBCThread(m_moveByMoogdotsTrajectory);

			//reset the bit in the PCI\DIO indicating the matlab if the moog is going to start sending the OculusHeadTracking data.
			int time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
			WRITE_LOG_PARAM(m_logger->m_logger, "SECONDPORTCH ack-reset sent for head motion tracking for the matlab [ms]", time);
			cbDConfigPort(PULSE_OUT_BOARDNUM, SECONDPORTCH, DIGITALOUT);
			cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCH, 0);
		}
		else if (m_data.X.size() == 0 && m_zero_length_m_data_size_trajectory)
		{
			m_zero_length_m_data_size_trajectory = false;

			int stimType = g_pList.GetVectorData("STIMULUS_TYPE").at(0);
			if (stimType == 100 ||
				stimType == 110 ||
				stimType == 114 ||
				stimType == 115 ||
				stimType == 120 ||
				stimType == 124 ||
				stimType == 125 ||
				stimType == 130)
				//if (g_pList.GetVectorData("MOOG_CREATE_TRAJ").at(0))
			{
				thread soundThread(&MoogDotsCom::PlaySoundThreadInverse, this, m_soundDataInverse);
				soundThread.detach();
			}
		}

		if (m_data.index < static_cast<int>(m_data.X.size()))
		{
			if (_freezeFrameIndex <= m_data.index)
			{
				if (_trialAborted)
				{
					//make the visual trial to be over (synthesizded).
					m_data.index = m_data.X.size();
					m_glData.index = m_glData.X.size();
				}
				else if (!_waitForSecondResponse)
					// Increment the counter which we use to pull data.
				{
					m_data.index++;
				}
			}
			else
			{
				// Increment the counter which we use to pull data.
				m_data.index++;
			}

			// Record the last send time's stamp.
#if USE_MATLAB
			m_sendStamp.push_back(ThreadGetSendTime());
#endif

			// Grab the shifted and interpolated data to draw.
			//Renders only at the forward movement due to the condition m_glData.index < static_cast<int>(m_glData.X.size()).
			if (m_data.index > m_recordOffset && m_glData.index < static_cast<int>(m_glData.X.size()))
			{
#if !CUSTOM_TIMER
				// Send out a sync pulse only after the 1st frame of the trajectory has been
				// processed by the platform.  This equates to the 2nd time into this section
				// of the function.
#if FIRST_PULSE_ONLY
				if (m_data.index == m_recordOffset + 1)
				{
#else
				if (m_data.index > m_recordOffset + 1)
				{
#endif // FIRST_PULSE_ONLY
					cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 5);
					cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 4);
				}
#endif
				if (m_glWindowExists)
				{
					//make the transformations afte the UpdateGlScee called in the first frame to updates all paramas and render the first frame.
					RenderFrameInGlPanel();
				}

				if (m_oculusIsOn)
				{
					AddFrameOculusOrientationToCommulativeOculusOrientationTracer();
				}
				if (_freezeFrameIndex <= m_glData.index)
				{
					WRITE_LOG_PARAM(m_logger->m_logger, "Checking _waitForSecondResponse  as true/false and it is", _waitForSecondResponse);
					if (_waitForSecondResponse == false)
					{
						//avi : this was edited , and in original increased by 1.
						m_glData.index++;
					}
				}
				else
				{
					m_glData.index++;
				}

				if (m_glData.index == 1)
				{
					startClk = clock();
				}

				if (m_glData.index >= static_cast<int>(m_glData.X.size()))
				{
					// Set B2, B1 and B0 = OFF, ON, OFF -> (010)=2
					cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 2);
				}
				}
			else
			{
				m_glWindow->GetGLPanel()->renderNow = false;
			}
			}
		else
		{
			if (_movingMBCThread.joinable())
			{
				_movingMBCThread.join();
			}

			// Stop telling the motion base to move, but keep on calling the ReceiveCompute() function.
			ThreadDoCompute(RECEIVE_COMPUTE);

			m_glWindow->GetGLPanel()->renderNow = false;

			UpdateStatusesMembers();

			finishClk = clock();
			double timeDiff = (finishClk - startClk) / double(CLOCKS_PER_SEC) * 1000;
			m_messageConsole->Append(wxString::Format("Compute finished, index = %d time = %d", m_data.index, timeDiff));
#if DEBUG_DEFAULTS
			m_messageConsole->Append(wxString::Format("Compute finished, index = %d", m_data.index));
#endif
		}
		}
	}

void MoogDotsCom::ShowCommandStatusValidation(string command, string keyword, CommandRecognitionType commandRecognitionType)
{
	wxString s;

	switch (commandRecognitionType)
	{
	case Valid:
		break;

	case UnknownType:
		s = wxString::Format("UNKNOWN COMMAND: %s.", command.c_str());
		m_messageConsole->InsertItems(1, &s, 0);
		break;

	case Invalid:
		s = wxString::Format("INVALID PARAMETER VALUES FOR %s.", keyword);
		m_messageConsole->InsertItems(1, &s, 0);
		break;

	default:
		break;
	}
}

CommandRecognitionType MoogDotsCom::AddCommandParamsToCommandsList(string keyword, vector<double> commandParams)
{
	if (g_pList.Exists(keyword) == true)
	{
		if (g_pList.IsVariable(keyword) == false)
		{
			// Make sure that we don't have a parameter count mismatch.  This could
			// cause unexpected results.
			if (static_cast<int>(commandParams.size()) != g_pList.GetParamSize(keyword))
			{
				return CommandRecognitionType::Invalid;
			}
			else
			{
				g_pList.SetVectorData(keyword, commandParams);
				return CommandRecognitionType::Valid;
			}
		}
		else
		{
			g_pList.SetVectorData(keyword, commandParams);
			return CommandRecognitionType::Valid;
		}
	}
	else
	{	// Didn't find keyword in the parameter list.
		return CommandRecognitionType::UnknownType;
	}
}

void MoogDotsCom::GrabCommand(string command, vector<double>& commandParamsOut, string& keywordOut)
{
	int spaceIndex, tmpIndex, tmpEnd;

	string param;

	double convertedValue;

	// Grab the keyword from the command.
	spaceIndex = command.find(" ", 0);

	// If we don't get a valid index, then we've likely gotten a command
	// for another program.  In that case, we don't parse the command string
	// and just set the keyword to "invalid" so that we ignore whatever
	// we just received.
	if (spaceIndex != string::npos)
	{
		keywordOut = command.substr(0, spaceIndex);

		//Grab the commands into the commandParams vector.
	}

	else
	{
		keywordOut = "invalid";
	}

	// Loop and grab the parameters from the command string.
	do
	{
		tmpIndex = command.find_first_not_of(" ", spaceIndex + 1);
		tmpEnd = command.find(" ", tmpIndex);

		// If someone accidentally put a space at the end of the
		// command string, then we want to skip extracting out a
		// parameter value.
		if (tmpIndex != string::npos)
		{
			if (tmpEnd != string::npos)
			{
				spaceIndex = tmpEnd;

				// Pull out the substring with the number in it.
				param = command.substr(tmpIndex, tmpEnd - tmpIndex);
			}
			else {
				// Pull out the substring with the number in it.
				param = command.substr(tmpIndex, command.size() - 1);
			}

			// Convert the string to a double and stuff it in the vector.
			convertedValue = atof(param.c_str());
			commandParamsOut.push_back(convertedValue);
		}
	} while (tmpEnd != string::npos);
}

void MoogDotsCom::AddItemToConsole(string command)
{
	// Put the command in the message console.
	if (m_verboseMode)
	{
		wxString s(command.c_str());
		m_messageConsole->InsertItems(1, &s, 0);
	}
}

void MoogDotsCom::ClearMessageConsoleMaxItems()
{
	// This removes lines in the message console if it is getting too long.
	int numItems = m_messageConsole->GetCount();
	if (numItems >= MAX_CONSOLE_LENGTH)
	{
		for (int i = 0; i <= numItems - MAX_CONSOLE_LENGTH; i++) {
			m_messageConsole->Delete(m_messageConsole->GetCount() - 1);
		}
	}
}


void MoogDotsCom::UpdateMovement()
{
	WRITE_LOG(m_logger->m_logger, "Updating movement...");

	vector<double> zeroVector;
	int switchCode = static_cast<int>(g_pList.GetVectorData("DO_MOVEMENT").at(0));

	zeroVector.push_back(0.0);

	if (g_pList.GetVectorData("GO_TO_ORIGIN").at(0) == 0.0)
	{
		DATA_FRAME startFrame;

		// Grab the movement parameters.
		vector<double> startPoint = g_pList.GetVectorData("M_ORIGIN");
		vector<double> rotStartPoint = g_pList.GetVectorData("ROT_ORIGIN");
		startFrame.lateral = startPoint.at(0); startFrame.surge = startPoint.at(1); startFrame.heave = startPoint.at(2);
		startFrame.yaw = rotStartPoint.at(0); startFrame.pitch = rotStartPoint.at(1); startFrame.roll = rotStartPoint.at(2);

		bool smooth = true;
		switch (switchCode)
		{
		case 0:	// Do nothing
			break;

		case 1:	// Start
#if TRAJECTORY_SAFETY_CHECK
			smooth = CheckTrajectories();
#endif
			if (smooth)
			{
				GenerateMovement();

				// This keeps the program from calculating the Gaussian movement over and over again.
				g_pList.SetVectorData("DO_MOVEMENT", zeroVector);

				ThreadDoCompute(RECEIVE_COMPUTE | COMPUTE);
			}
			break;

		case 2:	// Stop
			GenerateBufferedStop();

			// This keeps the program from calculating the stop movement over and over again.
			g_pList.SetVectorData("DO_MOVEMENT", zeroVector);

			break;
		};
	} // End if (g_pList.GetVectorData("GO_TO_ORIGIN")[0] == 0.0)
	else
	{
		//if the command is to go to origin due to start button experiment click.
		if (g_pList.GetVectorData("GO_TO_ORIGIN").at(0) == 2.0)
		{
			g_pList.SetVectorData("GO_TO_ORIGIN", zeroVector);
			m_startButtonGoToOriginCommand = true;
		}

		MovePlatformToOrigin();

		// Start sending trajectory data to the Moog.
		ThreadDoCompute(RECEIVE_COMPUTE | COMPUTE);
	}
}


void MoogDotsCom::GenerateMovement()
{
	WRITE_LOG(m_logger->m_logger, "Generating movement...");

	// Do no move these initializations.  Their location in the function is very important for
	// threading issues. - Johnny 1/24/08
	m_recordIndex = 0;

	// Generate a frame to represent the origin.
	vector<double> transOrigin = g_pList.GetVectorData("M_ORIGIN"),
		rotOrigin = g_pList.GetVectorData("ROT_ORIGIN");
	DATA_FRAME startFrame;
	//avi : interpolation version : delete the static_cast<float> (it not accurate as we want because it delete a lot of bits).
	startFrame.lateral = (transOrigin.at(0));
	startFrame.surge = (transOrigin.at(1));
	startFrame.heave = (transOrigin.at(2));
	startFrame.yaw = (rotOrigin.at(0));
	startFrame.pitch = (rotOrigin.at(1));
	startFrame.roll = (rotOrigin.at(2));

	// Calculates the actual trajectory to origin.
	MovePlatform(&startFrame);

	// Store the length of the data structure holding the movement to
	// origin.  This let's us know when to start setting the OpenGL data.
	m_recordOffset = static_cast<int>(m_data.X.size());

	// We use these parameters to calculate the center of rotation for OpenGL.
	vector<double> headCenter = g_pList.GetVectorData("HEAD_CENTER"),
		rotationOffsets = g_pList.GetVectorData("GL_ROT_OFFSETS"),
		eyeOffsets = g_pList.GetVectorData("EYE_OFFSETS");

	eyeOffsets.at(2) += FP_ORIGIN_ADJUST;

	// Grab the trajectory data from the parameter list.
	vector<double> trajectories[6];
	trajectories[0] = g_pList.GetVectorData("LATERAL_DATA");
	trajectories[1] = g_pList.GetVectorData("SURGE_DATA");
	trajectories[2] = g_pList.GetVectorData("HEAVE_DATA");
	trajectories[3] = g_pList.GetVectorData("YAW_DATA");
	trajectories[4] = g_pList.GetVectorData("PITCH_DATA");
	trajectories[5] = g_pList.GetVectorData("ROLL_DATA");

	// Grab the OpenGL trajectory data.
	vector<double> glTrajectories[4];
	glTrajectories[0] = g_pList.GetVectorData("GL_LATERAL_DATA");
	glTrajectories[1] = g_pList.GetVectorData("GL_SURGE_DATA");
	glTrajectories[2] = g_pList.GetVectorData("GL_HEAVE_DATA");
	// Grab the OpenGL object unit trajectory data.
	glTrajectories[3] = g_pList.GetVectorData("OBJECT_TRAJ");

	//Grab the draw flashing square frames data.
	vector<double> flashingSquareFramesData;
	flashingSquareFramesData = g_pList.GetVectorData("FLASH_SQUARE_DATA");

	// Grab the OpenGL rotation information.
	m_glRotData = g_pList.GetVectorData("GL_ROT_DATA");
	m_glRotEle = g_pList.GetVectorData("GL_ROT_ELE");
	m_glRotAz = g_pList.GetVectorData("GL_ROT_AZ");

	// Set the OpenGL starting position for cutout circle
	m_glWindow->GetGLPanel()->SetGlStartData(
		glTrajectories[0].at(0),
		glTrajectories[1].at(0),
		glTrajectories[2].at(0),
		m_glRotData.at(0));

	// Calulation of offsets for rotation.
	double xdist = -eyeOffsets.at(0) + rotationOffsets.at(0),
		ydist = -eyeOffsets.at(2) + rotationOffsets.at(2),
		zdist = CENTER2SCREEN - headCenter.at(1) - rotationOffsets.at(1);
	m_glWindow->GetGLPanel()->SetRotationCenter(xdist, ydist, zdist);

	//// Calculate the rotation vector describing the axis of rotation.
	//m_rotationVector = nmSpherical2Cartesian(rAngles.at(1), rAngles.at(0), 1.0, true);

	//// Swap the y and z values of the rotation vector to accomodate OpenGL.  We also have
	//// to negate the y value because forward is negative in our OpenGL axes.
	//double tmp = -m_rotationVector.y;
	//m_rotationVector.y = m_rotationVector.z;
	//m_rotationVector.z = tmp;
	//m_glWindow->GetGLPanel()->SetRotationVector(m_rotationVector);

	// Find the maximum and minimum length of all trajectories.
	// If the trajectories aren't all the same length print out a warning.
	// We'll use the minimum length when appending data from the server
	// to the internal data vectors.
	int minLength, maxLength;
	minLength = maxLength = static_cast<int>(trajectories[0].size());
	for (int i = 1; i < 6; i++) {
		int trajSize = static_cast<int>(trajectories[i].size());

		// Check for a minimum.
		if (trajSize < minLength) {
			minLength = trajSize;
		}

		// Check for a maximum.
		if (trajSize > maxLength) {
			maxLength = trajSize;
		}
	}

	if (minLength != maxLength) {
		//if (m_verboseMode) {
		wxString s = wxString::Format("*** Trajectories didn't have the same length, truncating to %d", minLength);
		m_messageConsole->InsertItems(1, &s, 0);
		//}
	}

	// Append the data received from the server to the trajectory information.
	for (int i = 0; i < minLength; i++) {
		m_data.X.push_back(trajectories[0].at(i) / 100.0);
		m_data.Y.push_back(trajectories[1].at(i) / 100.0);
		m_data.Z.push_back(trajectories[2].at(i) / 100.0);
		m_rotData.X.push_back(trajectories[3].at(i));
		m_rotData.Y.push_back(trajectories[4].at(i));
		m_rotData.Z.push_back(trajectories[5].at(i));
	}

	/////////////////////////////////////////////////////////////////////////////interpolated data version///////////////////////////////////////////////
	/*
	avi : matlab interpolation version:
	This will allow all the thread (the other threads) to connect to this particular matlab engine even if they have a pointer to others engine.
	It should be said that eacj thred should have it's own matlab engime (pointer) for using the matlab ,but because the line after the "m_engine = engOpen(NULL)" line , they will use the same engine either
	that the pointer of each thread is for different matlab engine (but should to is thread safe).
	*/
	/*EnterCriticalSection(&m_matlabInterpolation);
	Engine* m_engine2 = engOpen(NULL);
	int x = engEvalString(m_engine2, "format long");
	mxArray* result = mxCreateDoubleMatrix(minLength , 1, mxREAL);
	memcpy((void*)mxGetPr(result), (void*)&m_data.Y.at(0), sizeof(double) * minLength);
	engPutVariable(m_engine2, "points", result);
	engEvalString(m_engine2, "x = [1:1:60]");
	x = engEvalString(m_engine2, "f = interp1(x' , points , 'pchip')");
	LeaveCriticalSection(&m_matlabInterpolation);*/

	std::vector<double> X;
	tk::spline sX;
	tk::spline sY;
	tk::spline sZ;

	for (int i = 0; i < minLength; i++)
	{
		X.push_back(i* INTERPOLATION_WIDE * INTERPOLATION_UPSAMPLING_SIZE);
	}

	sX.set_points(X, m_data.X, true);    // currently it is required that X is already sorted
	sY.set_points(X, m_data.Y, true);    // currently it is required that X is already sorted
	sZ.set_points(X, m_data.Z, true);    // currently it is required that X is already sorted

	for (int i = 0; i < (minLength - 1)*INTERPOLATION_UPSAMPLING_SIZE; i++)
	{
		m_interpolatedData.X.push_back(sX(i* INTERPOLATION_WIDE));
		m_interpolatedData.Y.push_back(sY(i* INTERPOLATION_WIDE));
		m_interpolatedData.Z.push_back(sZ(i* INTERPOLATION_WIDE));
	}

	tk::spline sRotX;
	tk::spline sRotY;
	tk::spline sRotZ;

	sRotX.set_points(X, m_rotData.X, true);    // currently it is required that X is already sorted
	sRotY.set_points(X, m_rotData.Y, true);    // currently it is required that X is already sorted
	sRotZ.set_points(X, m_rotData.Z, true);    // currently it is required that X is already sorted

	for (int i = 0; i < (minLength - 1)*INTERPOLATION_UPSAMPLING_SIZE; i++)
	{
		m_interpolatedRotData.X.push_back(sRotX(i* INTERPOLATION_WIDE));
		m_interpolatedRotData.Y.push_back(sRotY(i* INTERPOLATION_WIDE));
		m_interpolatedRotData.Z.push_back(sRotZ(i* INTERPOLATION_WIDE));
	}
	/////////////////////////////////////////////////////////////////////////////end interpolated data version///////////////////////////////////////////

	//convert the degree values to radian values because the MBC gets the values as radians.
	for (int i = 0; i<(minLength - 1)*INTERPOLATION_UPSAMPLING_SIZE; i++)
	{
		m_interpolatedRotData.X[i] = deg2rad(m_interpolatedRotData.X[i]);
		m_interpolatedRotData.Y[i] = deg2rad(m_interpolatedRotData.Y[i]);
		m_interpolatedRotData.Z[i] = deg2rad(m_interpolatedRotData.Z[i]);
	}

	// Do the same finding of min and max lengths for the OpenGL trajectories.
	minLength = maxLength = static_cast<int>(glTrajectories[0].size());
	for (int i = 1; i < 4; i++) {
		int trajSize = static_cast<int>(glTrajectories[i].size());

		// Check for a minimum.
		if (trajSize < minLength) {
			minLength = trajSize;
		}

		// Check for a maximum.
		if (trajSize > maxLength) {
			maxLength = trajSize;
		}
	}

	if (minLength != maxLength) {
		//if (m_verboseMode) {
		wxString s = wxString::Format("*** Trajectories didn't have the same length, truncating to %d", minLength);
		m_messageConsole->InsertItems(1, &s, 0);
		//}
	}

	// Append the data received from the server to the OpenGL trajectory information,
	// and openGL object trajectory information
	double azimth = g_pList.GetVectorData("OBJECT_AZI").at(0)*DEG2RAD;
	double elevation = g_pList.GetVectorData("OBJECT_ELE").at(0)*DEG2RAD;

	nmClearMovementData(&m_glData);
	nmClearMovementData(&m_glObjectData);
	m_drawFlashingFrameSquareData.clear();
	for (int i = 0; i < minLength; i++)
	{
		m_glData.X.push_back(glTrajectories[0].at(i));
		m_glData.Y.push_back(glTrajectories[1].at(i));
		m_glData.Z.push_back(glTrajectories[2].at(i));

		m_glObjectData.X.push_back(glTrajectories[3].at(i)*cos(elevation)*cos(azimth));
		m_glObjectData.Y.push_back(glTrajectories[3].at(i)*sin(elevation));
		m_glObjectData.Z.push_back(glTrajectories[3].at(i)*cos(elevation)*sin(azimth));

		m_drawFlashingFrameSquareData.push_back(flashingSquareFramesData.at((i)) > 0);
	}

	AddNoise();

	//Create the sound vector if needed.
	m_moveByMoogdotsTrajectory = g_pList.GetVectorData("MOOG_CREATE_TRAJ").at(0);
	if (m_moveByMoogdotsTrajectory && m_forwardMovement)
	{
		//double azimuth = CalculateDistanceTrajectory();
		double azimuth = g_pList.GetVectorData("DISC_AMPLITUDES").at(0);

		CreateSoundVector(m_soundVelocity, azimuth , (SOUND_WAVE_TYPE)((int)g_pList.GetVectorData("WAV_TYPE").at(0)) , m_soundData);

		CreateSoundVectorInverse(m_soundDataInverse);
	}
}


void MoogDotsCom::GenerateBufferedStop()
{
	vector<double> x;
	double  ixv = 0.0, iyv = 0.0, izv = 0.0,		// Instananeous velocities (m/frame)
		iyawv = 0.0, ipitchv = 0.0, irollv = 0.0;

	// This will keep the Compute() function from trying to draw predicted data,
	// and use real feedback instead.
	m_recordOffset = SPEED_BUFFER_SIZE;

	// Get the current position of each axis.
	DATA_FRAME currentFrame;
	THREAD_GET_DATA_FRAME(&currentFrame);

	// Calculate the instantaneous velocity for each axis.
	ixv = currentFrame.lateral - m_previousPosition.lateral;
	iyv = currentFrame.surge - m_previousPosition.surge;
	izv = currentFrame.heave - m_previousPosition.heave;
	iyawv = currentFrame.yaw - m_previousPosition.yaw;
	ipitchv = currentFrame.pitch - m_previousPosition.pitch;
	irollv = currentFrame.roll - m_previousPosition.roll;

	// Reset the movement data.
	nmClearMovementData(&m_data);
	nmClearMovementData(&m_rotData);

	//avi : interpolated version
	nmClearMovementData(&m_interpolatedData);
	nmClearMovementData(&m_interpolatedRotData);

	// Create buffered movement data.
	for (int i = 0; i < SPEED_BUFFER_SIZE; i++)
	{
		// Translational movement data.
		currentFrame.lateral += ixv * m_speedBuffer[i];
		currentFrame.surge += iyv * m_speedBuffer[i];
		currentFrame.heave += izv * m_speedBuffer[i];
		m_data.X.push_back(currentFrame.lateral);
		m_data.Y.push_back(currentFrame.surge);
		m_data.Z.push_back(currentFrame.heave);

		// Rotational movement data.
		currentFrame.yaw += iyawv * m_speedBuffer[i];
		currentFrame.pitch += ipitchv * m_speedBuffer[i];
		currentFrame.roll += irollv * m_speedBuffer[i];
		m_rotData.X.push_back(currentFrame.yaw);
		m_rotData.Y.push_back(currentFrame.pitch);
		m_rotData.Z.push_back(currentFrame.roll);
	}
}

void MoogDotsCom::ConvertUnsignedShortArrayToByteArrayDedicatedToCommunication(byte data, byte  returnArray[2])
{
	returnArray[0] = ((data & 240) >> 4) | 128;	//MSB
	returnArray[1] = (data & 15) | 128;			//LSB
}

void MoogDotsCom::SendOculusHeadTrackingIfAckedTo()
{
	WRITE_LOG(m_logger->m_logger, "Checking if to send the Oculus data to the Matlab.");

	//receivedValue indicate if the Matlab send a command that it is ready for receiving the OculusHeadMotionTracking.
	unsigned short int receivedValue;
	cbDConfigPort(PULSE_OUT_BOARDNUM, FIRSTPORTCH, 0);
	cbDIn(PULSE_OUT_BOARDNUM, FIRSTPORTCH, &receivedValue);

	if (m_finishedMovingBackward && receivedValue == 2)
	{
		//send the matlab in the PCI\DIO that the moog is going to send the data the Matlab asked before(that would be after the Moog finished the forward and backward movement and get the Matlab command before , between the movements , when the PostTrialTime begin).
		int time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
		WRITE_LOG_PARAM(m_logger->m_logger, "SECONDPORTCH ack-set sent for head motion tracking for the matlab [ms]", time);
		cbDConfigPort(PULSE_OUT_BOARDNUM, SECONDPORTCH, DIGITALOUT);
		cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCH, 1);

		time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
		WRITE_LOG_PARAM(m_logger->m_logger, "Start sending oculus head motion tracking for the matlab [ms]", time);

		//send the data to Matlab.
		SendHeadMotionTrackToMatlab(&m_orientationsBytesArray[0], sizeof(ovrQuatf) / 2 * m_glData.index);

		time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
		WRITE_LOG_PARAM(m_logger->m_logger, "End sending oculus head motion tracking for the matlab [ms]", time);

		//reset the m_finishedMovingBackward if not reset yet(should be restet).
		m_finishedMovingBackward = false;
	}
}

void MoogDotsCom::AddFrameOculusOrientationToCommulativeOculusOrientationTracer()
{
	//avi : editing for the placing of the eyes trace.
	byte* byteArray = new byte[16];
	memcpy(byteArray, &m_eyeOrientationQuaternion, sizeof(ovrQuatf));
	memcpy(m_orientationsBytesArray + m_glData.index * sizeof(ovrQuatf) / 2, byteArray, sizeof(ovrQuatf));
	//memory deallocation.
	delete[] byteArray;
}

void MoogDotsCom::SendHeadMotionTrackToMatlab(unsigned short* orientationsBytesArray, int size)
{
	WRITE_LOG(m_logger->m_logger, "sending head motion treack to matlab...");

	//SECONDPORTCL - is the port to send the (at least) two bits in order to indicate start of sending information to matlab.
	//SECONDPORTB - is the port to send the information of the head racking to the matlab.
	//FIRSTPORTCL - is the port to liten to signals bits from matlab that have received the information bits.

	cbDConfigPort(PULSE_OUT_BOARDNUM, SECONDPORTCL, DIGITALOUT);
	cbDConfigPort(PULSE_OUT_BOARDNUM, FIRSTPORTCL, DIGITALIN);
	cbDConfigPort(PULSE_OUT_BOARDNUM, SECONDPORTB, DIGITALOUT);
	int x = 0;// = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCL, (unsigned short)1);

			  //here ,we send the information byte before the confirmation of the first bit of initialization due to races in the innformation byte(which may be not enough time to read after write ,
			  //so, in order to emit the damage of the delay we send it before the init bit to give a long pre time (so the delay kicked off).
	unsigned short readBytes = 0;
	bool skipFirstCondition = false;	//each of the skipCondition variable is indicaing if to skip the n'th condition because it was passed alrready and may now not passes due to needed changes.
	bool skipSecondCondition = false;
	bool skipThirdCondition = false;
	bool skipFirstWriting = false;
	bool skipSecondWriting = false;
	bool skipThirdWritinng = false;
	bool skipForthWriting = false;
	bool skipFifthWriting = false;
	bool skipSixWriting = false;
	byte* byteArray = new byte[2];		//The array toi catch each index in orientationsBytesArray of type unsigned short int = 2 bytes.
	byte firstChar1;					//The first char (byte) of the decompress byteArray.
	byte secondChar1;					//The second char (byte) of the decompress byteArray.
	ofstream myfile;
	myfile.open("log_writing.txt");
	byte* charArray = new byte[2];		/*
										Array include the data for only 1 byte in 2 bytes as folows that 'n' or '\0' would not fall never as the data sent (the Matlab would compress the byte data from the 2 bytes).
										The data send is really as follows: a[0] = xxxxd1d2d3d4 , a[1] = xxxxd5d6d7d8.
										The Matlab would compress it to 1 byte of data d1d2d3d4d5d6d7d8.
										*/
	int m2 = 0;							//when hits to 5000 - it's time to render the empty world. because if not, the hourglass would appear due to the do while loop.
	int startTime = clock();			//For timeouts for skip the sending.
	int currentTime;					//For timeouts for skip the sending.
	bool skipTheEndCharacterSend = false;	//If was timeout during the sending skip all the stages of handshake at the end include the '\n' inicator.
	double timeDiff;					//For timeouts for skip the sending.
	int beginSendingTime = clock();		//For measure the time to send the OculusHeadTrackingData.

										/*
										The 'n' is used as data to be send to indicate matlab that it is the last data to be received - so cant send it as data if not really indicating the end of the data stream.
										The '\0' is the c++ end of string when matlab mex file take the bytes to the string - so cant send it as data also.
										*/
	for (int i = 0; i < size; i++)
	{
		memcpy(byteArray, orientationsBytesArray + i, 2 * sizeof(byte));

		memcpy(&firstChar1, byteArray, sizeof(byte));
		memcpy(&secondChar1, byteArray + 1, sizeof(byte));

		int m = 0;
		for (int j = 0; j < 4; j++)
		{
			WRITE_LOG_PARAM(m_logger->m_logger, "writing to matlab from oculus", i);

			if (j == 0 || j == 1)
				ConvertUnsignedShortArrayToByteArrayDedicatedToCommunication(firstChar1, charArray);
			else
				ConvertUnsignedShortArrayToByteArrayDedicatedToCommunication(secondChar1, charArray);

			do
			{
				currentTime = clock();
				timeDiff = (currentTime - startTime) / double(CLOCKS_PER_SEC) * 1000;
				if (timeDiff > 3000.0)
				{
					skipTheEndCharacterSend = true;
					break;
				}

				cbDIn(PULSE_OUT_BOARDNUM, FIRSTPORTCL, &readBytes);
				m2++;
				//it's time to render the empty world. because if not, the hourglass would appear due to the do while loop.
				if (m2 > 50000)
				{
					m_glWindow->GetGLPanel()->ThreadLoop3();
					m2 = 0;
				}
			} while (readBytes);

			if (skipTheEndCharacterSend)
				break;
			Sleep(0.5);

			x = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTB, (char)(charArray[m]));
			m++;
			m = m % 2;
			x = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCL, (unsigned short)1);

			do
			{
				currentTime = clock();
				timeDiff = (currentTime - startTime) / double(CLOCKS_PER_SEC) * 1000;
				if (timeDiff > 3000.0)
				{
					skipTheEndCharacterSend = true;
					break;
				}

				cbDIn(PULSE_OUT_BOARDNUM, FIRSTPORTCL, &readBytes);
				m2++;
				//it's time to render the empty world. because if not, the hourglass would appear due to the do while loop.
				if (m2 > 50000)
				{
					m_glWindow->GetGLPanel()->ThreadLoop3();
					m2 = 0;
				}
			} while (!readBytes);

			if (skipTheEndCharacterSend)
				break;

			x = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCL, (unsigned short)0);

			Sleep(0.5);
		}

		if (skipTheEndCharacterSend)
			break;
	}

	//free the allocated memory.
	delete[] charArray;
	//memory deallocation.
	delete[] byteArray;

	if (skipTheEndCharacterSend)
		return;

	do
	{
		cbDIn(PULSE_OUT_BOARDNUM, FIRSTPORTCL, &readBytes);
	} while (readBytes);


	//sending the "new line" character to  indicate the end of the data (\n).
	//send the information 2bytes(unsigned short) before the init bit as explained before.
	x = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTB, '\n');
	//myfile << (int(0));

	//send the first bit of initialization
	x = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCL, (unsigned short)1);

	Sleep(0.5);

	//the server wait to know that matlab get the information bit.
	do
	{
		cbDIn(PULSE_OUT_BOARDNUM, FIRSTPORTCL, &readBytes);
	} while (!readBytes);

	//send confirmation to the matlab that we get the ack bytes.
	cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTCL, (unsigned short)0);

	//wait for matlab confirm that it waits for another byts of information
	do
	{
		cbDIn(PULSE_OUT_BOARDNUM, FIRSTPORTCL, &readBytes);
	} while (readBytes);

	//myfile.close();
	x = cbDOut(PULSE_OUT_BOARDNUM, SECONDPORTB, (unsigned short)0);

	int endSendingTime = clock();
	int diff = (endSendingTime - beginSendingTime) * 1000 / CLOCKS_PER_SEC;
}

void MoogDotsCom::SendMBCFrame(int& data_index)
{
	WRITE_LOG_PARAM(m_logger->m_logger, "Sending MBC frame thread.", data_index);

	int start = clock();
	if (m_data.X.size() >= 60)
	{
		for (int i = 0; i < INTERPOLATION_UPSAMPLING_SIZE; i++)
		{
			EnterCriticalSection(&m_CS);
			DATA_FRAME moogFrame;
			moogFrame.lateral = static_cast<double>(m_interpolatedData.X.at((data_index * INTERPOLATION_UPSAMPLING_SIZE + i)));
			moogFrame.surge = static_cast<double>(m_interpolatedData.Y.at((data_index * INTERPOLATION_UPSAMPLING_SIZE + i)));
			moogFrame.heave = static_cast<double>(m_interpolatedData.Z.at((data_index * INTERPOLATION_UPSAMPLING_SIZE + i))) + MOTION_BASE_CENTER;
			moogFrame.yaw = static_cast<double>(m_interpolatedRotData.X.at((data_index * INTERPOLATION_UPSAMPLING_SIZE + i)));
			moogFrame.pitch = static_cast<double>(m_interpolatedRotData.Y.at((data_index * INTERPOLATION_UPSAMPLING_SIZE + i)));
			moogFrame.roll = static_cast<double>(m_interpolatedRotData.Z.at((data_index * INTERPOLATION_UPSAMPLING_SIZE + i)));
			SET_DATA_FRAME(&moogFrame);
			LeaveCriticalSection(&m_CS);
			double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
			WRITE_LOG_PARAM(m_logger->m_logger, "The time for 1/16 frame was [ms]", time);
			WRITE_LOG_PARAM(m_logger->m_logger, "The surge for 1/16 frame was", moogFrame.surge);
		}
	}
}

void MoogDotsCom::ResetEEGPins(short trialNumber)
{
	WRITE_LOG(m_logger->m_logger, "Sending the trial number to the EEG pins...");

	trialNumber = trialNumber & 0x0fff;

	//reset the bits
	Sleep(10);
	m_EEGLptContoller->Write(LPT_PORT, 0);
	WRITE_LOG_PARAM(m_logger->m_logger, "Sending the trial number reset round", trialNumber);


	//1 t1 t2 t3
	Sleep(10);
	short firstRoundMSB = (trialNumber >> 9) & 0x07;
	m_EEGLptContoller->Write(LPT_PORT, firstRoundMSB | 0x08);
	WRITE_LOG_PARAM(m_logger->m_logger, "Sending the trial number first round", firstRoundMSB);

	//reset the bits
	Sleep(10);
	m_EEGLptContoller->Write(LPT_PORT, 0);
	WRITE_LOG(m_logger->m_logger, "Sending the trial number reset round");

	//1 t4 t5 t6
	Sleep(10);
	short secondRoundMSB = (trialNumber >> 6) & 0x07;
	m_EEGLptContoller->Write(LPT_PORT, secondRoundMSB | 0x08);
	WRITE_LOG_PARAM(m_logger->m_logger, "Sending the trial number second round", secondRoundMSB);

	//reset the bits
	Sleep(10);
	m_EEGLptContoller->Write(LPT_PORT, 0);
	WRITE_LOG(m_logger->m_logger, "Sending the trial number reset round");

	//1 t7 t8 t9
	Sleep(10);
	short thirdRoundMSB = (trialNumber >> 3) & 0x07;
	m_EEGLptContoller->Write(LPT_PORT, thirdRoundMSB | 0x08);
	WRITE_LOG_PARAM(m_logger->m_logger, "Sending the trial number third round", thirdRoundMSB);

	//reset the bits
	Sleep(10);
	m_EEGLptContoller->Write(LPT_PORT, 0);
	WRITE_LOG(m_logger->m_logger, "Sending the trial number reset round");

	//1 t10 t11 t12
	Sleep(10);
	short fourthRoundMSB = (trialNumber >> 0) & 0x07;
	m_EEGLptContoller->Write(LPT_PORT, fourthRoundMSB | 0x08);
	WRITE_LOG_PARAM(m_logger->m_logger, "Sending the trial number fourth round", fourthRoundMSB);
	
	//reset the bits
	Sleep(10);
	m_EEGLptContoller->Write(LPT_PORT, 0);
	WRITE_LOG(m_logger->m_logger, "Sending the trial number reset round");
}

void MoogDotsCom::CalculateRotateTrajectory()
{
	//todo:check what numbers goes here:
	double PLATFORM_ROT_CENTER_X = 0.0;
	double PLATFORM_ROT_CENTER_Y = 0.0;
	double PLATFORM_ROT_CENTER_Z = 122.0;
	double CUBE_ROT_CENTER_X = 0.0;
	double CUBE_ROT_CENTER_Y = 0.0;
	double CUBE_ROT_CENTER_Z = 0.0;

	nmMovementData tmpData, tmpRotData;

	m_continuousMode = false;

	vector<double> platformCenter = g_pList.GetVectorData("PLATFORM_CENTER"),
		headCenter = g_pList.GetVectorData("HEAD_CENTER"),
		origin = g_pList.GetVectorData("ORIGIN"),
		rotationCenterOffsets = g_pList.GetVectorData("ROT_CENTER_OFFSETS");

	// Parameters for the rotation.
	double amplitude = g_pList.GetVectorData("ROT_AMPLITUDE").at(0),
		duration = g_pList.GetVectorData("ROT_DURATION").at(0),
		sigma = g_pList.GetVectorData("ROT_SIGMA").at(0),

		// We negate elevation to be consistent with previous program conventions.
		elevation = g_pList.GetVectorData("ROT_ELEVATION").at(0),
		azimuth = g_pList.GetVectorData("ROT_AZIMUTH").at(0),
		step = 1.0 / SAMPLES_PER_SECOND;

	double elevationOffset = 0;
	double azimuthOffset = 0;

	// Generate the rotation amplitude with a Gaussian velocity profile.
	vector<double> aM;
	vector<double> vM;
	vector<double> dM;
	double isum;
	//nmGen1DVGaussTrajectory(&dM, amplitude, duration, 42000.0, sigma, 0.0, true);
	nmGenGaussianCurve(&vM, amplitude, duration, (int)SAMPLES_PER_SECOND, sigma, 2, true);
	double sum;
	nmTrapIntegrate(&vM, &dM, sum, 0, SAMPLES_PER_SECOND, 1 / SAMPLES_PER_SECOND);
	nmGenDerivativeCurve(&aM, &vM, 1 / SAMPLES_PER_SECOND, true);

	//make the gaussian distance trajectory with the needed amplitud (normalize it).
	//also convert to radians.
	double max = dM[(int)SAMPLES_PER_SECOND - 1];
	for (int i = 0; i < dM.size(); i++)
	{
		dM[i] = ((dM[i] * amplitude) / max);
	}

	// Point is the center of the platform, rotPoint is the subject's head + offsets.
	nm3DDatum point, rotPoint;
	point.x = platformCenter.at(0) + origin.at(0);
	point.y = platformCenter.at(1) + origin.at(1);
	point.z = platformCenter.at(2) - origin.at(2);

	//todo:check why the sign of the PLATFORM_ROT_CENTER_X is opposite to matlab.
	rotPoint.x = headCenter.at(0) + CUBE_ROT_CENTER_X + PLATFORM_ROT_CENTER_X + rotationCenterOffsets.at(0) + origin.at(0);
	rotPoint.y = headCenter.at(1) + CUBE_ROT_CENTER_Y + PLATFORM_ROT_CENTER_Y + rotationCenterOffsets.at(1) + origin.at(1);
	rotPoint.z = headCenter.at(2) + CUBE_ROT_CENTER_Z + PLATFORM_ROT_CENTER_Z + rotationCenterOffsets.at(2) - origin.at(2);

	double rotElevation = (elevation - elevationOffset);
	double rotAzimuth = (azimuth - azimuthOffset);
	rotAzimuth = -rotAzimuth;		//todo:the sigh here is opposite to the TOMORIG.
	rotElevation = -rotElevation;

	nmRotatePointAboutPoint(point, rotPoint, rotElevation, rotAzimuth, &dM,
		&tmpData, &tmpRotData, true, false);

	//down sampling to 1000Hz for the MBC.
	nmClearMovementData(&m_data);
	nmClearMovementData(&m_rotData);
	for (int i = 0; i < SAMPLES_PER_SECOND; i = i + (SAMPLES_PER_SECOND / 1000))
	{
		//normalize from cm to meters because the MBC takes it in meters.
		m_data.X.push_back(tmpData.X.at(i) / 100);
		m_data.Y.push_back(tmpData.Y.at(i) / 100);
		m_data.Z.push_back(tmpData.Z.at(i) / 100);

		m_rotData.X.push_back(deg2rad(tmpRotData.X.at(i)));
		m_rotData.Y.push_back(deg2rad(tmpRotData.Y.at(i)));
		m_rotData.Z.push_back(deg2rad(tmpRotData.Z.at(i)));
	}


	vector<double> dataVelocity;
	nmGenDerivativeCurve(&dataVelocity, &(tmpRotData.Y), 1 / SAMPLES_PER_SECOND, true);

	nmGenDerivativeCurve(&m_soundVelocity, &dataVelocity, 1 / SAMPLES_PER_SECOND, true);
}

double MoogDotsCom::CalculateDistanceTrajectory()
{
	vector<double> 		origin = g_pList.GetVectorData("ORIGIN");

	double azimuth = g_pList.GetVectorData("DISC_PLANE_AZIMUTH").at(0),
		elevation = g_pList.GetVectorData("DISC_PLANE_ELEVATION").at(0),
		tilt = g_pList.GetVectorData("DISC_PLANE_TILT").at(0);

	double amps = g_pList.GetVectorData("DISC_AMPLITUDES").at(0),
		dist = g_pList.GetVectorData("DIST").at(0),
		duration = g_pList.GetVectorData("DURATION").at(0),
		sigma = g_pList.GetVectorData("SIGMA").at(0),
		adaptation_amp = g_pList.GetVectorData("ADAPTATION_ANGLE").at(0);

	// Generate the distance amplitude with a Gaussian velocity profile.
	vector<double> aM;
	vector<double> vM;
	vector<double> dM;
	double isum;
	//nmGen1DVGaussTrajectory(&dM, amplitude, duration, 42000.0, sigma, 0.0, true);
	nmGenGaussianCurve(&vM, dist, duration / 1000, SAMPLES_PER_SECOND, sigma, 2, true);
	double sum;
	nmTrapIntegrate(&vM, &dM, sum, 0, SAMPLES_PER_SECOND, 1 / SAMPLES_PER_SECOND);
	nmGenDerivativeCurve(&aM, &vM, 1 / SAMPLES_PER_SECOND, true);

	//make the gaussian distance trajectory with the needed amplitud (normalize it).
	//also convert to radians.
	double max = dM[(int)SAMPLES_PER_SECOND - 1];
	for (int i = 0; i < dM.size(); i++)
	{
		dM[i] = ((dM[i] * dist) / max);
	}

	double amp = amps * PI / 180;
	double az = azimuth * PI / 180;
	double el = elevation * PI / 180;
	double ti = tilt * PI / 180;

	amp += adaptation_amp * PI / 180 / 2;

	double xM = -sin(amp)*sin(az)*cos(ti) +
		cos(amp)*(cos(az)*cos(el) + sin(az)*sin(ti)*sin(el));

	double yM = sin(amp)*cos(az)*cos(ti) +
		cos(amp)*(sin(az)*cos(el) - cos(az)*sin(ti)*sin(el));

	double zM = -sin(amp)*sin(ti) -
		cos(amp)*sin(el)*cos(ti);

	nmMovementData trajData;

	for (int i = 0; i < dM.size(); i++)
	{
		trajData.X.push_back(dM.at(i)*yM);
		trajData.Y.push_back(dM.at(i)*xM);
		trajData.Z.push_back(dM.at(i)*zM);
	}

	//todo: add here the data for the robot agian (when robot would work) and add it to the interpolation parameter and not the m_data because in the line : if (m_data.index < static_cast<int>(m_data.X.size())).
	//down sampling to 1000Hz for the MBC.
	//nmClearMovementData(&m_data);
	//nmClearMovementData(&m_rotData);
	//for (int i = 0; i < SAMPLES_PER_SECOND; i = i + (SAMPLES_PER_SECOND / 1000))
	//{
	//	m_data.X.push_back(trajData.X.at(i) / 100);
	//	m_data.Y.push_back(trajData.Y.at(i) / 100);
	//	m_data.Z.push_back(trajData.Z.at(i) / 100);
	//	m_rotData.X.push_back(0);
	//	m_rotData.Y.push_back(0);
	//	m_rotData.Z.push_back(0);
	//}

	vector<double> soundVelocityOneSideY;
	vector<double> soundVelocityOneSideX;
	//nmGenDerivativeCurve(&dataVelocity, &(trajData.Y), 1 / 42000.0, true);
	nmGenDerivativeCurve(&soundVelocityOneSideY, &(trajData.Y), 1 / SAMPLES_PER_SECOND, true);
	nmGenDerivativeCurve(&soundVelocityOneSideX, &(trajData.X), 1 / SAMPLES_PER_SECOND, true);
	//nmGenDerivativeCurve(&m_soundAcceleration, &dataVelocity, 1 / 42000.0, true);

	//split the music data to both ears (left and right with the given ITD).
	m_soundVelocity.clear();
	for (int i = 0; i < soundVelocityOneSideY.size(); i++)
	{
		m_soundVelocity.push_back(sqrt(pow(soundVelocityOneSideY[i], 2) + pow(soundVelocityOneSideX[i], 2)));
	}

	return amp;
}

double MoogDotsCom::CalculateITD(double azimuth, double frequency)
{
	double headRadius = 0.1; //in meters or in cm?
	double ITD = 3.0 / (C_SOUND)* headRadius * sin(azimuth);	//azimuth is in radians.

	return ITD;
}

double MoogDotsCom::CalculateIID(double azimuth, double frequency)
{
	double IID = 1.0 + pow((frequency / 1000), 0.8) * sin(azimuth);

	return IID;
}

double MoogDotsCom::ITD2Offset(double ITD)
{
	return (double)(SAMPLES_PER_SECOND * ITD);
}

double* MoogDotsCom::ChooseSoundWaveByType(SOUND_WAVE_TYPE soundWaveType)
{
	double* waveSound = { FLUTE_C_SOUND };

	switch (soundWaveType)
	{
	case FLUTE_C_SOUND_TYPE:
		waveSound = { FLUTE_C_SOUND };
		break;
	case REED_ORGAN_LOW_G_TYPE:
		waveSound = { REED_ORGAN_LOW_G };
		break;
	case FLUTE_HIGH_G_TYPE:
		waveSound = { FLUTE_HIGH_G };
		break;
	case FLUTE_LOW_G_TYPE:
		waveSound = { FLUTE_LOW_G };
		break;
	case REED_ORGAN_C_TYPE:
		waveSound = { REED_ORGAN_C };
		break;
	case REED_ORGAN_HIGH_G_TYPE:
		waveSound = { REED_ORGAN_HIGH_G };
	default:
		waveSound = { FLUTE_C_SOUND };
		break;
	}

	return waveSound;
}

void MoogDotsCom::CreateSoundVector(vector<double> acceleration, double azimuth, SOUND_WAVE_TYPE soundType, WORD* &  resultData)
{
	//The data to the board goes interlreaved by LRLRLRLRLRLRLRLRLRLR etc.
	//WORD* ADData = new WORD[(int)SAMPLES_PER_SECOND * TIME * 2];		//the data would return to tranfer to the board.
	double* ADDataDouble = new double[(int)SAMPLES_PER_SECOND * TIME * 2];//the data before converting it to the WORD type.
	vector<double> debugData;;

	double* leftChannelData;
	double* rightChannelData;
	unsigned dataSize;

	azimuth = g_pList.GetVectorData("DISC_AMPLITUDES").at(0);
	_matSoundStimReader->ReadStruct(azimuth, "stim", leftChannelData, rightChannelData, dataSize , false);

	for (int i = 0; i < dataSize; i++)
	{
		double val = ((leftChannelData[i])) * USHORT_MAX_HALF * MAX_VOLUME+ USHORT_MAX_HALF;
		ADDataDouble[2 * i] = val;
		debugData.push_back(val);
		val = ((rightChannelData[i])) * USHORT_MAX_HALF * MAX_VOLUME + USHORT_MAX_HALF;
		ADDataDouble[2 * i + 1] = val;

		resultData[2 * i] = (WORD)((ADDataDouble[2 * i] - USHORT_MAX_HALF) + USHORT_MAX_HALF);
		resultData[2 * i + 1] = (WORD)((ADDataDouble[2 * i + 1] - USHORT_MAX_HALF) + USHORT_MAX_HALF);
	}

	/*
	int i = 0;

	const double* waveSound = ChooseSoundWaveByType(soundType);

	int itdOffset = ITD2Offset(CalculateITD(abs(azimuth), MAIN_FREQ));
	double IID = CalculateIID(abs(azimuth), MAIN_FREQ);

	//counts the numbers of indexes given zeros and given not zeros at each cycle of sound-silence-sound-silence etc.
	int zeros2100 = 0;

	//for left heading direction.
	if (azimuth < 0)
	{
		//add values to the left ear.
		for (int i = 0; i < acceleration.size()-1; i += 1)
		{
			double val = ((waveSound[i])) * USHORT_MAX_HALF * acceleration[i]/ACCELERATION_AMPLITUDE_NORMALIZATION  + USHORT_MAX_HALF;
			ADData[2 * i] = (WORD)val;
			ADDataDouble[2 * i] = val;
			debugData.push_back(val);
		}

		//copy the values for the right ear with a delay and with negative gain.
		int j = 0;
		for (int i = 0; i < acceleration.size()-1; i += 1)
		{
			if (i < itdOffset)
			{
				ADData[2 * i + 1] = (WORD)USHORT_MAX_HALF;
			}
			else
			{
				ADData[2 * i + 1] = (WORD)((ADDataDouble[j] - USHORT_MAX_HALF) / IID + USHORT_MAX_HALF);
				j += 2;
			}
		}
	}

	//for right heading direction.
	if (azimuth > 0)
	{
		for (int i = 0; i < acceleration.size()-1; i += 1)
		{
			double val = (waveSound[i]) * USHORT_MAX_HALF * acceleration[i] / ACCELERATION_AMPLITUDE_NORMALIZATION + USHORT_MAX_HALF;
			ADData[2 * i + 1] = (WORD)(val);
			ADDataDouble[2 * i + 1] = val;
			debugData.push_back(val);
			//zeros the 2100 samples for the silence in the round.
		}

		//copy the values for the left ear with a delay and with negative gain.
		int j = 1;
		for (int i = 0; i < acceleration.size()-1; i += 1)
		{
			if (i < itdOffset)
			{
				ADData[2 * i] = (WORD)USHORT_MAX_HALF;
			}
			else
			{
				ADData[2 * i] = (WORD)((ADDataDouble[j] - USHORT_MAX_HALF) / IID + USHORT_MAX_HALF);
				j += 2;
			}
		}
	}
	*/

	debugData.clear();
	delete[] leftChannelData;
	delete[] rightChannelData;
	delete []ADDataDouble;

	//return ADData;
}

void MoogDotsCom::CreateSoundVectorInverse(WORD* &resultData)
{
	double azimuth = azimuth = g_pList.GetVectorData("DISC_AMPLITUDES").at(0);

	//The data to the board goes interlreaved by LRLRLRLRLRLRLRLRLRLR etc.
	WORD* ADData = new WORD[(int)(SAMPLES_PER_SECOND * TIME * 2 * 2.5)];		//the data would return to tranfer to the board.
	double*  ADDataDouble = new double[(int)(SAMPLES_PER_SECOND * TIME * 2 * 2.5)];//the data before converting it to the WORD type.
	vector<double> debugData;;

	double* leftChannelData;
	double* rightChannelData;
	unsigned int dataSize;


	azimuth = g_pList.GetVectorData("DISC_AMPLITUDES").at(0);
	_matSoundStimReader->ReadStruct(azimuth, "stim", leftChannelData, rightChannelData, dataSize, true);

	for (int i = 0; i < dataSize; i++)
	{
		double val = ((leftChannelData[i])) * USHORT_MAX_HALF * MAX_VOLUME + USHORT_MAX_HALF;
		ADDataDouble[2 * i] = val;
		debugData.push_back(val);
		val = ((rightChannelData[i])) * USHORT_MAX_HALF * MAX_VOLUME + USHORT_MAX_HALF;
		ADDataDouble[2 * i + 1] = val;

		resultData[2 * i] = (WORD)((ADDataDouble[2 * i] - USHORT_MAX_HALF) + USHORT_MAX_HALF);
		resultData[2 * i + 1] = (WORD)((ADDataDouble[2 * i + 1] - USHORT_MAX_HALF) + USHORT_MAX_HALF);
	}

	debugData.clear();

	delete[] leftChannelData;
	delete[] rightChannelData;
	delete []ADDataDouble;

	//return ADData;
}

void MoogDotsCom::PlaySoundThread(WORD* soundData)
{
	//TIME seconds of sine wave in the freq SAMPLES_PER_SECOND and stereo (2).
	long sampleRate = SAMPLES_PER_SECOND;

	WRITE_LOG(m_logger->m_logger, "Playing sound thread for trial # " << m_trialNumber << " starts.");

	short ULStat = cbAOutScan(m_USB_3101FS_AO_Object.DIO_board_num, LOW_CHANNEL, HIGH_CHANNEL, sampleRate * TIME * 2, &sampleRate, GAIN, soundData, OPTIONS);
}

void MoogDotsCom::PlaySoundThreadInverse(WORD* soundData)
{
	//TIME seconds of sine wave in the freq SAMPLES_PER_SECOND and stereo (2).
	long sampleRate = SAMPLES_PER_SECOND;

	WRITE_LOG(m_logger->m_logger, "Playing sound thread inverse for trial # " << m_trialNumber << " starts.");

	short ULStat = cbAOutScan(m_USB_3101FS_AO_Object.DIO_board_num, LOW_CHANNEL, HIGH_CHANNEL, sampleRate * TIME * 2 * 2.5, &sampleRate, GAIN, soundData, OPTIONS);
}

thread MoogDotsCom::MoveMBCThread(bool moveBtMoogdotsTraj)
{
	//open the thread for moving the MBC according to the m_data positions (and than the main - this function would countinue in parallel to that which mean that the Oculus would render in parallel to the MBC commands communication.
	thread t(&MoogDotsCom::SendMBCFrameThread, this, m_data.X.size());
	//t.detach();
	/*
	Sleep(2) because the comunication as a begining only delay of 1ms and for the timetransmission(truely ,it was a experimental time for making the Oculus render the the image at the middle of the 8/16 motion points .
	8/16 points before the render for the 16 of the current image and the rest 8/16 points for same current image.
	*/
	Sleep(2);

	return t;
}

void MoogDotsCom::SendMBCFrameThread(int data_size)
{
	//setting the sending to the sender loop in MBC_Interface to be time critical.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	m_trialNumber = g_pList.GetVectorData("Trial").at(0);

	if (!m_startButtonGoToOriginCommand)
	{
		WRITE_LOG(m_logger->m_logger, "Sending MBC frame thread for trial # " << m_trialNumber << " starts.");
	}
	else
	{
		WRITE_LOG(m_logger->m_logger, "Sending MBC frame thread for start button experiment " << m_trialNumber << ".");
	}

	int start = clock();

	if (m_forwardMovement && !m_startButtonGoToOriginCommand)
	{  //send the trial number LSB to the EEG.
		m_trialNumber = g_pList.GetVectorData("Trial").at(0);

		if (g_pList.GetVectorData("LPT_DATA_SEND").at(0))
		{
			//send the trial number start indication the EEG.
			m_EEGLptContoller->Write(LPT_PORT, 0x01);
			WRITE_LOG(m_logger->m_logger, "Sending the EEG start indication of data 0x01.");

			//write the full trial number to the log file.
			WRITE_LOG_PARAM(m_logger->m_logger, "Writing to the trial number", m_trialNumber);

			thread t1(&MoogDotsCom::ResetEEGPins, this, m_trialNumber);
			t1.detach();
		}
	}

	//todo: add here the data for the robot agian (when robot would work) and add it to the interpolation parameter and not the m_data because in the line : if (m_data.index < static_cast<int>(m_data.X.size())).
	if (data_size >= 60/* && !(m_moveByMoogdotsTrajectory && m_forwardMovement)*/)
	{
		MoogFrame* lastSentFrame;
		int interpolatedFreezeFrameIndex = _freezeFrameIndex * INTERPOLATION_UPSAMPLING_SIZE;

		//todo:zero the freeze freame index.
		for (int mbcFrameIndex = 0; mbcFrameIndex < INTERPOLATION_UPSAMPLING_SIZE * (data_size - 1) - 1 && !_trialAborted; mbcFrameIndex++)
		{
			if (mbcFrameIndex < m_interpolatedData.X.size())
			{
				DATA_FRAME moogFrame;

				if (interpolatedFreezeFrameIndex == mbcFrameIndex && m_forwardMovement)
				{
					g_pList.SetVectorData("DO_MOVEMENT_FREEZE", vector<double>(1, 0));
					WRITE_LOG(m_logger->m_logger, "Tagging _waitForSecondResponse as true.");
					wxString s = wxString::Format("&&&&&&&&&& Tagging _waitForSecondResponse as true. &&&&&&&&&&&&");
					m_messageConsole->InsertItems(1, &s, 0);
					_waitForSecondResponse = true;

					//critical section for the changing whem movePlatformToOrigin is coming and _trialAborted is being reset.
					EnterCriticalSection(&m_TrialAborted);

					do
					{
						if (g_pList.GetVectorData("DO_MOVEMENT_FREEZE").at(0) == 3)
						{
							WRITE_LOG(m_logger->m_logger, "Tagging _waitForSecondResponse as false.")
								_waitForSecondResponse = false;
							wxString s = wxString::Format("&&&&&&&&&& Tagging _waitForSecondResponse as false. &&&&&&&&&&&&");
							m_messageConsole->InsertItems(1, &s, 0);

							g_pList.SetVectorData("DO_MOVEMENT_FREEZE", vector<double>(1, 0));
						}

						if (g_pList.GetVectorData("DO_MOVEMENT_FREEZE").at(0) == 4)
						{
							_trialAborted = true;

							_waitForSecondResponse = false;

							g_pList.SetVectorData("DO_MOVEMENT_FREEZE", vector<double>(1, 0));
						}

						//if the trial is aborted , no need to send the data, also , the data is concurrently cleared by the UpdateMovement which clear it (race condition can be here).
						if (!_trialAborted)
						{
							EnterCriticalSection(&m_CS);
							moogFrame.lateral = static_cast<double>(m_interpolatedData.X.at((interpolatedFreezeFrameIndex)));
							moogFrame.surge = static_cast<double>(m_interpolatedData.Y.at((interpolatedFreezeFrameIndex)));
							moogFrame.heave = static_cast<double>(m_interpolatedData.Z.at((interpolatedFreezeFrameIndex))) + MOTION_BASE_CENTER;
							moogFrame.yaw = static_cast<double>(m_interpolatedRotData.X.at((interpolatedFreezeFrameIndex)));
							moogFrame.pitch = static_cast<double>(m_interpolatedRotData.Y.at((interpolatedFreezeFrameIndex)));
							moogFrame.roll = static_cast<double>(m_interpolatedRotData.Z.at((interpolatedFreezeFrameIndex)));
							lastSentFrame = &moogFrame;

							//send the same frame (freeze data frame).
							SET_DATA_FRAME(&moogFrame);
							LeaveCriticalSection(&m_CS);
						}

					} while (_waitForSecondResponse);

					LeaveCriticalSection(&m_TrialAborted);
				}

				else
				{
					EnterCriticalSection(&m_CS);

					if (mbcFrameIndex > 0)
					{
						/*if (!CheckMoogAtCorrectPosition(lastSentFrame, 0.01))
						break;*/
					}

					moogFrame.lateral = static_cast<double>(m_interpolatedData.X.at((mbcFrameIndex)));
					moogFrame.surge = static_cast<double>(m_interpolatedData.Y.at((mbcFrameIndex)));
					moogFrame.heave = static_cast<double>(m_interpolatedData.Z.at((mbcFrameIndex))) + MOTION_BASE_CENTER;
					moogFrame.yaw = static_cast<double>(m_interpolatedRotData.X.at((mbcFrameIndex)));
					moogFrame.pitch = static_cast<double>(m_interpolatedRotData.Y.at((mbcFrameIndex)));
					moogFrame.roll = static_cast<double>(m_interpolatedRotData.Z.at((mbcFrameIndex)));
					lastSentFrame = &moogFrame;
					SET_DATA_FRAME(&moogFrame);
					LeaveCriticalSection(&m_CS);
				}

#pragma region LOG-FRAME_MBC_TIME
				double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
				WRITE_LOG_PARAM3(m_logger->m_logger, "Command frame sent to the MBC.", mbcFrameIndex, moogFrame.surge, time);
#pragma endregion LOG-FRAME_MBC_TIME

#if USE_MATLAB_DEBUG_GRAPHS
				m_debugPlace.push_back(moogFrame.surge);
				m_debugPlaceTime.push_back(time);
#endif //USE_MATLAB_DEBUG_GRAPHS
			}
		}
		//send the Matlab indication that the robot
	}
	else
	{
		WRITE_LOG_PARAM(m_logger->m_logger, "Error occured - the number of frames was to low.", data_size);
	}

	if (!m_startButtonGoToOriginCommand)
	{
		//send the trial number MSB at the end of the forward movement.
		if (m_forwardMovement)
		{
			if (g_pList.GetVectorData("LPT_DATA_SEND").at(0))
			{
				//send the trial number end indication the EEG.
				m_EEGLptContoller->Write(LPT_PORT, 0x07);
				WRITE_LOG(m_logger->m_logger, "Sending the EEG end indication of data 0x07.");
				
				//reset the bits
				Sleep(10);
				m_EEGLptContoller->Write(LPT_PORT, 0);
				WRITE_LOG(m_logger->m_logger, "Sending the  number reset round.");
			}
		}

		//if this is the second time the thread has finished to the same step (the forward movement is ended) , than turn the m_finishedMovingBackward = true.
		if (m_forwardMovement == false)
		{
			m_finishedMovingBackward = true;
			WRITE_LOG(m_logger->m_logger, "Changing m_forwardMovement from false to true.");
			m_forwardMovement = true;
		}

		else
		{
			//if not 2nd interval with aborted
			if (!_trialAborted)
			{
				//save the final point in the forward trajectory.
				m_finalForwardMovementPosition.lateral = m_data.X.at(m_data.X.size() - 1);
				m_finalForwardMovementPosition.surge = m_data.Y.at(m_data.Y.size() - 1);
				m_finalForwardMovementPosition.heave = m_data.Z.at(m_data.Z.size() - 1);
				//need to convert from deg2rad because they come from the Matlab which retuns them with rad units.
				if (!m_moveByMoogdotsTrajectory || !m_forwardMovement)
				{
					m_finalForwardMovementPosition.yaw = deg2rad(m_rotData.X.at(m_rotData.X.size() - 1));
					m_finalForwardMovementPosition.pitch = deg2rad(m_rotData.Y.at(m_rotData.Y.size() - 1));
					m_finalForwardMovementPosition.roll = deg2rad(m_rotData.Z.at(m_rotData.Z.size() - 1));
				}
				//not need to convert from deg2rad because they come from the MoogCreate which retuns them with rad units.
				else
				{
					m_finalForwardMovementPosition.yaw = m_rotData.X.at(m_rotData.X.size() - 1);
					m_finalForwardMovementPosition.pitch = m_rotData.Y.at(m_rotData.Y.size() - 1);
					m_finalForwardMovementPosition.roll = m_rotData.Z.at(m_rotData.Z.size() - 1);
				}
			}
			else
			{
				int freezedFrameIndeex = _freezeFrameIndex;

				//save the final point in the forward trajectory.
				m_finalForwardMovementPosition.lateral = m_data.X.at(freezedFrameIndeex - 1);
				m_finalForwardMovementPosition.surge = m_data.Y.at(freezedFrameIndeex - 1);
				m_finalForwardMovementPosition.heave = m_data.Z.at(freezedFrameIndeex - 1);
				//need to convert from deg2rad because they come from the Matlab which retuns them with rad units.
				if (!m_moveByMoogdotsTrajectory || !m_forwardMovement)
				{
					m_finalForwardMovementPosition.yaw = deg2rad(m_rotData.X.at(freezedFrameIndeex - 1));
					m_finalForwardMovementPosition.pitch = deg2rad(m_rotData.Y.at(freezedFrameIndeex - 1));
					m_finalForwardMovementPosition.roll = deg2rad(m_rotData.Z.at(freezedFrameIndeex - 1));
				}
				//not need to convert from deg2rad because they come from the MoogCreate which retuns them with rad units.
				else
				{
					m_finalForwardMovementPosition.yaw = m_rotData.X.at(freezedFrameIndeex - 1);
					m_finalForwardMovementPosition.pitch = m_rotData.Y.at(freezedFrameIndeex - 1);
					m_finalForwardMovementPosition.roll = m_rotData.Z.at(freezedFrameIndeex - 1);
				}
			}

			//reset the m_forwardMovement flag to false because now the MBC finishe to move the forward movement and may start the backward movement (GO To Origin (MovePlatformToOrigin)).
			WRITE_LOG(m_logger->m_logger, "Changing m_forwardMovement from something to false.");
			m_forwardMovement = false;
		}

		m_moveByMoogdotsTrajectory = false;
	}

	//reset the flah after the experiment start (and even after each trial).
	m_startButtonGoToOriginCommand = false;

	WRITE_LOG(m_logger->m_logger, "Sending MBC frame thread experiment ended. " << m_trialNumber << ".");
}

bool MoogDotsCom::CheckMoogAtCorrectPosition(MoogFrame* position, double maxDistanceError)
{
	MoogFrame feedbackPosition = this->GetAxesFeedbackPosition();

#pragma region LOG_CHECKING_POSITION
	WRITE_LOG(m_logger->m_logger, "Checking robot is at position " << position->heave << "where feedback position is - heave = " << feedbackPosition.heave);
	WRITE_LOG(m_logger->m_logger, "Checking robot is at position " << position->lateral << "where feedback position is - lateral = " << feedbackPosition.lateral);
	WRITE_LOG(m_logger->m_logger, "Checking robot is at position " << position->pitch << "where feedback position is - pitch = " << feedbackPosition.pitch);
	WRITE_LOG(m_logger->m_logger, "Checking robot is at position " << position->roll << "where feedback position is - roll = " << feedbackPosition.roll);
	WRITE_LOG(m_logger->m_logger, "Checking robot is at position " << position->surge << "where feedback position is - surge = " << feedbackPosition.surge);
	WRITE_LOG(m_logger->m_logger, "Checking robot is at position " << position->yaw << "where feedback position is - yaw = " << feedbackPosition.yaw);
#pragma endregion LOG_CHECKING_POSITION

	if (abs(feedbackPosition.heave - position->heave) > maxDistanceError)
		return false;

	if (abs(feedbackPosition.lateral - position->lateral) > maxDistanceError)
		return false;

	if (abs(feedbackPosition.pitch - position->pitch) > maxDistanceError)
		return false;

	if (abs(feedbackPosition.roll - position->roll) > maxDistanceError)
		return false;

	if (abs(feedbackPosition.surge - position->surge) > maxDistanceError)
		return false;

	if (abs(feedbackPosition.yaw - position->yaw) > maxDistanceError)
		return false;

	return true;
}

bool MoogDotsCom::CheckMoogAtOrigin(double maxDifferentialError)
{
	WRITE_LOG(m_logger->m_logger, "Checking robot is at origin position.");

	MoogFrame* originPosition = new MoogFrame();
	/*originPosition->heave = MOTION_BASE_CENTER;
	originPosition->lateral = 0;
	originPosition->pitch = 0;
	originPosition->roll = 0;
	originPosition->surge = 0;
	originPosition->yaw = 0;*/

	vector<double> transOrigin = g_pList.GetVectorData("M_ORIGIN");
	vector<double>	rotOrigin = g_pList.GetVectorData("ROT_ORIGIN");
	originPosition->heave = MOTION_BASE_CENTER;
	originPosition->lateral = (transOrigin.at(0));
	originPosition->pitch = (rotOrigin.at(1));
	originPosition->roll = (rotOrigin.at(2));
	originPosition->surge = (transOrigin.at(1));;
	originPosition->yaw = (rotOrigin.at(0));

	return CheckMoogAtCorrectPosition(originPosition, maxDifferentialError);
}

bool MoogDotsCom::CheckMoogAtFinal(double maxDifferentialError)
{
	WRITE_LOG(m_logger->m_logger, "Checking robot is at final position.");

	MoogFrame* finalForwardPosition = new MoogFrame();
	finalForwardPosition->heave = m_finalForwardMovementPosition.heave + MOTION_BASE_CENTER;
	finalForwardPosition->lateral = m_finalForwardMovementPosition.lateral;
	finalForwardPosition->pitch = m_finalForwardMovementPosition.pitch;
	finalForwardPosition->roll = m_finalForwardMovementPosition.roll;
	finalForwardPosition->surge = m_finalForwardMovementPosition.surge;
	finalForwardPosition->yaw = m_finalForwardMovementPosition.yaw;

	//first chance to be at the correct position.
	if (CheckMoogAtCorrectPosition(finalForwardPosition, maxDifferentialError))
	{
		return true;
	}
	//wait for the next chance.
	Sleep(CORRECT_POSITION_CHANCE_TIME);

	//second chance to be at the correct position.
	WRITE_LOG(m_logger->m_logger, "Second chance - Checking robot is at final position.");
	if (CheckMoogAtCorrectPosition(finalForwardPosition, maxDifferentialError))
	{
		return true;
	}
	//wait for the next chance.
	Sleep(CORRECT_POSITION_CHANCE_TIME);

	//third and last chance to be at the correct position.
	WRITE_LOG(m_logger->m_logger, "Third chance - Checking robot is at final position.");
	return CheckMoogAtCorrectPosition(finalForwardPosition, maxDifferentialError);
}

bool MoogDotsCom::CheckMoogAtCorrectPosition(double maxDifferentialError)
{
	double stimType = g_pList.GetVectorData("STIMULUS_TYPE").at(0);

	WRITE_LOG(m_logger->m_logger, "StimulusType = " << stimType);

	//if visual only and there is no movement return true whatever be with the Moog.
	if (stimType == 2.0			//visual only.
		|| stimType == 7.0		//visual only with left prior.
		|| stimType == 10.0		//visual only with right prior.
		|| stimType == 100.0	//sound only.
		|| stimType == 120.0	//visual with sound only.
		|| stimType == 124.0	//visual with sound only (delta+ to visual).
		|| stimType == 125.0)	//visual with sound only (delta- to visual).
	{
		return true;
	}
	//check robot is at origin before moving forward.
	if (m_forwardMovement)
	{
		WRITE_LOG(m_logger->m_logger, "Checking robot is at origin position.");
		//if not at the origin show the error window and exit the function.
		if (!CheckMoogAtOrigin(maxDifferentialError))
		{
			wxWindow* erroeWindow = new wxWindow();
			erroeWindow->Show();

			wxMessageDialog d(erroeWindow, "The Moog is not at the origin position.");
			d.SetFocus();
			d.ShowModal();

			WRITE_LOG(m_logger->m_logger, "Moog is not at origin stopping the system.");

			return false;
		}
	}
	//check robot is at final before moving backward.
	else
	{
		WRITE_LOG(m_logger->m_logger, "Checking robot is at final position.");
		//if not at the origin show the error window and exit the function.
		if (!CheckMoogAtFinal(maxDifferentialError))
		{
			wxWindow* erroeWindow = new wxWindow();
			erroeWindow->Show();

			wxMessageDialog d(erroeWindow, "The Moog is not at the final position.");
			d.ShowModal();
			d.SetFocus();

			WRITE_LOG(m_logger->m_logger, "Moog is not at final position stopping the system.");

			return false;
		}
	}

	return true;
}

void MoogDotsCom::UpdateStatusesMembers()
{
	//avi:
	//the trial render time is finshed, but m_trial_finished is for freezing the last render for some moments.
	if (m_trial_finished)
	{
		m_waiting_a_little_after_finished = true;
		m_firstInOnlyFixationPoint = true;
	}
	m_trial_finished = true;
	//sending the eyes motion of all the frames during the trial to the matlab.
	if (m_waiting_a_little_after_finished)
	{
		m_oculusIsOn = true;
	}
}

#if !MINI_MOOG_SYSTEM
void MoogDotsCom::ReceiveCompute()
{
	// Get the latest return frame.
	DATA_FRAME returnFrame;
	returnFrame.heave = ThreadGetReturnedHeave();
	returnFrame.lateral = ThreadGetReturnedLateral();
	returnFrame.surge = ThreadGetReturnedSurge();
	returnFrame.yaw = ThreadGetReturnedYaw();
	returnFrame.pitch = ThreadGetReturnedPitch();
	returnFrame.roll = ThreadGetReturnedRoll();

#if RECORD_MODE
	// If we're actively putting movement data into the command buffer, store the
	// return data.  That is, if we're supposed to.
	int tmp = static_cast<int>(m_data.X.size());
	if (m_data.index > m_recordOffset + 0 && m_recordIndex < static_cast<int>(m_data.X.size()) - m_recordOffset + 11) {
		// Record the receive time of the return packet.
		m_receiveStamp.push_back(ThreadGetReceiveTime());

		m_recordIndex++;

		// We have to subtract off the startpoint of the Gaussian so that the recorded
		// data will always start from 0.
		m_recordedLateral.push_back((returnFrame.lateral - m_data.X[m_recordOffset])*100.0);
		m_recordedHeave.push_back((returnFrame.heave - m_data.Z[m_recordOffset])*100.0 + HEAVE_OFFSET);
		m_recordedSurge.push_back((returnFrame.surge - m_data.Y[m_recordOffset])*100.0);
		m_recordedYaw.push_back(returnFrame.yaw);
		m_recordedPitch.push_back(returnFrame.pitch);
		m_recordedRoll.push_back(returnFrame.roll);
	}
#else // #if RECORD_MODE
	if (m_drawRegularFeedback) {
		// Set the camera position.
		if (m_glWindowExists) {
			m_glWindow->GetGLPanel()->SetLateral(returnFrame.lateral*100.0);
			m_glWindow->GetGLPanel()->SetSurge(returnFrame.surge*100.0);
			m_glWindow->GetGLPanel()->SetHeave(returnFrame.heave*100.0 + HEAVE_OFFSET);

			m_glWindow->GetGLPanel()->Render();

#if !CUSTOM_TIMER
			// Swap the buffers.
			//wglMakeCurrent((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC(), m_threadGLContext);
			SwapBuffers((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC());
#endif
		}
	}
#endif // #if RECORD_MODE

#if USE_MATLAB && !RECORD_MODE
	m_receiveStamp.push_back(ThreadGetReceiveTime());
#endif

	// protect CED and make sure it doesn't set more than 5V - Johnny
	//	if ((returnFrame.lateral)*100.0 < 25.0)
	cbAOut(m_PCI_DIO48H_Object.DIO_board_num, 0, BIP10VOLTS, DASCALE((returnFrame.lateral)*100.0));
	//	if ((returnFrame.heave)*100.0 + HEAVE_OFFSET < 25.0)
	cbAOut(m_PCI_DIO48H_Object.DIO_board_num, 1, BIP10VOLTS, DASCALE((returnFrame.heave)*100.0 + HEAVE_OFFSET));
	//	if ((returnFrame.surge)*100.0 < 25.0)
	cbAOut(m_PCI_DIO48H_Object.DIO_board_num, 2, BIP10VOLTS, DASCALE((returnFrame.surge)*100.0));
	//	if ((returnFrame.yaw)*100.0 < 25.0)
	cbAOut(m_PCI_DIO48H_Object.DIO_board_num, 3, BIP10VOLTS, DASCALE(returnFrame.yaw));
	//	if ((returnFrame.pitch)*100.0 < 25.0)
	cbAOut(m_PCI_DIO48H_Object.DIO_board_num, 4, BIP10VOLTS, DASCALE(returnFrame.pitch));
	//	if ((returnFrame.roll)*100.0 < 25.0)
	cbAOut(m_PCI_DIO48H_Object.DIO_board_num, 5, BIP10VOLTS, DASCALE(returnFrame.roll));

} // ReceiveCompute()
#endif


void MoogDotsCom::CustomTimer()
{
#if CUSTOM_TIMER
#if SWAP_TIMER
	LARGE_INTEGER t;

	// Time stamp the SwapBuffers() call.
	QueryPerformanceCounter(&t);
	m_swapStamp.push_back((double)t.QuadPart / (double)m_freq.QuadPart * 1000.0);
#endif

	// Swap the buffers.
	wglMakeCurrent((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC(), m_threadGLContext);
	SwapBuffers((HDC)m_glWindow->GetGLPanel()->GetContext()->GetHDC());

	// Send out a sync pulse.
	if (m_doSyncPulse == true) {
		cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 1);
		cbDOut(PULSE_OUT_BOARDNUM, FIRSTPORTB, 0);
	}
#endif
}


string MoogDotsCom::replaceInvalidChars(string s)
{
	int i;

	for (i = 0; i < static_cast<int>(s.length()); i++) {
		switch (s[i]) {
		case '-':
			s[i] = 'n';
			break;
		case '.':
			s[i] = 'd';
			break;
		}
	}

	return s;
}


void MoogDotsCom::MovePlatformToOrigin()
{
	WRITE_LOG(m_logger->m_logger, "Moving platform to the origin...");

	vector<double> zeroVector;
	zeroVector.push_back(0.0);

	// Generate a frame to represent the origin.
	vector<double> transOrigin = g_pList.GetVectorData("M_ORIGIN"),
		rotOrigin = g_pList.GetVectorData("ROT_ORIGIN");
	DATA_FRAME startFrame;
	startFrame.lateral = (transOrigin.at(0));
	startFrame.surge = (transOrigin.at(1));
	startFrame.heave = (transOrigin.at(2));
	startFrame.yaw = (rotOrigin.at(0));
	startFrame.pitch = (rotOrigin.at(1));
	startFrame.roll = (rotOrigin.at(2));

	// Calculates the actual trajectory to origin.
	MovePlatform(&startFrame);

	// Store the length of the data structure holding the movement to
	// origin.  This let's us know when to start setting the OpenGL data.
	m_recordOffset = static_cast<int>(m_data.X.size());

	// Make sure that we don't execute the code to go to the origin, and
	// also the code to calculate the movement trajectory.
	g_pList.SetVectorData("GO_TO_ORIGIN", zeroVector);
	g_pList.SetVectorData("DO_MOVEMENT", zeroVector);
}


void MoogDotsCom::MovePlatform(DATA_FRAME *destination)
{
	WRITE_LOG_PARAM(m_logger->m_logger, "Moving platform.", destination->surge);

	//for race condition with the trial aborted (can be concurrently to the moving of the last point before the retuen command).
	EnterCriticalSection(&m_CS);
	// Empty the data vectors, which stores the trajectory data.
	nmClearMovementData(&m_data);
	nmClearMovementData(&m_rotData);

	//avi : interpolation version
	nmClearMovementData(&m_interpolatedData);
	nmClearMovementData(&m_interpolatedRotData);
	LeaveCriticalSection(&m_CS);

	// Get the positions currently in the command buffer.  We use the thread safe
	// version of GetAxesPositions() here because MovePlatform() is called from
	// both the main GUI thread and the communication thread.
	DATA_FRAME currentFrame;
	this->GetAxesCommandPosition(&currentFrame);

	// We assume that the heave value passed to us is based around zero.  We must add an offset
	// to that value to adjust for the Moog's inherent offset on the heave axis.
	currentFrame.heave -= MOTION_BASE_CENTER;

	// Check to see if the motion base's current position is the same as the startPosition.  If so,
	// we don't need to move the base into position.
	if (fabs(destination->lateral - currentFrame.lateral) > TINY_NUMBER ||
		fabs(destination->surge - currentFrame.surge) > TINY_NUMBER ||
		fabs(destination->heave - currentFrame.heave) > TINY_NUMBER)
	{
		// Move the platform from its current location to start position.
		nm3DDatum sp, ep;
		sp.x = currentFrame.lateral; sp.y = currentFrame.surge; sp.z = currentFrame.heave;
		ep.x = destination->lateral; ep.y = destination->surge; ep.z = destination->heave;
		//nmGen3DVGaussTrajectory(&m_data, sp, ep, 2.0, 60.0, 3.0, false);
		//Changed the above from 2 sec to 2.5 in an attempt to minimize bumping -- Tunde 12/01/09
		nmGen3DVGaussTrajectory(&m_data, sp, ep, 2.5, 60.0, 3.0, false);
	}
	else
	{
		//here need to make changes for forward and backward boolean movements due to no entereance in the Compute() function -  m_data.X.size() == 0.
		WRITE_LOG(m_logger->m_logger, "Changing m_forwardMovement from something to true.");
		m_zero_length_m_data_size_trajectory = true;
		m_forwardMovement = true;
		m_finishedMovingBackward = true;
		_trialAborted = false;
	}

	// Make sure that we're not rotated at all.
	if (fabs(destination->yaw - currentFrame.yaw) > TINY_NUMBER ||
		fabs(destination->pitch - currentFrame.pitch) > TINY_NUMBER ||
		fabs(destination->roll - currentFrame.roll) > TINY_NUMBER)
	{
		// Set the Yaw.
		//avi : Changed the above from 2 sec to 2.5 in an attempt to be the same size as m_data.
		nmGen1DVGaussTrajectory(&m_rotData.X, destination->yaw - currentFrame.yaw, 2.5, 60.0, 3.0, currentFrame.yaw, false);

		// Set the Pitch.
		//avi : Changed the above from 2 sec to 2.5 in an attempt to be the same size as m_data.
		nmGen1DVGaussTrajectory(&m_rotData.Y, destination->pitch - currentFrame.pitch, 2.5, 60.0, 3.0, currentFrame.pitch, false);

		// Set the Roll.
		//avi : Changed the above from 2 sec to 2.5 in an attempt to be the same size as m_data.
		nmGen1DVGaussTrajectory(&m_rotData.Z, destination->roll - currentFrame.roll, 2.5, 60.0, 3.0, currentFrame.roll, false);
	}

	// Now we make sure that data in m_data and data in m_rotData has the same length.
	int dataSize = m_data.X.size();
	int rotDataSize = m_rotData.X.size();
	if (m_data.X.size() > m_rotData.X.size()) {
		for (int i = 0; i < (int)m_data.X.size(); i++) {
			m_rotData.X.push_back(currentFrame.yaw);
			m_rotData.Y.push_back(currentFrame.pitch);
			m_rotData.Z.push_back(currentFrame.roll);
		}
		WRITE_LOG(m_logger->m_logger, "m_data.X.size() > m_rotData.X.size()");
	}
	else if (m_data.X.size() < m_rotData.X.size()) {
		for (int i = 0; i < (int)m_rotData.X.size(); i++) {
			m_data.X.push_back(currentFrame.lateral);
			m_data.Y.push_back(currentFrame.surge);
			m_data.Z.push_back(currentFrame.heave);
		}
		WRITE_LOG(m_logger->m_logger, "m_data.X.size() < m_rotData.X.size()");
	}

	////////////////////////////////////////////////////////////////////////////////////////interpolated version//////////////////////////////////////////////////////////////////////////
	std::vector<double> X;
	tk::spline sX;
	tk::spline sY;
	tk::spline sZ;
	int minLength = m_data.X.size();

	if (minLength > 0)
	{
		std::vector<double> X;
		tk::spline sX;
		tk::spline sY;
		tk::spline sZ;

		for (int i = 0; i < minLength; i++)
		{
			X.push_back(i* INTERPOLATION_WIDE * INTERPOLATION_UPSAMPLING_SIZE);
		}

		sX.set_points(X, m_data.X, true);    // currently it is required that X is already sorted
		sY.set_points(X, m_data.Y, true);    // currently it is required that X is already sorted
		sZ.set_points(X, m_data.Z, true);    // currently it is required that X is already sorted

		for (int i = 0; i < (minLength - 1) * INTERPOLATION_UPSAMPLING_SIZE; i++)
		{
			m_interpolatedData.X.push_back(sX(i* INTERPOLATION_WIDE));
			m_interpolatedData.Y.push_back(sY(i* INTERPOLATION_WIDE));
			m_interpolatedData.Z.push_back(sZ(i* INTERPOLATION_WIDE));
		}

		tk::spline sRotX;
		tk::spline sRotY;
		tk::spline sRotZ;

		sRotX.set_points(X, m_rotData.X, true);    // currently it is required that X is already sorted
		sRotY.set_points(X, m_rotData.Y, true);    // currently it is required that X is already sorted
		sRotZ.set_points(X, m_rotData.Z, true);    // currently it is required that X is already sorted

		for (int i = 0; i < (minLength - 1) * INTERPOLATION_UPSAMPLING_SIZE; i++)
		{
			m_interpolatedRotData.X.push_back(sRotX(i* INTERPOLATION_WIDE));
			m_interpolatedRotData.Y.push_back(sRotY(i* INTERPOLATION_WIDE));
			m_interpolatedRotData.Z.push_back(sRotZ(i* INTERPOLATION_WIDE));
		}

		//convert the degree values to radian values because the MBC gets the values as radians.
		for (int i = 0; i<(minLength - 1)*INTERPOLATION_UPSAMPLING_SIZE; i++)
		{
			//todo:check if can convert to radian all the degress when getting it from the Matlab.
			//no need here to convert to radians because it is get from the laastcommandframe which is in radian.
			m_interpolatedRotData.X[i] = m_interpolatedRotData.X[i];
			m_interpolatedRotData.Y[i] = m_interpolatedRotData.Y[i];
			m_interpolatedRotData.Z[i] = m_interpolatedRotData.Z[i];
		}
	}


	WRITE_LOG_PARAM(m_logger->m_logger, "m_interpolatedData.X.size()", (int)m_interpolatedData.X.size());
	WRITE_LOG_PARAM(m_logger->m_logger, "m_data size is : ", (int)m_data.X.size());
	////////////////////////////////////////////////////////////////////////////////////////end of interpolated version///////////////////////////////////////////////////////////////////
}


vector<double> MoogDotsCom::convertPolar2Vector(double elevation, double azimuth, double magnitude)
{
	vector<double> convertedVector;
	double x, y, z;

	// Calculate the z-component.
	z = magnitude * sin(elevation);

	// Calculate the y-component.
	y = magnitude * cos(elevation) * sin(azimuth);

	// Calculate the x-componenet.
	x = magnitude * cos(elevation) * cos(azimuth);

	// Stuff the results into a vector.
	convertedVector.push_back(x);
	convertedVector.push_back(y);
	convertedVector.push_back(z);

	return convertedVector;
}

GLWindow* MoogDotsCom::GetGLWindow(void) const
{
	return m_glWindow;
}

void MoogDotsCom::RenderFrameInGlPanel()
{
	double azimuth = 0.0, elevation = 0.0;

	// Grab a pointer to the GLPanel.
	GLPanel *glPanel = m_glWindow->GetGLPanel();

	m_glWindow->GetGLPanel()->renderNow = true;

	// Set the translation components to the camera.
	glPanel->SetLateral(m_glData.X.at(m_glData.index));
	glPanel->SetSurge(m_glData.Y.at(m_glData.index));
	glPanel->SetHeave(m_glData.Z.at(m_glData.index));

	// matan's flashing test
	/*vector<bool> matanArray;
	for (int i = 0; i < 60; i++) {
		if (i % 10 < 5) {
			matanArray.push_back(true);
		}
		else {
			matanArray.push_back(false);
		}
	}*/
	//glPanel->SetDrawFlashSquareAtCurrentFrame(matanArray.at((m_glData.index)));
	glPanel->SetDrawFlashSquareAtCurrentFrame(m_drawFlashingFrameSquareData.at((m_glData.index)));


	// Set the rotation angle.
	glPanel->SetRotationAngle(m_glRotData.at(m_glData.index));

	// Set sphere field translation
	glPanel->SetSphereFieldTran(m_glObjectData.X.at(m_glData.index),
		m_glObjectData.Y.at(m_glData.index),
		m_glObjectData.Z.at(m_glData.index));

	// Calculate the rotation vector describing the axis of rotation.
	m_rotationVector = nmSpherical2Cartesian(m_glRotEle.at(m_glData.index), m_glRotAz.at(m_glData.index), 1.0, true);

	// Swap the y and z values of the rotation vector to accomodate OpenGL.  We also have
	// to negate the y value because forward is negative in our OpenGL axes.
	double tmp = -m_rotationVector.y;
	m_rotationVector.y = m_rotationVector.z;
	m_rotationVector.z = tmp;
	glPanel->SetRotationVector(m_rotationVector);

	//// If we're doing rotation, set the rotation data.
	//if (m_setRotation == true) {
	//	double val = m_glRotData.at(m_grabIndex);
	//	glPanel->SetRotationAngle(m_interpRotation.at(m_grabIndex));

	//	if (g_pList.GetVectorData("FP_ROTATE").at(0) == 1.0) {
	//		azimuth = m_fpRotData.X.at(m_grabIndex);
	//		elevation = m_fpRotData.Y.at(m_grabIndex);
	//	}
	//}

#if USE_MATLAB_DEBUG_GRAPHS
	m_debugFrameTime.push_back(time);
	m_debugFramePlace.push_back(m_glData.Y.at(m_glData.index) / 100);
#endif

#pragma region LOG-START_RENDER
	double time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
	WRITE_LOG_PARAMS2(m_logger->m_logger, "Starting rendering for the new frame. ", m_glData.index, time);
#pragma endregion LOG-START_RENDER

	glPanel->Render(m_eyeOrientationQuaternion);

#pragma region LOG-END_RENDER
	time = (double)((clock() - m_roundStartTime) * 1000) / (double)CLOCKS_PER_SEC;
	WRITE_LOG_PARAMS2(m_logger->m_logger, "Ending rendering for the new frame. ", m_glData.index, time);
#pragma endregion LOG-END_RENDER

	m_trial_finished = false;
	m_waiting_a_little_after_finished = false;
	m_drawRegularFeedback = false;
}

double MoogDotsCom::deg2rad(double deg)
{
	return deg / 180 * PI;
}

Cube MoogDotsCom::createCube(void)
{
	Cube cube;

	cube.enable = g_pList.GetVectorData("ENABLE_CUBE").at(0) ? true : false;
	cube.style = g_pList.GetVectorData("CUBE_DATA").at(0);
	cube.size = g_pList.GetVectorData("CUBE_DATA").at(1);
	cube.rotateAngle = g_pList.GetVectorData("CUBE_DATA").at(2);
	cube.rx = g_pList.GetVectorData("CUBE_DATA").at(3);
	cube.ry = g_pList.GetVectorData("CUBE_DATA").at(4);
	cube.rz = g_pList.GetVectorData("CUBE_DATA").at(5);
	cube.tx = g_pList.GetVectorData("CUBE_DATA").at(6);
	cube.ty = g_pList.GetVectorData("CUBE_DATA").at(7);
	cube.tz = g_pList.GetVectorData("CUBE_DATA").at(8);

	return cube;
}

#if TRAJECTORY_SAFETY_CHECK
bool MoogDotsCom::CheckTrajectories()
{
	// Grab the trajectory data from the parameter list.
	vector<double> trajectories[6];
	trajectories[0] = g_pList.GetVectorData("LATERAL_DATA");
	trajectories[1] = g_pList.GetVectorData("SURGE_DATA");
	trajectories[2] = g_pList.GetVectorData("HEAVE_DATA");
	trajectories[3] = g_pList.GetVectorData("YAW_DATA");
	trajectories[4] = g_pList.GetVectorData("PITCH_DATA");
	trajectories[5] = g_pList.GetVectorData("ROLL_DATA");

	// for test only
	// trajectories[1].at(10) = trajectories[1].at(10)+TRAJ_CHANGE_CHECK_SCALE;

	//stuffDoubleVector(trajectories[1], "ox");

	// Check the trajectories have any bumping inside
	bool smooth = true;
	for (int i = 0; i < 6; i++) {
		if (FindBumping(trajectories[i]) == false) {
			wxString s;
			smooth = false;
			switch (i)
			{
			case 0:
				s = "Error: Lateral trajectory has problem.";
				break;
			case 1:
				s = "Error: Surge trajectory has problem.";
				break;
			case 2:
				s = "Error: Heave trajectory has problem.";
				break;
			case 3:
				s = "Error: Yaw trajectory has problem.";
				break;
			case 4:
				s = "Error: Pitch trajectory has problem.";
				break;
			case 5:
				s = "Error: Roll trajectory has problem.";
				break;
			}
			m_messageConsole->InsertItems(1, &s, 0);
			// Stop the Compute() function but let ReceiveCompute() continue.
			//DoCompute(RECEIVE_COMPUTE);
			break;
		}
	}

	return smooth;
}

bool MoogDotsCom::FindBumping(vector<double> trajectory)
{
	// Derivative make the smooth part of trajectory close to zero, 
	// but amplify the bumping (non-smooth) part

	// Find out how many times we need derivative of trajectory
	int DTimes = (int)ceil(log(1 / TRAJ_CHANGE_CHECK_SCALE) / log(10.0));
	int size = trajectory.size();
	vector<double> traj;

	// velocity
	for (int i = 0; i<size - 1; i++) {
		traj.push_back(trajectory[i + 1] - trajectory[i]);
	}
	size--;

	// acceleration
	for (int i = 0; i<size - 1; i++) {
		traj[i] = traj[i + 1] - traj[i];
		if (traj[i]>MAX_ACCELERATION)
			return false;
	}
	size--;

	// for test and use_Matlab
	// stuffDoubleVector(traj, "oxb");

	for (int j = 0; j<DTimes - 1; j++) {
		for (int i = 0; i<size - 1; i++) {
			traj[i] = traj[i + 1] - traj[i];
		}
		size = size - 1;
	}

	double mean = 0;
	double maxDiff = 0;
	// last derivative
	double tmp = 0;
	for (int i = 0; i<size - 1; i++) {
		traj[i] = traj[i + 1] - traj[i];
		if (traj[i] >= 0) { //positive
			mean = mean + traj[i];
			if (maxDiff<traj[i]) maxDiff = traj[i];
		}
		else { // negative
			mean = mean - traj[i];
			if (maxDiff < -traj[i]) maxDiff = -traj[i];
		}
	}
	mean = mean / (size - 1);

	// for test and use_Matlab
	// stuffDoubleVector(traj, "ox");

	if (maxDiff > TRAJ_CHANGE_CHECK_SCALE && maxDiff > mean * 10) return false;
	else return true;
}
#endif

void MoogDotsCom::AddNoise()
{
	vector<int> MI;
	bool delayInterval = false;

	// Add noise to the signal if flagged.
	if (g_pList.GetVectorData("NOISE_PARAMS").at(0)) {
		nm3DDatum mag;
		// change Magnitude from cm to meter before call nmGenerateFilteredNoise function
		mag.x = g_pList.GetVectorData("NOISE_PARAMS").at(2) / 100;
		mag.y = g_pList.GetVectorData("NOISE_PARAMS").at(3) / 100;
		mag.z = g_pList.GetVectorData("NOISE_PARAMS").at(4) / 100;

		// Prepare to add noise on multi-interval experiment.
		int noiseLength = (int)m_data.X.size() - m_recordOffset;
		if (g_pList.GetVectorData("NOISE_PARAMS").at(9)) { // Multi-Interval
			int i = 0, index = m_recordOffset;
			while (i < (int)m_data.X.size() - m_recordOffset - 1) {
				index = i + m_recordOffset;
				if (!delayInterval) {
					if (m_data.X.at(index) == m_data.X.at(index + 1) && // When all data are same,
						m_data.Y.at(index) == m_data.Y.at(index + 1) &&  // it is in between two intervals.
						m_data.Z.at(index) == m_data.Z.at(index + 1) &&
						m_glData.X.at(i) == m_glData.X.at(i + 1) &&
						m_glData.Y.at(i) == m_glData.Y.at(i + 1) &&
						m_glData.Z.at(i) == m_glData.Z.at(i + 1))
					{
						MI.push_back(i);
						delayInterval = true;
					}
				}
				else { //Delay Interval
					if (m_data.X.at(index) != m_data.X.at(index + 1) || // If find data are not same,
						m_data.Y.at(index) != m_data.Y.at(index + 1) ||  // then it is in next intervals.
						m_data.Z.at(index) != m_data.Z.at(index + 1) ||
						m_glData.X.at(i) != m_glData.X.at(i + 1) ||
						m_glData.Y.at(i) != m_glData.Y.at(i + 1) ||
						m_glData.Z.at(i) != m_glData.Z.at(i + 1))
					{
						MI.push_back(i);
						delayInterval = false;
					}
				}
				i++;
			}

			if (MI.size()>0) {
				noiseLength = MI.at(0) + 1;
				MI.push_back((int)m_data.X.size() - m_recordOffset);
			}
		}

		// Generate the filtered noise that we'll add to the command buffer.
		nmGenerateFilteredNoise((long)g_pList.GetVectorData("NOISE_PARAMS").at(6), // Gaussian seed
			noiseLength,
			g_pList.GetVectorData("NOISE_PARAMS").at(1), // Cutoff freq
			mag, g_pList.GetVectorData("NOISE_PARAMS").at(5), // Dimension
			true, true, &m_noise, &m_filteredNoise);

		// This is a function from NumericalMethods that will rotate a data set.
		nmRotateDataYZ(&m_filteredNoise, g_pList.GetVectorData("NOISE_PARAMS").at(7), // Noise Azimuth 
			g_pList.GetVectorData("NOISE_PARAMS").at(8)); // Noise Elevation

		if (g_pList.GetVectorData("NOISE_PARAMS").at(9)) { // Multi-Interval
			int i = 0, j = 0, k = 0, index = m_recordOffset;
			while (i < (int)m_data.X.size() - m_recordOffset) {
				index = i + m_recordOffset;
				long GaussianSeed = (long)g_pList.GetVectorData("NOISE_PARAMS").at(6);
				// Command
				m_data.X.at(index) += m_filteredNoise.X.at(j);
				m_data.Y.at(index) += m_filteredNoise.Y.at(j);
				m_data.Z.at(index) += m_filteredNoise.Z.at(j);

				// Visual
				m_glData.X.at(i) += m_filteredNoise.X.at(j) * 100;
				m_glData.Y.at(i) += m_filteredNoise.Y.at(j) * 100;
				m_glData.Z.at(i) += m_filteredNoise.Z.at(j) * 100;

				if (j + 1 >= (int)m_filteredNoise.X.size()) {
					k++;
					if (k < (int)MI.size()) {
						noiseLength = MI.at(k + 1) - MI.at(k) + 1;
						if (g_pList.GetVectorData("NOISE_PARAMS").at(10) != 1) // not same noise, then change Gaussian seed
							GaussianSeed += k;

						// Generate the filtered noise that we'll add to the command buffer.
						nmGenerateFilteredNoise(GaussianSeed, // Gaussian seed
							noiseLength,
							g_pList.GetVectorData("NOISE_PARAMS").at(1), // Cutoff freq
							mag, g_pList.GetVectorData("NOISE_PARAMS").at(5), // Dimension
							true, true, &m_noise, &m_filteredNoise);

						// This is a function from NumericalMethods that will rotate a data set.
						nmRotateDataYZ(&m_filteredNoise, g_pList.GetVectorData("NOISE_PARAMS").at(7), // Noise Azimuth 
							g_pList.GetVectorData("NOISE_PARAMS").at(8)); // Noise Elevation

						i = MI.at(k);
						k++;
						j = 0;
					}
					else break;
				}
				i++;
				j++;
			}
		}
		else { // one interval
			int minSize = m_data.X.size();
			if (minSize > (int)m_filteredNoise.X.size()) minSize = m_filteredNoise.X.size();
			// Add the noise to the command and visual feed.
			for (int i = 0; i < minSize; i++) {
				int index = i + m_recordOffset;

				// Command
				m_data.X.at(index) += m_filteredNoise.X.at(i);
				m_data.Y.at(index) += m_filteredNoise.Y.at(i);
				m_data.Z.at(index) += m_filteredNoise.Z.at(i);

				// Visual
				m_glData.X.at(i) += m_filteredNoise.X.at(i) * 100;
				m_glData.Y.at(i) += m_filteredNoise.Y.at(i) * 100;
				m_glData.Z.at(i) += m_filteredNoise.Z.at(i) * 100;
			}
		}
	}
}