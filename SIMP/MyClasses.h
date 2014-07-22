
/* Using the standard output for fprintf */
#include <stdio.h>
#include <stdlib.h>
/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/freeglut.h>

#include "glm.h"//obj model loader

#include <glm/glm.hpp>

#include<vector>
using std::vector;

#include<limits>
using std::numeric_limits;

struct xDMaterial
{
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

class xDModel
{
public:
    vector<glm::vec3> vertices;
    vector<GLuint> elements;

    vector<glm::vec2> texcoords;
    vector<glm::vec2> vert_texcoord;

    vector<glm::vec3> normals;
    vector<glm::vec3> vert_normal;

    glm::vec3 center;
    GLfloat boundingShpereRadius;

    GLMmodel *glmModel;

    xDModel(char *filename)
    {
        glmModel = NULL;

        glmModel = glmReadOBJ(filename);

        fillIn();
        calCenterPos();
    }

    void fillIn()
    {
		//////////////////////////////////////////////////////
		////                                              ////
		//// there is problem assigning the texcoords     ////
		////                                              ////
		//////////////////////////////////////////////////////


        //vertices coordinates
        GLfloat *ptr = &glmModel->vertices[3];
        for(int i = 0; i < glmModel->numvertices; i++)
        {
            vertices.push_back(glm::vec3(ptr[0], ptr[1], ptr[2]));
            ptr += 3;
        }

        //texture coordinates
        ptr = &glmModel->texcoords[2];
        for(int i = 0; i < glmModel->numtexcoords; i++)
        {
            texcoords.push_back(glm::vec2(ptr[0], ptr[1]));
            ptr += 2;
        }

        //normals
        ptr = &glmModel->normals[3];
        for(int i = 0; i < glmModel->numnormals; i++)
        {
            normals.push_back(glm::vec3(ptr[0], ptr[1], ptr[2]));
            ptr += 3;
        }

        vert_normal = vector<glm::vec3>(glmModel->numvertices, glm::vec3(0.0f));
        vert_texcoord = vector<glm::vec2>(glmModel->numvertices, glm::vec2(0.0f));

        for(int i = 0; i < glmModel->numtriangles; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                GLuint vIdx = glmModel->triangles[i].vindices[j] - 1;
                GLuint nIdx = glmModel->triangles[i].nindices[j] - 1;
                GLuint tIdx = glmModel->triangles[i].tindices[j] - 1;

                elements.push_back(vIdx);

                vert_normal[vIdx] = normals[nIdx];
                vert_texcoord[vIdx] = texcoords[tIdx];
            }
        }
    }

    void calCenterPos()
    {
        glm::vec3 min(numeric_limits<float>::max());
        glm::vec3 max(numeric_limits<float>::min());

        for(int i = 0; i < vertices.size(); i++)
        {
            if(vertices[i].x < min.x)
                min.x = vertices[i].x;
            if(vertices[i].y < min.y)
                min.y = vertices[i].y;
            if(vertices[i].z < min.z)
                min.z = vertices[i].z;

            if(vertices[i].x > max.x)
                max.x = vertices[i].x;
            if(vertices[i].y > max.y)
                max.y = vertices[i].y;
            if(vertices[i].z > max.z)
                max.z = vertices[i].z;
        }

        center.x = (min.x + max.x) / 2.0f;
        center.y = (min.y + max.y) / 2.0f;
        center.z = (min.z + max.z) / 2.0f;

        boundingShpereRadius = sqrtf(((center.x - min.x) * (center.x - min.x)
                                      + (center.y - min.y) * (center.y - min.y)
                                      + (center.z - min.z) * (center.z - min.z)));
    }


    ~xDModel()
    {
        if(glmModel)
            glmDelete(glmModel);
    }


};