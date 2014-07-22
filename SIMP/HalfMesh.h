#ifndef HALFMESH_H
#define HALFMESH_H

#include "glm.h"//obj model loader

#include<vector>
using std::vector;

#include<map>
using std::map;
using std::pair;
using std::make_pair;

struct HEVertex;
struct HEFace;

typedef float Point3D[3];

struct HalfEdge
{
    HEVertex* vertex_begin;
    HalfEdge* next_edge;
    HalfEdge* paired_edge;
    HEFace* left_face;
};

struct HEVertex
{
    Point3D coordinate;
    HalfEdge* heEdge;
};

struct HEFace
{
    HalfEdge* heEdge;
};

class HalfMesh
{
public:
    map<GLuint, HEVertex*> vertices;

    //pair<from, to>
    map< pair<GLuint, GLuint>, HalfEdge* > halfEdges;

    map<GLuint, HEFace*> heFaces;

    HalfMesh(GLMmodel *m)
    {
        ////populate vertex coordinate data
        HEVertex *hv = NULL;
        GLuint idx;
        for(GLuint i = 0; i < m->numvertices; i++)
        {
            hv = new HEVertex();

            idx = i * 3 + 3;
            hv->coordinate[0] = m->vertices[idx];
            hv->coordinate[1] = m->vertices[idx + 1];
            hv->coordinate[2] = m->vertices[idx + 2];

            vertices.insert(make_pair(i + 1, hv));
        }
        ////

        ////populate halfedge
        HalfEdge *he = NULL;
        HEFace *hef = NULL;
        GLMtriangle *mt = NULL;
        GLuint triangleIdx;

        GLMgroup *gp = m->groups;
        while(gp)
        {
            for(GLuint i = 0; i < gp->numtriangles; i++)
            {
                triangleIdx = gp->triangles[i];
                mt = &m->triangles[triangleIdx];

                hef = new HEFace();
                heFaces.insert(make_pair(triangleIdx, hef));

                GLuint edges[][2] =
                {
                    {mt->vindices[0],mt->vindices[1]},
                    {mt->vindices[1],mt->vindices[2]},
                    {mt->vindices[2],mt->vindices[0]}
                };

                HalfEdge *hes[3];
                for(int j = 0; j < 3; j++)
                {
                    GLuint u = edges[j][0];
                    GLuint v = edges[j][1];

                    he = new HalfEdge();
                    he->vertex_begin = vertices[u];
                    he->left_face = hef;

                    halfEdges.insert(make_pair(make_pair(u, v), he));
                    hes[j] = he;
                }

                //assigne next half edge for each half edge in face F
                hes[0]->next_edge = hes[1];
                hes[1]->next_edge = hes[2];
                hes[2]->next_edge = hes[0];

                hef->heEdge = hes[0];
            }

            gp = gp->next;
        }

        //// assign half edge pair field
        map< pair<GLuint, GLuint>, HalfEdge* >::const_iterator citer = halfEdges.begin();
        pair<GLuint, GLuint> e;
        pair<GLuint, GLuint> inverse_e;
        while(citer != halfEdges.end())
        {
            e = citer->first;
            he = citer->second;

            inverse_e.first = e.second;
            inverse_e.second = e.first;

            if(halfEdges.find(inverse_e) != halfEdges.end())
            {
                halfEdges[inverse_e]->paired_edge = halfEdges[e];
                halfEdges[e]->paired_edge = halfEdges[inverse_e];
            }

            citer++;
        }
        ////
        ////
    }

private:

};

#endif