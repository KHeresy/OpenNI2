// Example to display OpenNI mesh with OpenGL
//
// 		by Heresy
// 		http://kheresy.wordpress.com
//
// version 1.01 @2013/03/12

// STL Header
#include <iostream>

// glut Header
#include <GL/glut.h>

// OpenNI Header
#include <OpenNI.h>

#include "OpenGLCamera.h"
#include "../vDeviceNiTE1/VirtualDevice.h"

// namespace
using namespace std;
using namespace openni;

// global OpenNI object
Device			g_devDevice;
VideoStream*	g_vsDepthStream;
VideoStream*	g_vsColorStream;

// global object
SimpleCamera	g_Camera;
float*			g_pPoints;
unsigned char*	g_pColors;
unsigned int*	g_pIndex;
unsigned int	g_uWidth, g_uHeight;

// global flag
bool	g_bUseCoordinateConverter = false;
bool	g_bHaveColorSensor = true;

// function to build triangle
inline bool BuildTriangle( unsigned int& iIdx, unsigned int v1, unsigned int v2, unsigned int v3, float th = 500.0f )
{
	float	z1 = g_pPoints[ 3* v1 + 2 ],
		z2 = g_pPoints[ 3* v2 + 2 ],
		z3 = g_pPoints[ 3* v3 + 2 ];

	if( z1 > 0 && z1 > 0 && z3 > 0 )
	{
		if( abs( z1 - z2 ) > th ||
			abs( z1 - z3 ) > th ||
			abs( z2 - z3 ) > th )
			return false;

		g_pIndex[iIdx++] = v1;
		g_pIndex[iIdx++] = v2;
		g_pIndex[iIdx++] = v3;

		return true;
	}
	return false;
}

// glut display function(draw)
void display()
{
	// clear previous screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// get depth frame
	VideoFrameRef	vfDepthFrame;
	if( g_vsDepthStream->readFrame( &vfDepthFrame ) == STATUS_OK )
	{
		const DepthPixel* pDepthArray = static_cast<const DepthPixel*>( vfDepthFrame.getData() );
		int iW = vfDepthFrame.getWidth(),
			iH = vfDepthFrame.getHeight();

		// update data array
		for( int y = 0; y < iH; ++ y )
		{
			for( int x = 0; x < iW; ++ x )
			{
				int idx = x + y * iW;

				// update points array
				const DepthPixel& rDepth = pDepthArray[idx];
				if( rDepth > 0 )
				{
					int i = idx * 3;
					CoordinateConverter::convertDepthToWorld( *g_vsDepthStream, x, y, rDepth, &(g_pPoints[i]), &(g_pPoints[i+1]), &(g_pPoints[i+2]) );
				}
			}
		}
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 3, GL_FLOAT, 0, g_pPoints );

		glEnableClientState( GL_COLOR_ARRAY );
		// check is color stream is available
		if( g_bHaveColorSensor )
		{
			// get color frame
			VideoFrameRef	vfColorFrame;
			if( g_vsColorStream->readFrame( &vfColorFrame ) == STATUS_OK )
			{
				if( g_bUseCoordinateConverter )
				{
					const RGB888Pixel* pColorArray = static_cast<const RGB888Pixel*>( vfColorFrame.getData() );
					for( int y = 0; y < iH; ++ y )
					{
						for( int x = 0; x < iW; ++ x )
						{
							// update points array
							int idx = x + y * iW;
							const DepthPixel& rDepth = pDepthArray[idx];
							if( rDepth > 0 )
							{
								int cX, cY;
								CoordinateConverter::convertDepthToColor( *g_vsDepthStream, *g_vsColorStream, x, y, rDepth, &cX, &cY );
								if( cX >= 0 && cX < iW && cY >= 0 && cY < iH )
								{
									int idx2 = ( cX + cY * iW );
									g_pColors[ 3 * idx     ] = pColorArray[idx2].r;
									g_pColors[ 3 * idx + 1 ] = pColorArray[idx2].g;
									g_pColors[ 3 * idx + 2 ] = pColorArray[idx2].b;
								}
							}
						}
					}
					glColorPointer( 3, GL_UNSIGNED_BYTE, 0, g_pColors );
				}
				else
				{
					glColorPointer( 3, GL_UNSIGNED_BYTE, 0, vfColorFrame.getData() );
				}
			}
		}
		else
		{
			// build color by depth
			for( int y = 0; y < iH; ++ y )
			{
				for( int x = 0; x < iW; ++ x )
				{
					int idx = x + y * iW;
					const DepthPixel& rDepth = pDepthArray[idx];

					g_pColors[ 3 * idx     ] = 255 * rDepth / 10000;
					g_pColors[ 3 * idx + 1 ] = 0;
					g_pColors[ 3 * idx + 2 ] = 0;
				}
			}
			glColorPointer( 3, GL_UNSIGNED_BYTE, 0, g_pColors );
		}

		unsigned int uTriangles = 0;
		for( int y = 0; y < iH - 1; ++ y )
		{
			for( int x = 0; x < iW - 1; ++ x )
			{
				unsigned int idx = x + y * iW;
				BuildTriangle( uTriangles, idx, idx + 1, idx + iW );
				BuildTriangle( uTriangles, idx + 1, idx + iW + 1, idx + iW );
			}
		}

		// draw
		glDrawElements( GL_TRIANGLES, uTriangles, GL_UNSIGNED_INT, g_pIndex );

		glDisableClientState( GL_COLOR_ARRAY );
		glDisableClientState( GL_VERTEX_ARRAY );
	}

	// Coordinate
	glLineWidth( 5.0f );
	glBegin( GL_LINES );
	glColor3ub( 255, 0, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 100, 0, 0 );

	glColor3ub( 0, 255, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 100, 0 );

	glColor3ub( 0, 0, 255 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 0, 100 );
	glEnd();
	glLineWidth( 1.0f );

	// swap buffer
	glutSwapBuffers();
}

// glut idle function
void idle()
{
	glutPostRedisplay();
}

// glut keyboard function
void keyboard( unsigned char key, int x, int y )
{
	float fSpeed = 50.0f;
	switch( key )
	{
	case 'q':
		// release memory
		delete [] g_pPoints;
		delete [] g_pColors;
		delete [] g_pIndex;

		// stop OpenNI
		g_vsDepthStream->destroy();
		g_vsColorStream->destroy();
		g_devDevice.close();
		OpenNI::shutdown();
		exit( 0 );

	case 's':
		g_Camera.MoveForward( -fSpeed );
		break;

	case 'w':
		g_Camera.MoveForward( fSpeed );
		break;

	case 'a':
		g_Camera.MoveSide( -fSpeed );
		break;

	case 'd':
		g_Camera.MoveSide( fSpeed );
		break;

	case 'z':
		g_Camera.MoveUp( -fSpeed );
		break;

	case 'x':
		g_Camera.MoveUp( fSpeed );
		break;

	case 'p':
		{
			static bool bPoly = true;
			if( bPoly )
			{
				bPoly = false;
				glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			}
			else
			{
				bPoly = true;
				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			}
		}
		break;
	}
}

// glut special keyboard function
void specialKey( int key, int x, int y )
{
	float fRotateScale = 0.01f;
	switch( key )
	{
	case GLUT_KEY_DOWN:
		g_Camera.RotateUp( -fRotateScale );
		break;

	case GLUT_KEY_UP:
		g_Camera.RotateUp( fRotateScale );
		break;

	case GLUT_KEY_RIGHT:
		g_Camera.RotateSide( fRotateScale );
		break;

	case GLUT_KEY_LEFT:
		g_Camera.RotateSide( -fRotateScale );
		break;
	}
}

int main( int argc, char** argv )
{
#pragma region OpenNI initialize
	// Initial OpenNI
	if( OpenNI::initialize() != STATUS_OK )
	{
		cerr << "OpenNI Initial Error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Open Device
	if( g_devDevice.open( ANY_DEVICE ) != STATUS_OK )
	{
		cerr << "Can't Open Device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Create depth stream
	VideoStream vsDepth;
	if( g_devDevice.hasSensor( SENSOR_DEPTH ) )
	{
		if( vsDepth.create( g_devDevice, SENSOR_DEPTH ) == STATUS_OK )
		{
			//vsDepth.setMirroringEnabled( false );
			// set video mode
			VideoMode mMode;
			mMode.setResolution( 640, 480 );
			mMode.setFps( 30 );
			mMode.setPixelFormat( PIXEL_FORMAT_DEPTH_1_MM );

			if( vsDepth.setVideoMode( mMode) != STATUS_OK )
			{
				cout << "Can't apply VideoMode: " << OpenNI::getExtendedError() << endl;
			}
		}
		else
		{
			cerr << "Can't create depth stream on device: " << OpenNI::getExtendedError() << endl;
			return -1;
		}
	}
	else
	{
		cerr << "ERROR: This device does not have depth sensor" << endl;
		return -1;
	}

	// Create color stream
	VideoStream vsColor;
	if( g_devDevice.hasSensor( SENSOR_COLOR ) )
	{
		if( vsColor.create( g_devDevice, SENSOR_COLOR ) == STATUS_OK )
		{
			//vsColor.setMirroringEnabled( false );
			// set video mode
			VideoMode mMode;
			mMode.setResolution( 640, 480 );
			mMode.setFps( 30 );
			mMode.setPixelFormat( PIXEL_FORMAT_RGB888 );

			if( vsColor.setVideoMode( mMode) != STATUS_OK )
			{
				cout << "Can't apply VideoMode: " << OpenNI::getExtendedError() << endl;
			}

			// image registration
			if( g_devDevice.isImageRegistrationModeSupported( IMAGE_REGISTRATION_DEPTH_TO_COLOR ) )
			{
				g_devDevice.setImageRegistrationMode( IMAGE_REGISTRATION_DEPTH_TO_COLOR );
			}
			else
			{
				cout << "This device doesn't support IMAGE_REGISTRATION_DEPTH_TO_COLOR, will use CoordinateConverter." << endl;
				g_bUseCoordinateConverter = true;
			}
		}
		else
		{
			cerr <<  "Can't create color stream on device: " << OpenNI::getExtendedError() << endl;
			g_bHaveColorSensor = false;
		}
	}
	else
	{
		cerr << "This device does not have depth sensor" << endl;
		g_bHaveColorSensor = false;
	}
#pragma endregion

#pragma region Virtual Device
	// Open Virtual Device
	Device	virDevice;
	if( virDevice.open( "\\OpenNI2\\VirtualDevice\\TEST" ) != STATUS_OK )
	{
		cerr << "Can't Open Device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	g_vsColorStream = CreateVirtualStream( virDevice, vsColor, []( const OniFrame& rF1,OniFrame& rF2 ){
		RGB888Pixel* pImg1 = (RGB888Pixel*)rF1.data;
		RGB888Pixel* pImg2 = (RGB888Pixel*)rF2.data;
		for( int y = 0; y < rF2.height; ++ y )
		{
			for( int x = 0; x < rF2.width; ++ x )
			{
				pImg2[ x + y * rF2.width ] = pImg1[ x + y * rF2.width ];
			}
		}
	} );

	g_vsDepthStream = CreateVirtualStream( virDevice, vsDepth, []( const OniFrame& rF1,OniFrame& rF2 ){
		DepthPixel* pImg1 = (DepthPixel*)rF1.data;
		DepthPixel* pImg2 = (DepthPixel*)rF2.data;
		for( int y = 0; y < rF2.height; ++ y )
		{
			for( int x = 0; x < rF2.width; ++ x )
			{
				pImg2[ x + y * rF2.width ] = pImg1[ x + y * rF2.width ];
			}
		}
	} );
#pragma endregion

#pragma region OpenGL Initialize
	// initial glut
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );

	// create glut window
	glutInitWindowSize( 640, 480 );
	glutCreateWindow( "OpenNI 3D OpenGL 3D Mesh" );

	// set OpenGL environment
	glEnable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );

	// OpenGL projection
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 40.0, 1.0, 100.0, 20000.0 );	// FOV, aspect ration, near, far
	g_Camera.vCenter	= Vector3( 0.0, 0.0, 1000.0 );
	g_Camera.vPosition	= Vector3( 0.0, 0.0, -2000.0 );
	g_Camera.vUpper		= Vector3( 0.0, 1.0, 0.0 );
	g_Camera.SetCamera();

	// register glut callback functions
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutKeyboardFunc( keyboard );
	glutSpecialFunc( specialKey );
#pragma endregion

#pragma region allocation memory
	// allocate memory
	VideoMode mMode = vsDepth.getVideoMode();
	int g_uWidth = mMode.getResolutionX(),
		g_uHeight = mMode.getResolutionY();
	g_pPoints	= new float[ g_uWidth * g_uHeight * 3 ];
	g_pColors	= new unsigned char[ g_uWidth * g_uHeight * 3 ];
	g_pIndex	= new unsigned int[ g_uWidth * g_uHeight * 6 ];
#pragma endregion

	// start
	if( vsDepth.start() == STATUS_OK )
	{
		vsColor.start();
		g_vsDepthStream->start();
		g_vsColorStream->start();
		glutMainLoop();
	}
	else
	{
		cerr << "Stream start error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	return 0;
}
