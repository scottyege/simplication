
/* Using the standard output for fprintf */
#include <stdio.h>
#include <stdlib.h>

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/freeglut.h>

#include "shader_utils.h"

#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SOIL.h>

#include<iostream>
using std::cerr;
using std::cout;

#include "MyClasses.h"

#include "HalfMesh.h"

/* ADD GLOBAL VARIABLES HERE LATER */
GLuint program;

xDModel *xdModel = NULL;
HalfMesh *halfMesh = NULL;

int screen_width = 800, screen_height = 600;

int last_mx = 0, last_my = 0, cur_mx = 0, cur_my = 0;
int arcball_on = false;

struct FPSCount
{
    int frame;
    int timebase;
    int time;
    float fps;
} fpsCount = {0, 0, 0, 0.0f};

struct CameraProperty
{
    glm::vec3 eye;
    glm::vec3 at;
    glm::vec3 up;

    GLfloat fov;
} cameraProp;

struct ShaderAttriLoc
{
    GLuint vObjPos;
} attriLoc;

struct ShaderUniformLoc
{
    GLuint modelMatrix;
    GLuint viewMatrix;
    GLuint projMatrix;

    GLuint fcolor;
} uniformLoc;

char filename[] = "gourd.obj";
/*
cube
gourd
sphere
teapot
teapot2
tetrahedron
*/

void print_bitmap_string(void* font, char* s)
{
    if (s && strlen(s))
    {
        while (*s)
        {
            glutBitmapCharacter(font, *s);
            s++;
        }
    }
}

void calCameraSet()
{
    cameraProp.up = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraProp.at = glm::vec3(0.0f, 0.0f, 0.0f);;

    cameraProp.fov = 30.0f;

    float distanceToCamera = xdModel->boundingShpereRadius / tanf(M_PI * cameraProp.fov / 360.0f);
    cameraProp.eye = xdModel->center;
    cameraProp.eye.z = distanceToCamera;
}

/*
Function: init_resources
Receives: void
Returns: int
This function creates all GLSL related stuff
explained in this example.
Returns 1 when all is ok, 0 with a displayed error
*/
int init_resources(void)
{
    //loading model
    xdModel = new xDModel(filename);
    halfMesh = new HalfMesh(xdModel->glmModel);

    //
    //setting shader
    GLint link_ok = GL_FALSE;

    GLuint vs, fs;
    if ((vs = create_shader("triangle.vert", GL_VERTEX_SHADER))   == 0) return 0;
    if ((fs = create_shader("triangle.frag", GL_FRAGMENT_SHADER)) == 0) return 0;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        fprintf(stderr, "glLinkProgram:");
        print_log(program);
        return 0;
    }


    ////attributes
    if(!getAttributeLoc(program, "vObjPos", attriLoc.vObjPos)
      )
        return 0;

    //uniforms
    if(
        !getUniformLoc(program, "modelMatrix", uniformLoc.modelMatrix)
        || !getUniformLoc(program, "projMatrix", uniformLoc.projMatrix)
        || !getUniformLoc(program, "viewMatrix", uniformLoc.viewMatrix)
        || !getUniformLoc(program, "fColor", uniformLoc.fcolor)
    )
        return 0;

    calCameraSet();

    return 1;
}

glm::mat4 moveToCenter;
glm::mat4 teapotMatrix(1.0f);

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;



void onDisplay()
{

    /* Clear the background as white */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT, GL_LINE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(program);

    glEnableVertexAttribArray(attriLoc.vObjPos);

    glUniformMatrix4fv(uniformLoc.modelMatrix, 1, GL_FALSE, glm::value_ptr(teapotMatrix * moveToCenter));
    glUniformMatrix4fv(uniformLoc.viewMatrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(uniformLoc.projMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glUniform4fv(uniformLoc.fcolor, 1, glm::value_ptr(glm::vec4(1.0f)));
    glBegin(GL_TRIANGLES);
    map<GLuint, HEFace*>::const_iterator citer = halfMesh->heFaces.begin();
    HEFace *hf = NULL;
    while(citer != halfMesh->heFaces.end())
    {
        hf = citer->second;
		if(hf->heEdge && !hf->isBoundaryFace)
        {
            glVertexAttrib3fv(attriLoc.vObjPos, hf->heEdge->vertex_begin->coordinate);
            glVertexAttrib3fv(attriLoc.vObjPos, hf->heEdge->next_edge->vertex_begin->coordinate);
            glVertexAttrib3fv(attriLoc.vObjPos, hf->heEdge->next_edge->next_edge->vertex_begin->coordinate);
        }
        citer++;
    }
    glEnd();

    glDisableVertexAttribArray(attriLoc.vObjPos);

    glUseProgram(0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-400, 400, -300, 300);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRasterPos2f(-400.0f, 280.0f);
    char aa[100];
    sprintf(aa, "fps: %.2f", fpsCount.fps);

    glColor4f(0.0, 1.0, 0.0, 0.0);
    print_bitmap_string(GLUT_BITMAP_TIMES_ROMAN_24, aa);

    /* Display the result */
    glutSwapBuffers();
}

void free_resources()
{
    glDeleteProgram(program);

    if(xdModel)
        delete xdModel;
    if(halfMesh)
        delete halfMesh;
}

void onReshape(int width, int height)
{
    screen_width = width;
    screen_height = height;
    glViewport(0, 0, screen_width, screen_height);
}

/**
 * Get a normalized vector from the center of the virtual ball O to a
 * point P on the virtual ball surface, such that P is aligned on
 * screen's (X,Y) coordinates.  If (X,Y) is too far away from the
 * sphere, return the nearest point on the virtual ball surface.
 */
glm::vec3 get_arcball_vector(int x, int y)
{
    glm::vec3 P = glm::vec3(1.0*x/screen_width*2 - 1.0,
                            1.0*y/screen_height*2 - 1.0,
                            0);
    P.y = -P.y;
    float OP_squared = P.x * P.x + P.y * P.y;
    if (OP_squared <= 1*1)
        P.z = sqrt(1*1 - OP_squared);  // Pythagore
    else
        P = glm::normalize(P);  // nearest point
    return P;
}

void onIdle()
{

    fpsCount.frame++;
    fpsCount.time = glutGet(GLUT_ELAPSED_TIME);

    if (fpsCount.time - fpsCount.timebase > 1000)
    {
        fpsCount.fps = fpsCount.frame * 1000.0 / (fpsCount.time - fpsCount.timebase);
        fpsCount.timebase = fpsCount.time;
        fpsCount.frame = 0;
        //printf("fps: %f\n", fps);
    }


    moveToCenter = glm::translate(glm::mat4(1.0f), -xdModel->center);

    //translate and rotate camera position
    viewMatrix = glm::lookAt(cameraProp.eye, cameraProp.at, cameraProp.up);

    //projection matrix
    projectionMatrix = glm::perspective(cameraProp.fov, 1.0f*screen_width/screen_height, 0.1f, 1000.0f);

    if (cur_mx != last_mx || cur_my != last_my)
    {
        glm::vec3 va = get_arcball_vector(last_mx, last_my);
        glm::vec3 vb = get_arcball_vector( cur_mx,  cur_my);
        float angle = acos(glm::min(1.0f, glm::dot(va, vb)));
        glm::vec3 axis_in_camera_coord = glm::cross(va, vb);
        glm::mat3 camera2object = glm::mat3(glm::inverse(viewMatrix)) * glm::mat3(teapotMatrix);
        glm::vec3 axis_in_object_coord = camera2object * axis_in_camera_coord;

        teapotMatrix = glm::rotate(teapotMatrix, glm::degrees(angle), axis_in_object_coord);
        last_mx = cur_mx;
        last_my = cur_my;
    }

    glutPostRedisplay();
}

void onMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        arcball_on = true;
        last_mx = cur_mx = x;
        last_my = cur_my = y;
    }
    else
    {
        arcball_on = false;
    }
}

void onMotion(int x, int y)
{
    if (arcball_on)    // if left button is pressed
    {
        cur_mx = x;
        cur_my = y;
    }
}

void MyKeyboardFunc(unsigned char c, int x, int y)
{
    if(c == 'z')
    {
        if(cameraProp.fov >= 15)
            cameraProp.fov -= 5;
    }
    else if(c == 'Z')
    {
        if(cameraProp.fov <= 80)
            cameraProp.fov += 5;
    }
    else if(c == 'c')
    {
        printf("%f, %f, %f\n", xdModel->center[0], xdModel->center[1], xdModel->center[2]);
    }
    else if(c == ' ')
    {
        halfMesh->randomCollapse();
    }
}

int main(int argc, char* argv[])
{
    /* Glut-related initialising functions */
    glutInit(&argc, argv);
    glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_ALPHA);
    glutInitWindowSize(screen_width, screen_height);
    glutInitWindowPosition(800, 400);
    glutCreateWindow("ZZZZZZZZ");

    /* Extension wrangler initialising */
    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return EXIT_FAILURE;
    }

    /* When all init functions run without errors,
    the program can initialise the resources */
    if (1 == init_resources())
    {
        /* We can display it if everything goes OK */
        glutDisplayFunc(onDisplay);
        glutIdleFunc(onIdle);
        glutReshapeFunc(onReshape);

        glutMouseFunc(onMouse);
        glutMotionFunc(onMotion);

        glutKeyboardFunc(MyKeyboardFunc);

        glutMainLoop();
    }

    /* If the program exits in the usual way,
    free resources and exit with a success */
    free_resources();
    return EXIT_SUCCESS;
}