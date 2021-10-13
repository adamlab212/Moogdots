#include "ParameterList.h"

// Critical function lock for the parameter list.
wxCriticalSection g_paramLock;

// Global parameter list
CParameterList g_pList;

CParameterList::CParameterList()
{
	LoadHash();
}

void CParameterList::LoadHash()
{
	ParameterValue x;
	int i;
	double j;


	/***************** Variable value parameters **************/
	// Lateral data.
	x.data.clear();
	x.variable = true;
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		//x.data.push_back(static_cast<double>(j));
		x.data.push_back(0.0);
	}
	x.description = "Lateral trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("LATERAL_DATA", x));

	// Surge data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		//x.data.push_back(static_cast<double>(j));
		x.data.push_back(0.0);
	}
	x.description = "Surge trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("SURGE_DATA", x));

	// Heave data.
	x.data.clear();
	// for test only
	double HEAVE_DATA[]={0.000000, -0.005000, -0.010000, -0.015000, -0.021000, -0.028000, -0.036000, -0.044000, -0.053000, -0.063000, -0.074000, -0.086000, -0.099000, -0.114000, -0.129000, -0.146000, -0.164000, -0.184000, -0.205000, -0.228000, -0.252000, -0.278000, -0.307000, -0.337000, -0.369000, -0.403000, -0.440000, -0.478000, -0.519000, -0.563000, -0.609000, -0.657000, -0.708000, -0.762000, -0.818000, -0.876000, -0.938000, -1.002000, -1.069000, -1.138000, -1.210000, -1.285000, -1.362000, -1.441000, -1.524000, -1.608000, -1.695000, -1.784000, -1.874000, -1.967000, -2.062000, -2.158000, -2.256000, -2.356000, -2.456000, -2.558000, -2.660000, -2.764000, -2.867000, -2.972000, -3.076000, -3.180000, -3.284000, -3.388000, -3.492000, -3.594000, -3.696000, -3.796000, -3.896000, -3.994000, -4.090000, -4.185000, -4.277000, -4.368000, -4.457000, -4.544000, -4.628000, -4.710000, -4.790000, -4.867000, -4.942000, -5.014000, -5.083000, -5.150000, -5.214000, -5.275000, -5.334000, -5.390000, -5.444000, -5.495000, -5.543000, -5.589000, -5.633000, -5.674000, -5.712000, -5.749000, -5.783000, -5.815000, -5.845000, -5.874000, -5.900000, -5.924000, -5.947000, -5.968000, -5.988000, -6.006000, -6.023000, -6.038000, -6.052000, -6.066000, -6.078000, -6.088000, -6.099000, -6.108000, -6.116000, -6.124000, -6.130000, -6.137000, -6.142000, -6.147000};
	for(i=0; i<120; i++) x.data.push_back(HEAVE_DATA[i]);
	
	//for (j = 0.0; j <= 50.0; j += 10.0/60.0/2.0) {
	//	x.data.push_back(0.0);
	//}
	x.description = "Heave trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("HEAVE_DATA", x));

	// Yaw data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(0.0);
	}
	x.description = "Yaw trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("YAW_DATA", x));

	// Pitch data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(0.0);
	}
	x.description = "Pitch trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("PITCH_DATA", x));

	// Roll data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(0.0);
	}
	x.description = "Roll trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("ROLL_DATA", x));

	// Flash Square Data data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0 / 60.0 / 2.0) {
		x.data.push_back(0.0);
	}
	x.description = "Flash Square Vector";
	m_pHash.insert(ParameterKeyPair("FLASH_SQUARE_DATA", x));

	// Rotation data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(0.0);
	}
	x.description = "OpenGL rotation angles (deg)";
	m_pHash.insert(ParameterKeyPair("GL_ROT_DATA", x));

	// OpenGL lateral data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(0.0);
	}
	x.description = "OpenGL Lateral trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("GL_LATERAL_DATA", x));

	// OpenGL surge data.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		//x.data.push_back(static_cast<double>(j));
		x.data.push_back(0.0);
	}
	x.description = "OpenGL Surge trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("GL_SURGE_DATA", x));

	// OpenGL heave data.
	x.data.clear();
	// for test only
	for(i=0; i<120; i++) x.data.push_back(HEAVE_DATA[i]);

	//for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
	//	x.data.push_back(0.0);
	//}
	x.description = "OpenGL Heave trajectory data (cm)";
	m_pHash.insert(ParameterKeyPair("GL_HEAVE_DATA", x));

	// OpenGL elevation for the rotation vector.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(0.0);
	}
	x.description = "OpenGL elevation for the rotation vector.";
	m_pHash.insert(ParameterKeyPair("GL_ROT_ELE", x));

	// OpenGL azimuth for the rotation vector.
	x.description = "OpenGL azimuth for the rotation vector.";
	m_pHash.insert(ParameterKeyPair("GL_ROT_AZ", x));

	// OpenGL object unit trajectory.
	x.data.clear();
	for (j = 0.0; j <= 10.0; j += 10.0/60.0/2.0) {
		x.data.push_back(static_cast<double>(j));
	}
	x.description = "OpenGL object trajectory.";
	m_pHash.insert(ParameterKeyPair("OBJECT_TRAJ", x));

	// OpenGL Object field parameters, such as Sphere or Circle.
	x.variable = false;
	x.data.clear();
	for (i = 0; i < 5; i++) {
		x.data.push_back(0.0);
	}
	x.data[0] = 1.0;	// Off/Sphere/Circle
	x.data[1] = 10.0;	// radius of patch (cm)
	x.data[2] = 1.0;	// Size of triangles' base (cm)
	x.data[3] = 1.0;	// Size of triangles' height (cm)
	x.data[4] = 0.1;	// density (number of triangles per cm^3)
	x.description = "Sphere field: (Off,Sphere,Circle)=(0,1,2); radius(cm); tri base(cm); tri height(cm); density(num/cm^3 or cm^2)";
	m_pHash.insert(ParameterKeyPair("SPHERE_FIELD_PARAMS", x));

	// Noise parameters
	x.variable = false;
	x.data.clear();
	for (i = 0; i < 11; i++) {
		x.data.push_back(0.0);
	}

	x.data[0] = 0.0;	// Enable Noise
	x.data[1] = 2.0;	// Cutoff freq
	x.data[2] = 1.0;	// Moise Mag X (cm)
	x.data[3] = 1.0;	// Moise Mag Y
	x.data[4] = 1.0;	// Moise Mag Z
	x.data[5] = 3.0;	// Dimension 
	x.data[6] = 1978.0;	// Gaussian seed
	x.data[7] = 0.0;	// Noise Azimuth 
	x.data[8] = 0.0;	// Noise Elevation 
	x.data[9] = 0.0;	// Multi-Interval 
	x.data[10] = 1.0;	// Same noise on all interval 
	x.description = "Noise [Enable; Cutoff freq; Noise Mag(cm) X; Y; Z; Dim; Gaussian seed; Azi; Ele; Multi-Interval; Same Noise]";
	m_pHash.insert(ParameterKeyPair("NOISE_PARAMS", x));

	// Cube setup
	x.variable = false;
	x.data.clear();
	for (i = 0; i < 9; i++) {
		x.data.push_back(0.0);
	}

	x.data[0] = 0.0;	// wire=0.0 or solid=1.0 cube
	x.data[1] = 20.0;	// cube size in world coordinate
	// rotation setup
	x.data[2] = 45.0;	// rotation angle in degree
	// rotation vector
	x.data[3] = 0.0;	// x-coordinate 
	x.data[4] = 1.0;	// y-coordinate 
	x.data[5] = 0.0;	// z-coordinate 
	// Translate setup
	x.data[6] = 10.0;	// x-coordinate 
	x.data[7] = 0.0;	// y-coordinate 
	x.data[8] = 0.0;	// z-coordinate 
	x.description = "Cube style: Wire=0 / Solid=1; size(cm);\nglRotatef(angle,x,y,z); glTranslatef(x,y,z)";
	m_pHash.insert(ParameterKeyPair("CUBE_DATA", x));

	// Drawing calibration stars
	x.variable = false;
	x.data.clear();
	for (i = 0; i < 4; i++) {
		x.data.push_back(0.0);
	}

	x.data.at(0) = 0.0; x.data.at(1) = 0.0; x.data.at(2) = 3.0; x.data.at(3) = 10.0;
	x.description = "On/Off, [horizontal, vertical dist, depth dist](cm)";
	m_pHash.insert(ParameterKeyPair("ENABLE_CALIB_STAR", x));

	/***************** Three value parameters *******************/
	x.variable = false;
	x.data.clear();
	for (i = 0; i < 3; i++) {
		x.data.push_back(0.0);
	}

	// X locations for the cylinders.
	x.data.at(0) = 0.0; x.data.at(1) = 10.0; x.data.at(2) = -10.0;
	x.description = "Cylinders X positions (cm).";
	m_pHash.insert(ParameterKeyPair("CYLINDERS_XPOS", x));

	// Y locations for the cylinders.
	x.data.at(0) = 0.0; x.data.at(1) = 10.0; x.data.at(2) = -10.0;
	x.description = "Cylinders Y positions (cm).";
	m_pHash.insert(ParameterKeyPair("CYLINDERS_YPOS", x));

	// Z locations for the cylinders.
	x.data.at(0) = 10.0; x.data.at(1) = 10.0; x.data.at(2) = 10.0;
	x.description = "Cylinders Z positions (cm).";
	m_pHash.insert(ParameterKeyPair("CYLINDERS_ZPOS", x));

	// Default point of origin.
	x.data.at(0) = 0.0; x.data.at(1) = 0.0; x.data.at(2) = 0.0;
	x.description = "Point of Origin (x, y, z) (m)";
	m_pHash.insert(ParameterKeyPair("M_ORIGIN", x));

	// Offsets that are added onto the OpenGL center of rotation.
	x.description = "Offsets added to the OpenGL center of rotation (x,y,z)cm";
	m_pHash.insert(ParameterKeyPair("GL_ROT_OFFSETS", x));

	// Center of the floor.
	x.data.at(0) = 0.0; x.data.at(1) = 0.0; x.data.at(2) = 10.0;
	x.description = "Center of the floor (x,y,z) (cm).";
	m_pHash.insert(ParameterKeyPair("FLOOR_ORIGIN", x));

	// Global coordinates for the center of the platform.
	x.data.at(0) = 0.0; x.data.at(1) = 0.0; x.data.at(2) = 0.0;
	x.description = "Platform center coordinates.";
	m_pHash.insert(ParameterKeyPair("PLATFORM_CENTER", x));

	// Global coordinates for the center of the head.
	x.data.at(0) = 0.0; x.data.at(1) = 23.0; x.data.at(2) = 0.0;
	x.description = "Center of head based around cube center (cm).";
	m_pHash.insert(ParameterKeyPair("HEAD_CENTER", x));

	// Coordinates for the rotation origin.
	x.data.at(0) = 0.0; x.data.at(1) = 0.0; x.data.at(2) = 0.0;
	x.description = "Rotation origin (yaw, pitch, roll) (deg).";
	m_pHash.insert(ParameterKeyPair("ROT_ORIGIN", x));

	// Starfield dimensions in cm.
	x.description = "Starfield dimensions in cm.";
	x.data[0] = 100.0; x.data[1] = 100.0; x.data[2] = 50.0;
	m_pHash.insert(ParameterKeyPair("STAR_VOLUME", x));

	// X-coordinate for fixation point, target 1 & 2.
	x.description = "Target x-coordinates.";
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	m_pHash.insert(ParameterKeyPair("TARG_XCTR", x));

	// Y-coordinate for fixation point, target 1 & 2.
	x.description = "Target y-coordinates.";
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	m_pHash.insert(ParameterKeyPair("TARG_YCTR", x));

	// Z-coordinate for fixation point, target 1 & 2.
	x.description = "Target z-coordinates.";
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	m_pHash.insert(ParameterKeyPair("TARG_ZCTR", x));

	// Color for the left star scene.
	x.description = "Color for the left-eye stars.";
	x.data[0] = 1.0; x.data[1] = 0.0; x.data[2] = 0.0;
	m_pHash.insert(ParameterKeyPair("STAR_LEYE_COLOR", x));

	// Color for the right star scene.
	x.description = "Color for the right-eye stars.";
	x.data[0] = 0.0; x.data[1] = 1.0; x.data[2] = 0.0;
	m_pHash.insert(ParameterKeyPair("STAR_REYE_COLOR", x));

	// Location of the monocular eye measured from the center of the head.
	x.data.at(0) = 0.0; x.data.at(1) = 11.0; x.data.at(2) = 0.0;
	x.description = "Location of the monocular eye measured from the center of the head. (x, y, z)cm.";
	m_pHash.insert(ParameterKeyPair("EYE_OFFSETS", x));

	// Location of the monocular eye measured from the center of the head.
	x.data.at(0) = 1.0; x.data.at(1) = 1.0; x.data.at(2) = 0.0;
	x.description = "Target Color for FP and 2 targets.";
	m_pHash.insert(ParameterKeyPair("FP_COLOR", x));

	// Flags a circle cutout to appear at the center of the screen.
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	x.description = "Flags circle cutout; Draw stuff in circle; Stuff [Move/Stay]=[0/1]";
	m_pHash.insert(ParameterKeyPair("ENABLE_CUTOUT", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	x.description = "Origin of Object field (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("OBJECT_POS", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	x.description = "Offsets added to center of rotation (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("ROT_CENTER_OFFSETS", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0; x.data[1] = 0.0; x.data[2] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("ORIGIN", x));

	/***************** Two value parameters *****************/
	x.data.clear();
	for (i = 0; i < 2; i++) {
		x.data.push_back(0.0);
	}

	// Screen dimensions.
	x.description = "Screen Width and Height (cm).";
	//x.data[0] = 58.8;	// Width
	//x.data[1] = 58.9;	// Height
	x.data[0] = 127.0;	// Width
	x.data[1] = 149.0;	// Height
	m_pHash.insert(ParameterKeyPair("SCREEN_DIMS", x));

	// Floor dimensions.
	x.description = "Floor width and depth (cm).";
	x.data.at(0) = 70.0; x.data.at(1) = 70.0;
	m_pHash.insert(ParameterKeyPair("FLOOR_DIMS", x));

	// Near and far clipping planes.
	x.description = "Near and Far clipping planes (cm).";
	x.data[0] = 5.0;	// Near
	x.data[1] = 200.0;	// Far
	m_pHash.insert(ParameterKeyPair("CLIP_PLANES", x));

	// Triangle dimensions.
	x.description = "Triangle Base and Height (cm).";
//#if DEBUG_DEFAULTS
//	x.data[0] = 0.3; x.data[1] = 0.3;
//#else
//	x.data[0] = 0.15; x.data[1] = 0.15;
//#endif
	x.data[0] = 1; x.data[1] = 1;
	m_pHash.insert(ParameterKeyPair("STAR_SIZE", x));

	// FP cross length and width.
	x.description = "FP cross length(cm) and width(pixel)";
	x.data[0] = 1.0;	// length (cm)
	x.data[1] = 1.0;	// width (pixel)
	m_pHash.insert(ParameterKeyPair("FP_CROSS_LW", x));

	/***************** One value parameters *****************/
	x.data.clear();
	x.data.push_back(0.0);

#if DEBUG_DEFAULTS
	x.data.at(0) = 1.0;
#else
	x.data.at(0) = 0.0;
#endif
	x.description = "Toggle fixation point rotation.";
	m_pHash.insert(ParameterKeyPair("FP_ROTATE", x));

	// Enables/Disables floor.
	x.data.at(0) = 0.0;
	x.description = "Enable/Disable floor.";
	m_pHash.insert(ParameterKeyPair("ENABLE_FLOOR", x));

	// Enables/Disables the cylinders.
	x.data.at(0) = 0.0;
	x.description = "Enable/Disable the cylinders.";
	m_pHash.insert(ParameterKeyPair("ENABLE_CYLINDERS", x));

	// Enables/Disables grid.
	x.data.at(0) = 1.0;
	x.description = "Enable/Disable grid.";
	m_pHash.insert(ParameterKeyPair("ENABLE_GRID", x));

	// Enables/Disables Frame Counter.
	x.data.at(0) = 1.0;
	x.description = "Enable/Disable Frame Counter.";
	m_pHash.insert(ParameterKeyPair("ENABLE_COUNTER", x));

	// Curve screen on/off .
	//x.data.at(0) = 1.0;
	//x.description = "Curve screen on/off.";
	//m_pHash.insert(ParameterKeyPair("CURVE_SCREEN_ON", x));

	// Curve screen texture mapping square size. (cm)
	x.data.at(0) = 10.0;
	x.description = "Curve screen texture mapping square size. (cm)";
	m_pHash.insert(ParameterKeyPair("CURVE_SCREEN_SPACE", x));

	x.data.at(0) = 0;
	x.description = "Trial number";
	m_pHash.insert(ParameterKeyPair("Trial", x));


	// Enables/Disables cube
	x.data.at(0) = 0.0;
	x.description = "Enables/Disables cube";
	m_pHash.insert(ParameterKeyPair("ENABLE_CUBE", x));

	// Radius of a star in centimeters.
	x.data.at(0) = 0.1;
	x.description = "Radius of a star in centimeters.";
	m_pHash.insert(ParameterKeyPair("STAR_RADIUS", x));

	// point size of a star in pixels.
	x.data.at(0) = 3;
	x.description = "Point size of a star in pixels.";
	m_pHash.insert(ParameterKeyPair("STAR_POINT_SIZE", x));

	// Size of the floor objects.
	x.data.at(0) = .25;
	x.description = "Size of floor objects";
	m_pHash.insert(ParameterKeyPair("FLOOR_OBJ_SIZE", x));

	// Increment of triangle fan.
	x.data.at(0) = 22.5;
	x.description = "Increment of the triangle fan (deg).";
	m_pHash.insert(ParameterKeyPair("STAR_INC", x));

	// Delay after the sync.
	x.data.at(0) = 0.0;
	x.description = "Delay (ms) after the sync.";
	m_pHash.insert(ParameterKeyPair("SYNC_DELAY", x));

	// Star lifetime.
	x.description = "Star lifetime (#frames).";
	x.data[0] = 5.0;
	m_pHash.insert(ParameterKeyPair("STAR_LIFETIME", x));

	// Star motion coherence factor.  0 means all stars change.
	x.description = "Star motion coherence (% out of 100).";
	x.data[0] = 100.0;
	m_pHash.insert(ParameterKeyPair("STAR_MOTION_COHERENCE", x));
	
	// Turns star lifetime on and off.
	x.description = "Star lifetime on/off.";
	x.data[0] = 1.0;
	m_pHash.insert(ParameterKeyPair("STAR_LIFETIME_ON", x));

	// Star luminance multiplier.
	x.description = "Star luminance multiplier.";
	x.data[0] = 1.0;
	m_pHash.insert(ParameterKeyPair("STAR_LUM_MULT", x));

	// Target 1 on/off.
	x.description = "Target 1 on/off.";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("TARG1_ON", x));

	// Target 2 on/off.
	x.description = "Target 2 on/off.";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("TARG2_ON", x));
	
	// Fixation point on/off.
	x.description = "Fixation point on/off.";
#if DEBUG_DEFAULTS
	x.data[0] = 1.0;
#else
	x.data[0] = 0.0;
#endif
	m_pHash.insert(ParameterKeyPair("FP_ON", x));

	// Flashing fixation point on/off.
	x.data[0] = 0.0; 
	m_pHash.insert(ParameterKeyPair("FP_FLASH_ON", x));

	//Frame freeze number to indicate the frame to freeze with (untill 2nd response indication).
	x.data[0] = -1.0;
	m_pHash.insert(ParameterKeyPair("FREEZE_FRAME", x));

	// Turns movement on and off.
	x.description = "Enables motion base movement. (0.0=off, 1.0==on)";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("DO_MOVEMENT", x));

	// Turns movement on and off.
	x.description = "Enables motion base movement after a freeze. (0.0=off, 1.0==on)";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("DO_MOVEMENT_FREEZE", x));

	x.description = "Makes the motion base move to the origin.";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("GO_TO_ORIGIN", x));

	// Indicates if the target should be on.
	x.description = "Indicates if the target should be on.";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("TARGET_ON", x));

	// Size of the center target.
	x.description = "Size of the center target.";
	x.data[0] = 7.0;
	m_pHash.insert(ParameterKeyPair("TARGET_SIZE", x));

	// Indicates if the background shoud be on.
	x.description = "Indicates if the background should be on.";
#if DEBUG_DEFAULTS
	x.data[0] = 1.0;
#else
	x.data[0] = 0.0;
#endif
	m_pHash.insert(ParameterKeyPair("BACKGROUND_ON", x));

	// Draw circles or triangles.
	x.description = "Draw circles (0.0) or triangles (1.0) or dots (2.0).";
	x.data[0] = 1.0;
	m_pHash.insert(ParameterKeyPair("DRAW_MODE", x));

	//Draw cube sensor for visual stimulus timing.
	x.description = "Draw sensor cube (1.0) or not (0.0)";
	x.data.at(0) = 0.0;
	m_pHash.insert(ParameterKeyPair("PHOTODIODE_ON", x));

	//Send data to the LPT controller.
	x.description = "Send data to the LPT controller.";
	x.data.at(0) = 0.0;
	m_pHash.insert(ParameterKeyPair("LPT_DATA_SEND", x));

	// Floor draw mode.
	x.description = "0.0 = circles, 1.0 = squares.";
	x.data.at(0) = 1.0;
	m_pHash.insert(ParameterKeyPair("FLOOR_DRAW_MODE", x));

	// Interocular distance.
	x.description = "Interocular distance (cm).";
//#if DEBUG_DEFAULTS
//	x.data.at(0) = 0.7;
//#else
//	x.data[0] = 3.0;
//#endif
	x.data[0] = 7.0;
	m_pHash.insert(ParameterKeyPair("IO_DIST", x));

	// Starfield density (stars/cm^3).
	x.description = "Starfield Density (stars/cm^3).";
	//x.data[0] = 0.01;
	x.data[0] = 0.005;
	m_pHash.insert(ParameterKeyPair("STAR_DENSITY", x));

	// Floor density (objects/cm^2)
	x.description = "Floor density (objects/cm^2).";
	x.data.at(0) = 1.0;
	m_pHash.insert(ParameterKeyPair("FLOOR_DENSITY", x));

	// Cylinder height.
	x.description = "Cylinder height (cm)";
	x.data.at(0) = 5.0;
	m_pHash.insert(ParameterKeyPair("CYLINDER_HEIGHT", x));

	// Cylinder radius.
	x.description = "Cylinder radius (cm)";
	x.data.at(0) = 1.0;
	m_pHash.insert(ParameterKeyPair("CYLINDER_RADIUS", x));

	// Cylinder slices.
	x.description = "Number of cylinder slices.";
	x.data.at(0) = 4.0;
	m_pHash.insert(ParameterKeyPair("CYLINDER_SLICES", x));

	// Number of cylinder stacks.
	x.description = "Number of cylinder stacks.";
	x.data.at(0) = 1.0;
	m_pHash.insert(ParameterKeyPair("CYLINDER_STACKS", x));

	// Number of cylinder stacks.
	x.description = "Stereo on/off.";
	x.data.at(0) = 1.0;
	m_pHash.insert(ParameterKeyPair("ENABLE_STEREO", x));

	// The radius of the cutout.
	x.data.at(0) = 5.0;
	x.description = "Cutout radius (cm)";
	m_pHash.insert(ParameterKeyPair("CUTOUT_RADIUS", x));

	// Draw FP for dots or cross.
	x.description = "Draw FP for dots(0) or cross(1)";
	x.data.at(0) = 0.0;
	m_pHash.insert(ParameterKeyPair("FP_DRAW_MODE", x));

	//Object star field moving parameters
	x.description = "Object star field moving Azimuth angle (degree)";
	x.data.at(0) = 0.0;
	m_pHash.insert(ParameterKeyPair("OBJECT_AZI", x));

	x.description = "Object star field moving Elevation angle (degree)";
	x.data.at(0) = 0.0;
	m_pHash.insert(ParameterKeyPair("OBJECT_ELE", x));

	// Object lifetime.
	x.description = "Object lifetime (#frames).";
	x.data[0] = 5.0;
	m_pHash.insert(ParameterKeyPair("OBJECT_LIFETIME", x));

	// Star Object coherence factor.  0 means all stars change.
	x.description = "Object motion coherence (% out of 100).";
	x.data[0] = 100.0;
	m_pHash.insert(ParameterKeyPair("OBJECT_MOTION_COHERENCE", x));
	
	// Turns Object lifetime on and off.
	x.description = "Object lifetime on/off.";
	x.data[0] = 1.0;
	m_pHash.insert(ParameterKeyPair("OBJECT_LIFETIME_ON", x));

	// Clear color buffer each frame.
	x.description = "Clear all opengl buffer before drawing new frame.";
	x.data[0] = 1.0;
	m_pHash.insert(ParameterKeyPair("BUFFER_CLEAR", x));

	// Rotate View in countclockwise 90 degree
	x.description = "Rotate View in countclockwise 90 degree. On/Off";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("ROTATE_VIEW_90", x));

	// Let Moog control timing.
	x.description = "Let Moog control timing";
	x.data[0] = 2.0;
	m_pHash.insert(ParameterKeyPair("MOOG_CTRL_TIME", x));


	//26.11 added
	// Let Moog control timing.
	x.description = "STAIRCASE_START_VALUE";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("STAIRCASE_START_VAL", x));

	// Let Moog control timing.
	x.description = "LIGHT_CONTROL";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("LIGHT_CONTROL", x));

	// Let Moog control timing.
	x.description = "DURATION";
	x.data[0] = 1000.0;
	m_pHash.insert(ParameterKeyPair("DURATION", x));

	// Let Moog control timing.
	x.description = "ADAPTATION_ANGLE";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("ADAPTATION_ANGLE", x));

	// Let Moog control timing.
	x.description = "STIMULUS_TYPE";
	x.data[0] = 2.0;
	m_pHash.insert(ParameterKeyPair("STIMULUS_TYPE", x));
	//untill here

	//Sounds wave types.
	x.description = "WAV_TYPE";
	x.data[0] = 1.0;
	m_pHash.insert(ParameterKeyPair("WAV_TYPE", x));

	// Elevation of axis rotation.
	x.description = "Elevation of axis of rotation (degrees)";
	x.data[0] = 2.0;
	m_pHash.insert(ParameterKeyPair("ROT_ELEVATION", x));

	// Azimuth of axis rotation.
	x.description = "Azimuth of axis of rotation (degrees)";
	x.data[0] = 2.0;
	m_pHash.insert(ParameterKeyPair("ROT_AZIMUTH", x));

	// The amplitude of the rotation.
	x.description = "The amplitude of the rotation (degrees)";
	x.data[0] = 2.0;
	m_pHash.insert(ParameterKeyPair("ROT_AMPLITUDE", x));

	// Sigma of the saussian to take.
	// The amplitude of the rotation.
	x.description = "The sigma of the created gaussian";
	x.data[0] = 2.0;
	m_pHash.insert(ParameterKeyPair("ROT_SIGMA", x));
	// Sigma of the saussian to take.
	
	// The duration of the rotation.
	x.description = "The duration of the created gaussian (sec)";
	x.data[0] = 60.0;
	m_pHash.insert(ParameterKeyPair("ROT_DURATION", x));

	// Whetere the MoogDots should create it's own trajectory and use them, or to use the Matlab trajectory (if not set/received - use the Matlab's).
	x.description = "Indicate if to create and use the MoogDots trajectory or to use the Matlab trajectory (0/1)";
	x.data[0] = 0.0;
	m_pHash.insert(ParameterKeyPair("MOOG_CREATE_TRAJ", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("DISC_PLANE_AZIMUTH", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("DISC_PLANE_ELEVATION", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("DISC_PLANE_TILT", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("DISC_AMPLITUDES", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("DIST", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("DURATION", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("SIGMA", x));

	// Default point of origin of object star field.
	x.data[0] = 0.0;
	x.description = "Default point of origin (x, y, z) (cm)";
	m_pHash.insert(ParameterKeyPair("ADAPTATION_ANGLE", x));
}

void CParameterList::SetVectorData(string key, vector<double> value)
{
	g_paramLock.Enter();

	// Find the key, value pair associated with the given key.
	ParameterIterator i = m_pHash.find(key);

	// Set the key value if we found the key pair.
	if (i != m_pHash.end()) {
		i->second.data = value;
	}

	g_paramLock.Leave();
}

bool CParameterList::IsVariable(string param)
{
	bool isVariable = false;

	g_paramLock.Enter();

	// Try to find the pair associated with the given key.
	ParameterIterator i = m_pHash.find(param.c_str());

	if (i != m_pHash.end()) {
		isVariable = i->second.variable;
	}

	g_paramLock.Leave();

	return isVariable;
}

vector<double> CParameterList::GetVectorData(string key)
{
	g_paramLock.Enter();

	vector<double> value;

	// Try to find the pair associated with the given key.
	ParameterIterator i = m_pHash.find(key.c_str());

	// If we found an entry associated with the key, store the data
	// vector associated with it.
	if (i != m_pHash.end()) {
		value = i->second.data;
	}

	g_paramLock.Leave();

	return value;
}

bool CParameterList::Exists(string key)
{
	bool keyExists = false;

	g_paramLock.Enter();

	// Try to find the pair associated with the given key.
	ParameterIterator i = m_pHash.find(key.c_str());

	if (i != m_pHash.end()) {
		keyExists = true;
	}

	g_paramLock.Leave();

	return keyExists;
}

int CParameterList::GetParamSize(string param)
{
	g_paramLock.Enter();

	int paramSize = 0;

	// Try to find the pair associated with the given key.
	ParameterIterator i = m_pHash.find(param.c_str());

	if (i != m_pHash.end()) {
		paramSize = static_cast<int>(i->second.data.size());
	}

	g_paramLock.Leave();

	return paramSize;
}

string CParameterList::GetParamDescription(string param)
{
	g_paramLock.Enter();

	string s = "";

	// Find the parameter iterator.
	ParameterIterator i = m_pHash.find(param);

	if (i == m_pHash.end()) {
		s = "No Data Found";
	}
	else {
		s = i->second.description;
	}

	g_paramLock.Leave();

	return s;
}

string * CParameterList::GetKeyList(int &keyCount)
{
	g_paramLock.Enter();

	string *keyList;
	int i;

	// Number of elements in the hash.
	keyCount = m_pHash.size();

	// Initialize the key list.
	keyList = new string[keyCount];

	// Iterate through the hash and extract all the key names.
	ParameterIterator x;
	i = 0;
	for (x = m_pHash.begin(); x != m_pHash.end(); x++) {
		keyList[i] = x->first;
		i++;
	}

	g_paramLock.Leave();

	return keyList;
}

int CParameterList::GetListSize() const
{
	g_paramLock.Enter();
	int hashSize = m_pHash.size();
	g_paramLock.Leave();

	return hashSize;
}
