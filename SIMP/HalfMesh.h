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

    GLuint id;
};

struct HEVertex
{
    GLuint id;
    Point3D coordinate;
    HalfEdge* heEdge;
};

struct HEFace
{
    GLuint id;
    HalfEdge* heEdge;
    bool isBoundaryFace;
};

class HalfMesh
{
public:
    map<GLuint, HEVertex*> vertices;

    //pair<from, to>
    map< pair<GLuint, GLuint>, HalfEdge* > halfEdges;

    map<GLuint, HEFace*> heFaces;

    GLuint nextNewIdCount;

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

        GLuint heIdx = 1;

        GLMgroup *gp = m->groups;
        while(gp)
        {
            for(GLuint i = 0; i < gp->numtriangles; i++)
            {
                triangleIdx = gp->triangles[i];
                mt = &m->triangles[triangleIdx];

                hef = new HEFace();
                hef->id = triangleIdx;
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
                    he->id = heIdx;
                    heIdx++;

                    halfEdges.insert(make_pair(make_pair(u, v), he));
                    hes[j] = he;

                    if(!vertices.find(u)->second->heEdge)
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

        map<GLuint, HEFace*>::iterator ic = heFaces.begin();
        while(ic != heFaces.end())
        {
            if(!ic->second->heEdge->paired_edge->left_face
                    || !ic->second->heEdge->next_edge->paired_edge->left_face
                    || !ic->second->heEdge->next_edge->next_edge->paired_edge->left_face)
                ic->second->isBoundaryFace = true;
            else
                ic->second->isBoundaryFace = false;

            ic++;
        }

        nextNewIdCount = vertices.size() + 2;
    }

    void randomCollapse()
    {

        map< pair<GLuint, GLuint>, HalfEdge* >::iterator iter = halfEdges.begin();

        HEMetric mc;
        vector<HEMetric> cc;

        /*
        preprocess
        */
        map< pair<GLuint, GLuint>, HalfEdge* > ee;
        while(iter != halfEdges.end())
        {
            pair<GLuint, GLuint> pq = iter->first;
            pair<GLuint, GLuint> qp(pq.second, pq.first);

            if(ee.find(pq) == ee.end() && ee.find(qp) == ee.end())
                ee.insert(*iter);

            iter++;
        }

        iter = ee.begin();
        vector< pair<GLuint, GLuint> > delList; //store the halfedge with invalid end vertex
        while(iter != ee.end())
        {
            mc.he = iter->second;

            if(vertices.find(iter->first.first) != vertices.end() &&
                    vertices.find(iter->first.second) != vertices.end())
            {
                Point3D v1 =
                {
                    vertices[iter->first.first]->coordinate[0],
                    vertices[iter->first.first]->coordinate[1],
                    vertices[iter->first.first]->coordinate[2],
                };
                Point3D v2 =
                {
                    vertices[iter->first.second]->coordinate[0],
                    vertices[iter->first.second]->coordinate[1],
                    vertices[iter->first.second]->coordinate[2],
                };
                //mc.length = HEMetric::edgeDistance(vertices[iter->first.first]->coordinate, vertices[iter->first.second]->coordinate);
                mc.length = HEMetric::edgeDistance(v1, v2);
                cc.push_back(mc);
            }
            else
            {
                delList.push_back(iter->first);
            }

            iter++;
        }

        for(int i = 0; i < delList.size(); i++)
        {
            halfEdges.erase(delList[i]);
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

        //printf("collapse! %u, %u, length: %f\n", u, v, m->length);

        edgeCollapse(u, v);

        /*vector< pair<GLuint, GLuint> > delList; //store the halfedge with invalid end vertex
        while(iter != halfEdges.end())
        {
            mc.he = iter->second;
            if(iter->second->left_face && !iter->second->left_face->isBoundaryFace) //exclude the boundary edge
            {
                if(vertices.find(iter->first.first) != vertices.end() &&
                        vertices.find(iter->first.second) != vertices.end())
                {
                    Point3D v1 =
                    {
                        vertices[iter->first.first]->coordinate[0],
                        vertices[iter->first.first]->coordinate[1],
                        vertices[iter->first.first]->coordinate[2],
                    };
                    Point3D v2 =
                    {
                        vertices[iter->first.second]->coordinate[0],
                        vertices[iter->first.second]->coordinate[1],
                        vertices[iter->first.second]->coordinate[2],
                    };
                    //mc.length = HEMetric::edgeDistance(vertices[iter->first.first]->coordinate, vertices[iter->first.second]->coordinate);
                    mc.length = HEMetric::edgeDistance(v1, v2);
                    cc.push_back(mc);
                }
                else
                {
                    delList.push_back(iter->first);
                }
            }
            iter++;
        }

        for(int i = 0; i < delList.size(); i++)
        {
            halfEdges.erase(delList[i]);
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
        */
    }

    void edgeCollapse(GLuint u, GLuint v)
    {
        pair<GLuint, GLuint> uv(u, v);
        pair<GLuint, GLuint> vu(v, u);

        ////calculate new vertex position
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
        GLuint hv_mid_id = nextNewIdCount;
        nextNewIdCount++;

        hv_mid->id = hv_mid_id;
        hv_mid->heEdge = halfEdges[uv]->next_edge->next_edge->paired_edge;
        vertices.insert(make_pair(hv_mid_id, hv_mid));
        ////

        //will paring he11-he12, he21-he22

        HalfEdge *he11 = halfEdges[uv]->next_edge->paired_edge;
        HalfEdge *he12 = halfEdges[uv]->next_edge->next_edge->paired_edge;
        HalfEdge *he21 = halfEdges[vu]->next_edge->paired_edge;
        HalfEdge *he22 = halfEdges[vu]->next_edge->next_edge->paired_edge;

        HEFace *face1 = halfEdges[uv]->left_face;
        HEFace *face2 = halfEdges[vu]->left_face;

        vector< pair< pair<GLuint, GLuint>, HalfEdge* > > candidates;
        //start from u
        HalfEdge *he = halfEdges[uv]->paired_edge->next_edge;
        do
        {
            candidates.push_back(make_pair(make_pair(he->vertex_begin->id, he->paired_edge->vertex_begin->id), he));
            candidates.push_back(make_pair(make_pair(he->paired_edge->vertex_begin->id, he->vertex_begin->id), he->paired_edge));

            he = he->paired_edge->next_edge;
        }
        while(he != halfEdges[uv]);

        //start from v
        he = halfEdges[vu]->paired_edge->next_edge;
        do
        {
            candidates.push_back(make_pair(make_pair(he->vertex_begin->id, he->paired_edge->vertex_begin->id), he));
            candidates.push_back(make_pair(make_pair(he->paired_edge->vertex_begin->id, he->vertex_begin->id), he->paired_edge));

            he = he->paired_edge->next_edge;
        }
        while(he != halfEdges[vu]);

        halfEdges.erase(uv);
        halfEdges.erase(vu);
        //printf("erase uv: %d\n", halfEdges.erase(uv));
        //printf("erase vu: %d\n", halfEdges.erase(vu));

        for(int i = 0; i < candidates.size(); i++)
        {
            pair<GLuint, GLuint> &pq = candidates[i].first;

            halfEdges.erase(pq);
            //printf("erase candidates: %d\n", halfEdges.erase(pq));

            if(candidates[i].second->id == he12->paired_edge->id
                    || candidates[i].second->id == he21->paired_edge->id
                    || candidates[i].second->id == he11->paired_edge->id
                    || candidates[i].second->id == he22->paired_edge->id
              )
            {
                candidates[i].second = NULL;
                continue;
            }

            HalfEdge *he = candidates[i].second;

            if(pq.first == u || pq.first == v)
            {
                pq.first = hv_mid_id;
                he->vertex_begin = hv_mid;
            }
            else if(pq.second == u || pq.second == v)
            {
                pq.second = hv_mid_id;
            }
            else
            {
                printf("WTF?\n");
            }
        }

        //pareing outer halfedges
        he11->paired_edge = he12;
        he12->paired_edge = he11;
        he21->paired_edge = he22;
        he22->paired_edge = he21;

        //add what should be add
        for(int i = 0; i < candidates.size(); i++)
        {
            if(candidates[i].second)
                halfEdges.insert(candidates[i]);
        }

        face1->heEdge = NULL;
        face2->heEdge = NULL;
        heFaces.erase(face1->id);
        heFaces.erase(face2->id);
        //printf("erase face 1: %d\n", heFaces.erase(face1->id));
        //printf("erase face 2: %d\n", heFaces.erase(face2->id));

        vertices.erase(u);
        vertices.erase(v);
        //printf("erase vertices u: %d\n", vertices.erase(u));
        //printf("erase vertices v: %d\n",  vertices.erase(v));
    }
};

#endif