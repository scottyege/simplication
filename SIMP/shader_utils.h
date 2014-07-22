#ifndef _CREATE_SHADER_H
#define _CREATE_SHADER_H
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
char* file_read(const char* filename);
void print_log(GLuint object);
GLuint create_shader(const char* filename, GLenum type);
int getAttributeLoc(GLuint program, const GLchar *name, GLuint &attLoc);
int getUniformLoc(GLuint program, const char *name, GLuint &uniLoc);
#endif