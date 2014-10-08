//-----------------------------------------------------------------------------
// name: HelloSine.cpp
// desc: hello sine wave, real-time
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: fall 2011
//   uses: RtAudio by Gary Scavone
//-----------------------------------------------------------------------------
#include "RtAudio.h"
#include "chuck_fft.h"
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
using namespace std;

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif




//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void initGfx();
void idleFunc();
void displayFunc();
void reshapeFunc( GLsizei width, GLsizei height );
void keyboardFunc( unsigned char, int, int );
void mouseFunc( int button, int state, int x, int y );

// our datetype
#define SAMPLE float
// corresponding format for RtAudio
#define MY_FORMAT RTAUDIO_FLOAT32
// sample rate
#define MY_SRATE 44100
// number of channels
#define MY_CHANNELS 1
// for convenience
#define MY_PIE 3.14159265358979

// width and height
long g_width = 1024;
long g_height = 720;
// global buffer
SAMPLE * g_buffer = NULL;
SAMPLE * g_freq_buffer = NULL;
long g_bufferSize;
// flags
bool g_rotate = false;
bool g_circle = false;
bool g_window_on = false;
// radius for circle
float g_radius_factor = 1.0f;
float g_radius = 1.0f;
float g_radius_base = 1.0f;
// window
SAMPLE * g_window = NULL;




//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data )
{
    // cast!
    SAMPLE * input = (SAMPLE *)inputBuffer;
    SAMPLE * output = (SAMPLE *)outputBuffer;
    
    // fill
    for( int i = 0; i < numFrames; i++ )
    {
        // assume mono
        g_buffer[i] = input[i];
        // zero output
        output[i] = 0;
    }
    
    // apply window
    apply_window(g_buffer, g_window, (unsigned long) g_bufferSize);
    
    return 0;
}

//-----------------------------------------------------------------------------
// Name: drawCircle( )
// Desc: draws a circle
//-----------------------------------------------------------------------------
void drawCircle() {
    float radius, angle, x, y;
    glBegin(GL_LINE_LOOP);
    
    for(int i =0; i <= 300; i++){
        angle = 2 * M_PI * i / 300;
        radius = g_radius_factor * g_radius + g_radius_base;
        //radius = 1.0f;
        x = (5*g_buffer[i]+radius)*cos(angle);
        y = (5*g_buffer[i]+radius)*sin(angle);

        glVertex2f(x,y);
    }
    glEnd();
}

//-----------------------------------------------------------------------------
// Name: drawSquare( )
// Desc: draws a square
//-----------------------------------------------------------------------------
void drawSquare() {
    glBegin(GL_QUADS); // Start drawing a quad primitive
    glVertex3f(-1.0f, -1.0f, -1.0f); // The bottom left corner
    glVertex3f(-1.0f, 1.0f, -1.0f); // The top left corner
    glVertex3f(1.0f, 1.0f, -0.2f); // The top right corner
    glVertex3f(1.0f, -1.0f, -0.2f); // The bottom right corner
    glEnd();
}

//-----------------------------------------------------------------------------
// Name: drawWindow( )
// Desc: draws a square
//-----------------------------------------------------------------------------
void drawWindow() {
    // step through and plot the waveform
    GLfloat x = -5;
    // increment
    GLfloat xinc = ::fabs(2*x / (g_bufferSize));

    
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < g_bufferSize; i++) {
        glVertex2f(x, g_window[i]);
        x += xinc;
    }
    glEnd();
}



//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv )
{
    // instantiate RtAudio object
    RtAudio audio;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 512;
    
    // check for audio devices
    if( audio.getDeviceCount() < 1 )
    {
        // nopes
        cout << "no audio devices found!" << endl;
        exit( 1 );
    }

    // initialize GLUT
    glutInit( &argc, argv );
    // init gfx
    initGfx();

    // let RtAudio print messages to stderr.
    audio.showWarnings( true );

    // set input and output parameters
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = audio.getDefaultInputDevice();
    iParams.nChannels = MY_CHANNELS;
    iParams.firstChannel = 0;
    oParams.deviceId = audio.getDefaultOutputDevice();
    oParams.nChannels = MY_CHANNELS;
    oParams.firstChannel = 0;
    
    // create stream options
    RtAudio::StreamOptions options;

    // go for it
    try {
        // open a stream
        audio.openStream( &oParams, &iParams, MY_FORMAT, MY_SRATE, &bufferFrames, &callme, (void *)&bufferBytes, &options );
    }
    catch( RtError& e )
    {
        // error!
        cout << e.getMessage() << endl;
        exit( 1 );
    }

    // compute
    bufferBytes = bufferFrames * MY_CHANNELS * sizeof(SAMPLE);
    // allocate global buffer
    g_bufferSize = bufferFrames;
    g_buffer = new SAMPLE[g_bufferSize];
    memset( g_buffer, 0, sizeof(SAMPLE)*g_bufferSize );
    g_window = new SAMPLE[g_bufferSize];
    
    hanning(g_window, (unsigned long)g_bufferSize);
    
    
    // go for it
    try {
        // start stream
        audio.startStream();

        // let GLUT handle the current thread from here
        glutMainLoop();
        
        // stop the stream.
        audio.stopStream();
    }
    catch( RtError& e )
    {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
cleanup:
    // close if open
    if( audio.isStreamOpen() )
        audio.closeStream();
    
    // done
    return 0;
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void initGfx()
{
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    // initialize the window size
    glutInitWindowSize( g_width, g_height );
    // set the window postion
    glutInitWindowPosition( 100, 100 );
    // create the window
    glutCreateWindow( "VisualSine" );
    
    // set the idle function - called when idle
    glutIdleFunc( idleFunc );
    // set the display function - called when redrawing
    glutDisplayFunc( displayFunc );
    // set the reshape function - called when client area changes
    glutReshapeFunc( reshapeFunc );
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc( keyboardFunc );
    // set the mouse function - called on mouse stuff
    glutMouseFunc( mouseFunc );
    
    // set clear color
    glClearColor( 0, 0, 0, 1 );
    // enable color material
    glEnable( GL_COLOR_MATERIAL );
    // enable depth test
    glEnable( GL_DEPTH_TEST );
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( GLsizei w, GLsizei h )
{
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, 1.0, 300.0 );
    // set the matrix mode to modelview
    glMatrixMode( GL_MODELVIEW );
    // load the identity matrix
    glLoadIdentity( );
    // position the view point
    gluLookAt( 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}




//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
    switch( key )
    {
        case 'Q':
        case 'q':
            exit(1);
            break;
        case 'C':
        case 'c':
            g_circle = !g_circle;
            break;
        case 'R':
        case 'r':
            g_rotate = !g_rotate;
            break;
        case 'W':
        case 'w':
            g_window_on = !g_window_on;
            break;
    }
    
    glutPostRedisplay( );
}




//-----------------------------------------------------------------------------
// Name: mouseFunc( )
// Desc: handles mouse stuff
//-----------------------------------------------------------------------------
void mouseFunc( int button, int state, int x, int y )
{
    if( button == GLUT_LEFT_BUTTON )
    {
        // when left mouse button is down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else if ( button == GLUT_RIGHT_BUTTON )
    {
        // when right mouse button down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else
    {
    }
    
    glutPostRedisplay( );
}




//-----------------------------------------------------------------------------
// Name: idleFunc( )
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc( )
{
    // render the scene
    glutPostRedisplay( );
}



//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
    // local state
    static GLfloat zrot = 0.0f, c = 0.0f, xrot = 0.0f, breathe = 0.0f, breathe_angle = 0.0f;
    
    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // line width
    glLineWidth( 2.0 );
    // step through and plot the waveform
    GLfloat x = -5;
    // increment
    GLfloat xinc = ::fabs(2*x / (g_bufferSize));
    
    // push the matrix
    glPushMatrix();
    
    // rotation
    if (g_rotate) {
        glRotatef( zrot, 0, 0, 1 );
        zrot += .1;
    } else {
        zrot = 0.0f;
    }

    // set the color
    // glColor3f( (sin(c)+1)/2, (sin(c*2)+1)/2, (sin(c+.5)+1)/2 );
    glColor3f( 1, 1, 1 );
    
    if (g_window_on) drawWindow();

    // go
    glBegin( GL_LINE_STRIP );
    // loop through global buffer
    for( int i = 0; i < g_bufferSize; i++ )
    {
        // set the next vertex
        glVertex2f( x, 5*g_buffer[i] );
        // increment x
        x += xinc;
    }
    // done
    glEnd();
    
    // rotation
    if (g_rotate) {
        glRotatef( xrot, 1, 0, 0 );
        xrot += .5;
    } else {
        xrot = 0.0f;
    }
    
    glColor3f( (sin(c)+1)/2, (sin(c*2)+1)/2, (sin(c+.5)+1)/2 );
    
    // Set up breathing circle
    breathe_angle = 2 * M_PI * breathe / 300;
    g_radius = .5*sin(breathe_angle)+1;
    
    //drawSquare();
    if (g_circle) drawCircle();
    
    // increment color
    c += .01;
    
    // increment breathing counter
    breathe += .5;
    
    // pop
    glPopMatrix();
    
    // flush!
    glFlush( );
    // swap the double buffer
    glutSwapBuffers( );
}
