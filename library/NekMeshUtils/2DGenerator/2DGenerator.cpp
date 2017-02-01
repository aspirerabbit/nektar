////////////////////////////////////////////////////////////////////////////////
//
//  File: Generator2D.cpp
//
//  For more information, please see: http://www.nektar.info/
//
//  The MIT License
//
//  Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
//  Department of Aeronautics, Imperial College London (UK), and Scientific
//  Computing and Imaging Institute, University of Utah (USA).
//
//  License for the specific language governing rights and limitations under
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//
//  Description: 2D generator object methods.
//
////////////////////////////////////////////////////////////////////////////////
#include <algorithm>

#include <NekMeshUtils/2DGenerator/2DGenerator.h>

#include <LibUtilities/BasicUtils/ParseUtils.hpp>
#include <LibUtilities/BasicUtils/Progressbar.hpp>

using namespace std;
namespace Nektar
{
namespace NekMeshUtils
{

ModuleKey Generator2D::className = GetModuleFactory().RegisterCreatorFunction(
    ModuleKey(eProcessModule, "2dgenerator"), Generator2D::create,
    "Generates a 2D mesh");

Generator2D::Generator2D(MeshSharedPtr m) : ProcessModule(m)
{
    m_config["blcurves"] =
        ConfigOption(false, "0", "Generate parallelograms on these curves");
    m_config["blthick"] =
        ConfigOption(false, "0", "Parallelogram layer thickness");
}

Generator2D::~Generator2D()
{
}

void Generator2D::Process()
{
    if (m_mesh->m_verbose)
    {
        cout << endl << "2D meshing" << endl;
        cout << endl << "\tCurve meshing:" << endl << endl;
    }

    m_mesh->m_numNodes = m_mesh->m_cad->GetNumVerts();

    if(m_config["blcurves"].beenSet)
    {
        ParseUtils::GenerateSeqVector(m_config["blcurves"].as<string>().c_str(),
                                      m_blCurves);
        m_thickness = m_config["blthick"].as<NekDouble>();
    }

    // linear mesh all curves
    for (int i = 1; i <= m_mesh->m_cad->GetNumCurve(); i++)
    {
        if (m_mesh->m_verbose)
        {
            LibUtilities::PrintProgressbar(i, m_mesh->m_cad->GetNumCurve(),
                                           "Curve progress");
        }

        vector<unsigned int>::iterator f = find(m_blCurves.begin(),
                                                m_blCurves.end(), i);

        if(f == m_blCurves.end())
        {
            m_curvemeshes[i] =
                MemoryManager<CurveMesh>::AllocateSharedPtr(i, m_mesh);
        }
        else
        {
            m_curvemeshes[i] =
                MemoryManager<CurveMesh>::AllocateSharedPtr(i, m_mesh, m_thickness);
        }

        m_curvemeshes[i]->Mesh();
    }

    ////////////////////////////////////////

    EdgeSet::iterator it;
    for (it = m_mesh->m_edgeSet.begin(); it != m_mesh->m_edgeSet.end(); it++)
    {
        vector<NodeSharedPtr> ns;
        ns.push_back((*it)->m_n1);
        ns.push_back((*it)->m_n2);

        // for each iterator create a LibUtilities::eSegement
        // push segment into m_mesh->m_element[1]
        // tag for the elements shoudl be the CAD number of the curves

        ElmtConfig conf(LibUtilities::eSegment, 1, false, false);

        vector<int> tags;
        tags.push_back((*it)->m_parentCAD->GetId());

        ElementSharedPtr E2 = GetElementFactory().CreateInstance(
            LibUtilities::eSegment, conf, ns, tags);

        m_mesh->m_element[1].push_back(E2);
    }

    for (int i = 1; i <= m_mesh->m_cad->GetNumSurf(); i++)
    {
        m_facemeshes[i] = MemoryManager<FaceMesh>::AllocateSharedPtr(
            i, m_mesh, m_curvemeshes, 100);

        m_facemeshes[i]->OrientateCurves();
    }


    if (m_config["blcurves"].beenSet)
    {
        // we need to do the boundary layer generation in a face by face basis
        MakeBLPrep();

        // Im going to do a horrendous trick to get the edge orientaion.
        // Going to activate the first routine of facemeshing without actually
        // face meshing, this will orientate the edgeloop objects (hopefully);
        // which can be used by the makebl command to know the normal
        // orienation
        for (int i = 1; i <= m_mesh->m_cad->GetNumSurf(); i++)
        {
            MakeBL(i, m_facemeshes[i]->GetEdges());
        }
    }

    //m_mesh->m_element[1].clear();

    if (m_mesh->m_verbose)
    {
        cout << endl << "\tFace meshing:" << endl << endl;
    }

    // linear mesh all surfaces
    map<int,FaceMeshSharedPtr>::iterator fit;
    int i = 1;
    for(fit = m_facemeshes.begin(); fit != m_facemeshes.end(); fit++)
    {
        if (m_mesh->m_verbose)
        {
            LibUtilities::PrintProgressbar(
                i, m_mesh->m_cad->GetNumSurf(), "Face progress");
        }

        if (m_config["blcurves"].beenSet)
        {
            //for bl surfaces orientate curves needs to be run again to
            //push nodes to the edges of the system
            fit->second->ResetCurvemeshes(m_curvemeshes);
            fit->second->OrientateCurves();
        }

        fit->second->Mesh();
        i++;
    }

    ProcessVertices();
    ProcessEdges();
    ProcessFaces();
    ProcessElements();
    ProcessComposites();

    Report();
}

void Generator2D::MakeBLPrep()
{
    if (m_mesh->m_verbose)
    {
        cout << endl << "\tBoundary layer meshing:" << endl << endl;
    }

    for (vector<unsigned>::iterator it = m_blCurves.begin();
         it != m_blCurves.end(); ++it)
    {
        vector<EdgeSharedPtr> localedges = m_curvemeshes[*it]->GetMeshEdges();
        for (int i = 0; i < localedges.size(); i++)
        {
            m_nodesToEdge[localedges[i]->m_n1].push_back(localedges[i]);
            m_nodesToEdge[localedges[i]->m_n2].push_back(localedges[i]);
        }
    }
}

void Generator2D::MakeBL(int faceid, vector<EdgeLoopSharedPtr> e)
{
    map<int, int> edgeToOrient;
    for (int j = 0; j < e.size(); j++)
    {
        for (int i = 0; i < e[j]->edges.size(); ++i)
        {
            edgeToOrient[e[j]->edges[i]->GetId()] = e[j]->edgeo[i];
        }
    }

    map<int, Array<OneD, NekDouble> > edgeNormals;

    int eid = 0;

    for (vector<unsigned>::iterator it = m_blCurves.begin();
                    it != m_blCurves.end(); ++it)
    {
        int edgeo = edgeToOrient[*it];

        vector<EdgeSharedPtr> es = m_curvemeshes[*it]->GetMeshEdges();

        // on each !!!EDGE!!! calculate a normal
        // always to the left unless edgeo is 1
        for(int j = 0; j < es.size(); j++)
        {
            es[j]->m_id = eid++;
            Array<OneD, NekDouble> p1 = (edgeo == 0) ? es[j]->m_n1->GetLoc()
                                                     : es[j]->m_n2->GetLoc();
            Array<OneD, NekDouble> p2 = (edgeo == 0) ? es[j]->m_n2->GetLoc()
                                                     : es[j]->m_n1->GetLoc();
            Array<OneD, NekDouble> n(2);
            n[0] = p1[1] - p2[1];
            n[1] = p2[0] - p1[0];
            NekDouble mag = sqrt(n[0]*n[0]+n[1]*n[1]);
            n[0] /= mag;
            n[1] /= mag;
            edgeNormals[es[j]->m_id] = n;
        }
    }

    map<NodeSharedPtr, NodeSharedPtr> nodeNormals;
    map<NodeSharedPtr, vector<EdgeSharedPtr> >::iterator it;
    for(it = m_nodesToEdge.begin(); it != m_nodesToEdge.end(); it++)
    {
        Array<OneD, NekDouble> n(3);
        ASSERTL0(it->second.size() == 2,
                 "wierdness, most likely bl_surfs are incorrect");
        Array<OneD, NekDouble> n1 = edgeNormals[it->second[0]->m_id];
        Array<OneD, NekDouble> n2 = edgeNormals[it->second[1]->m_id];

        n[0] = (n1[0] + n2[0]) / 2.0;
        n[1] = (n1[1] + n2[1]) / 2.0;
        NekDouble mag = sqrt(n[0]*n[0]+n[1]*n[1]);
        n[0] /= mag;
        n[1] /= mag;

        n[0] = n[0] * m_thickness + it->first->m_x;
        n[1] = n[1] * m_thickness + it->first->m_y;
        n[2] = 0.0;

        NodeSharedPtr nn = boost::shared_ptr<Node>(
            new Node(m_mesh->m_numNodes++, n[0], n[1], 0.0));
        CADSurfSharedPtr s = m_mesh->m_cad->GetSurf(faceid);
        Array<OneD, NekDouble> uv = s->locuv(n);
        nn->SetCADSurf(faceid,s,uv);
        nodeNormals[it->first] = nn;
    }

    for (vector<unsigned>::iterator it = m_blCurves.begin();
                    it != m_blCurves.end(); ++it)
    {
        int edgeo = edgeToOrient[*it];

        vector<NodeSharedPtr> ns = m_curvemeshes[*it]->GetMeshPoints();
        vector<NodeSharedPtr> newNs;
        for(int i = 0; i < ns.size(); i++)
        {
            newNs.push_back(nodeNormals[ns[i]]);
        }
        m_curvemeshes[*it] = MemoryManager<CurveMesh>::AllocateSharedPtr(
                                        *it, m_mesh, newNs);

        if(edgeo == 1)
        {
            reverse(ns.begin(), ns.end());
        }
        for (int i = 0; i < ns.size() - 1; ++i)
        {
            vector<NodeSharedPtr> qns;

            qns.push_back(ns[i]);
            qns.push_back(ns[i+1]);
            qns.push_back(nodeNormals[ns[i+1]]);
            qns.push_back(nodeNormals[ns[i]]);

            ElmtConfig conf(LibUtilities::eQuadrilateral, 1, false, false);

            vector<int> tags;
            tags.push_back(101);

            ElementSharedPtr E = GetElementFactory().CreateInstance(
                LibUtilities::eQuadrilateral, conf, qns, tags);

            E->m_parentCAD = m_mesh->m_cad->GetSurf(faceid);

            for (int j = 0; j < E->GetEdgeCount(); ++j)
            {
                pair<EdgeSet::iterator,bool> testIns;
                EdgeSharedPtr ed = E->GetEdge(j);
                // look for edge in m_mesh edgeset from curves
                EdgeSet::iterator s = m_mesh->m_edgeSet.find(ed);
                if (!(s == m_mesh->m_edgeSet.end()))
                {
                    ed = *s;
                    E->SetEdge(j, *s);
                }
            }
            m_mesh->m_element[2].push_back(E);
        }
    }
}

void Generator2D::Report()
{
    if (m_mesh->m_verbose)
    {
        int ns = m_mesh->m_vertexSet.size();
        int es = m_mesh->m_edgeSet.size();
        int ts = m_mesh->m_element[2].size();
        int ep = ns - es + ts;
        cout << endl << "\tSurface mesh statistics" << endl;
        cout << "\t\tNodes: " << ns << endl;
        cout << "\t\tEdges: " << es << endl;
        cout << "\t\tTriangles " << ts << endl;
        cout << "\t\tEuler-Poincaré characteristic: " << ep << endl;
    }
}
}
}
