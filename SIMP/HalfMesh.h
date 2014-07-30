#ifndef HALFMESH_H
#define HALFMESH_H

#include "glm.h"//obj model loader

#include<vector>
using std::vector;

#include<map>
using std::map;
using std::pair;
using std::make_pair;

#include<queue>
using std::queue;

struct HEVertex;
struct HEFace;
struct HalfEdge;

#include<ctime>
#include<cstdlib>

#include<algorithm>
using std::min_element;

typedef float Point3D[3];


struct HEMetric
{
    HalfEdge *he;
    float length;

    bool operator() (HEMetric a, HEMetric b)
    {
        return a.length < b.length;
    }

    static float edgeDistance(Point3D v1, Point3D v2)
    {
        return sqrtf((v1[0] - v2[0]) * (v1[0] - v2[0])
                     + (v1[1] - v2[1]) * (v1[1] - v2[1])
                     + (v1[2] - v2[2]) * (v1[2] - v2[2]));
    }
};



struct HalfEdge
{
    HEVertex* vertex_begin;
    HalfEdge* next_edge;
    HalfEdge* paired_edge;
    HEFace* left_face;
};

struct HEVertex
{
    GLuint id;
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

            hv->heEdge = NULL;
            hv->id = i + 1;

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

                    if(vertices.find(u)->second->heEdge == NULL)
                    {
                        //if the vertex u has not associated to a half edge
                        vertices.find(u)->second->heEdge = he;
                    }
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
                //a paired half edge is found
                halfEdges[inverse_e]->paired_edge = halfEdges[e];
                halfEdges[e]->paired_edge = halfEdges[inverse_e];
            }
            else
            {
                /*if a paired half edge is not found, it is a boundadry edge*/
                HalfEdge *hee = new HalfEdge();
                hee->left_face = NULL;
                hee->paired_edge = halfEdges[e];
                hee->vertex_begin = vertices[e.second];
                hee->next_edge = NULL;

                halfEdges[e]->paired_edge = hee;
                halfEdges.insert(make_pair(make_pair(inverse_e.first, inverse_e.second), hee));
            }

            citer++;
        }
        ////
        ////

        //check
        citer = halfEdges.begin();
        while(citer != halfEdges.end())
        {
            if(!citer->second->paired_edge)
                printf("GGGGGGGGGGGGGGGG\n");
            citer++;
        }
    }

    void randomCollapse()
    {

        map< pair<GLuint, GLuint>, HalfEdge* >::iterator iter = halfEdges.begin();

        HEMetric mc;
        vector<HEMetric> cc;

        while(iter != halfEdges.end())
        {
            mc.he = iter->second;
            if(mc.he->left_face && mc.he->next_edge->left_face && mc.he->next_edge->next_edge->left_face) //exclude the boundary edge
            {
                mc.length = HEMetric::edgeDistance(vertices[iter->first.first]->coordinate, vertices[iter->first.second]->coordinate);
                cc.push_back(mc);
            }
            iter++;
        }

        vector<HEMetric>::iterator m = min_element(cc.begin(), cc.end(), mc);

        GLuint u = m->he->vertex_begin->id;
        GLuint v = m->he->paired_edge->vertex_begin->id;

        while(halfEdges.find(make_pair(u, v)) == halfEdges.end())
        {
            printf("cannot find edge %u, %u, regenerate\n", u, v);
            cc.erase(m);

            m = min_element(cc.begin(), cc.end(), mc);
            u = m->he->vertex_begin->id;
            v = m->he->paired_edge->vertex_begin->id;
        }

        printf("collapse! %u, %u, length: %f\n", u, v, m->length);

        edgeCollapse(u, v);
    }

    void edgeCollapse(GLuint u, GLuint v)
    {

        pair<GLuint, GLuint> uv(u, v);
        pair<GLuint, GLuint> vu(v, u);

        //calculate new vertex position
        Point3D mid =
        {
            (vertices[u]->coordinate[0] + vertices[v]->coordinate[0]) / 2.0f,
            (vertices[u]->coordinate[1] + vertices[v]->coordinate[1]) / 2.0f,
            (vertices[u]->coordinate[2] + vertices[v]->coordinate[2]) / 2.0f,
        };
        HEVertex *hv_mid = new HEVertex();
        hv_mid->coordinate[0] = mid[0];
        hv_mid->coordinate[1] = mid[1];
        hv_mid->coordinate[2] = mid[2];
        GLuint hv_mid_id = vertices.size() + 1;
        hv_mid->id = hv_mid_id;
        hv_mid->heEdge = vertices[v]->heEdge;
        vertices.insert(make_pair(hv_mid_id, hv_mid));

        HEFace *face1 = halfEdges[uv]->left_face;
        HEFace *face2 = halfEdges[vu]->left_face;

        vector< pair< pair<GLuint, GLuint>, HalfEdge* > > adjEdges;
        HalfEdge *he = halfEdges[uv]->paired_edge->next_edge;

        adjEdges.push_back(make_pair(make_pair(u, v), halfEdges[uv]));
        adjEdges.push_back(make_pair(make_pair(v, u), halfEdges[vu]));
        while(he != halfEdges[uv])
        {
            pair<GLuint, GLuint> pq(u, he->paired_edge->vertex_begin->id);
            pair<GLuint, GLuint> qp(pq.second, pq.first);

            adjEdges.push_back(make_pair(pq, he));
            adjEdges.push_back(make_pair(qp, he->paired_edge));

            halfEdges.erase(pq);
            halfEdges.erase(qp);

            he = he->paired_edge->next_edge;
        }

        he = halfEdges[vu]->paired_edge->next_edge;
        while(he != halfEdges[vu])
        {
            pair<GLuint, GLuint> pq(v, he->paired_edge->vertex_begin->id);
            pair<GLuint, GLuint> qp(pq.second, pq.first);

            adjEdges.push_back(make_pair(pq, he));
            adjEdges.push_back(make_pair(qp, he->paired_edge));

            halfEdges.erase(pq);
            halfEdges.erase(qp);

            he = he->paired_edge->next_edge;
        }

        for(int i = 0; i < adjEdges.size(); i++)
        {
            if(adjEdges[i].first.first == u || adjEdges[i].first.first == v)
            {
                adjEdges[i].first.first = hv_mid_id;
                adjEdges[i].second->vertex_begin = hv_mid;
            }
            else if(adjEdges[i].first.second == u || adjEdges[i].first.second == v)
            {
                adjEdges[i].first.second = hv_mid_id;
            }
        }

        for(int i = 0; i < adjEdges.size(); i++)
        {
            if(adjEdges[i].second->left_face == face1 || adjEdges[i].second->left_face == face2)
            {
                adjEdges[i].second = NULL;
            }
        }

        for(int i = 0; i < adjEdges.size(); i++)
        {
            if(!adjEdges[i].second)
                continue;

            for(int j = 0; j < adjEdges.size(); j++)
            {

                if(!adjEdges[j].second)
                    continue;

                //paring the half edge will repeat, but the result is not affected
                if(adjEdges[i].first.first == adjEdges[j].first.second
                        && adjEdges[i].first.second == adjEdges[j].first.first)
                {
                    adjEdges[i].second->paired_edge = adjEdges[j].second;
                    adjEdges[j].second->paired_edge = adjEdges[i].second;
                    break;
                }
            }
        }

        for(int i = 0; i < adjEdges.size(); i++)
        {
            if(adjEdges[i].second)
                halfEdges.insert(adjEdges[i]);
        }

        face1->heEdge = NULL;
        face2->heEdge = NULL;
    }

private:

};

#endif