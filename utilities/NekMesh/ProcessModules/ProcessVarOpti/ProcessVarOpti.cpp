////////////////////////////////////////////////////////////////////////////////
//
//  File: ProcessJac.cpp
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
//  Description: Calculate Jacobians of elements.
//
////////////////////////////////////////////////////////////////////////////////

#include <LibUtilities/Foundations/ManagerAccess.h>

#include <NekMeshUtils/MeshElements/Element.h>
#include "ProcessVarOpti.h"
#include "NodeOpti.h"
#include "ElUtil.h"

#include <boost/thread/mutex.hpp>

#include <StdRegions/StdTriExp.h>
#include <StdRegions/StdQuadExp.h>
#include <StdRegions/StdTetExp.h>
#include <StdRegions/StdPrismExp.h>

#include <LibUtilities/BasicUtils/Timer.h>
#include <LibUtilities/Foundations/NodalUtil.h>

using namespace std;
using namespace Nektar::NekMeshUtils;

namespace Nektar
{
namespace Utilities
{

ModuleKey ProcessVarOpti::className = GetModuleFactory().RegisterCreatorFunction(
    ModuleKey(eProcessModule, "varopti"),
    ProcessVarOpti::create,
    "Optimise mesh locations.");

ProcessVarOpti::ProcessVarOpti(MeshSharedPtr m) : ProcessModule(m)
{
    m_config["linearelastic"] =
        ConfigOption(true, "", "Optimise for linear elasticity");
    m_config["winslow"] =
        ConfigOption(true, "", "Optimise for winslow");
    m_config["roca"] =
        ConfigOption(true, "", "Optimise for roca method");
    m_config["hyperelastic"] =
        ConfigOption(true, "", "Optimise for hyper elasticity");
    m_config["numthreads"] =
        ConfigOption(false, "1", "Number of threads");
    m_config["restol"] =
        ConfigOption(false, "1e-6", "Tolerance criterion");
    m_config["maxiter"] =
        ConfigOption(false, "500", "Maximum number of iterations");
    m_config["nq"] =
        ConfigOption(false, "-1", "Order of mesh");
    m_config["region"] =
        ConfigOption(false, "0.0", "create regions based on target");
    m_config["resfile"] =
        ConfigOption(false, "", "writes residual values to file");
    m_config["histfile"] =
        ConfigOption(false, "", "histogram of scaled jac");
}

ProcessVarOpti::~ProcessVarOpti()
{
}

void ProcessVarOpti::Process()
{
    if (m_mesh->m_verbose)
    {
        cout << "ProcessVarOpti: Optimising... " << endl;
    }

    if(m_config["linearelastic"].beenSet)
    {
        opti = eLinEl;
    }
    else if(m_config["winslow"].beenSet)
    {
        opti = eWins;
    }
    else if(m_config["roca"].beenSet)
    {
        opti = eRoca;
    }
    else if(m_config["hyperelastic"].beenSet)
    {
        opti = eHypEl;
    }
    else
    {
        ASSERTL0(false,"not opti type set");
    }

    const int maxIter = m_config["maxiter"].as<int>();
    const NekDouble restol = m_config["restol"].as<NekDouble>();

    //m_mesh->m_nummode = m_config["nq"].as<int>();

    EdgeSet::iterator eit;
    m_mesh->m_nummode = -1;
    if (m_mesh->m_nummode == -1)
    {
        bool fd = false;

        if(m_config["nq"].beenSet)
        {
            m_mesh->m_nummode = m_config["nq"].as<int>();
            fd = true;
        }

        if(!fd)
        {
            for(eit = m_mesh->m_edgeSet.begin(); eit != m_mesh->m_edgeSet.end(); eit++)
            {
                if((*eit)->m_edgeNodes.size() > 0)
                {
                    m_mesh->m_nummode = (*eit)->m_edgeNodes.size() + 2;
                    fd = true;
                    break;
                }
            }
        }

        ASSERTL0(fd,"failed to find order of mesh");
    }

    if(m_mesh->m_verbose)
    {
        cout << "Identified mesh order as: " << m_mesh->m_nummode - 1 << endl;
    }

    if (m_mesh->m_expDim == 2 && m_mesh->m_spaceDim == 3)
    {
        ASSERTL0(false,"cannot deal with manifolds");
    }

    res = boost::shared_ptr<Residual>(new Residual);
    res->val = 1.0;

    m_mesh->MakeOrder(m_mesh->m_nummode-1,LibUtilities::eGaussLobattoLegendre);
    BuildDerivUtil();
    GetElementMap();

    vector<ElementSharedPtr> elLock;

    if(m_config["region"].beenSet)
    {
        elLock = GetLockedElements(m_config["region"].as<NekDouble>());
    }

    vector<vector<NodeSharedPtr> > freenodes = GetColouredNodes(elLock);
    vector<vector<NodeOptiSharedPtr> > optiNodes;

    //turn the free nodes into optimisable objects with all required data
    for(int i = 0; i < freenodes.size(); i++)
    {
        vector<NodeOptiSharedPtr> ns;
        for(int j = 0; j < freenodes[i].size(); j++)
        {
            NodeElMap::iterator it = nodeElMap.find(freenodes[i][j]->m_id);
            ASSERTL0(it != nodeElMap.end(), "could not find");

            int optiType = m_mesh->m_spaceDim;

            if (freenodes[i][j]->GetNumCadCurve() == 1)
            {
                optiType += 10;
            }
            else if (freenodes[i][j]->GetNumCADSurf() == 1)
            {
                optiType += 20;
            }
            else
            {
                optiType += 10*m_mesh->m_expDim;
            }

            ns.push_back(
                GetNodeOptiFactory().CreateInstance(
                    optiType, freenodes[i][j], it->second, res, derivUtil, opti));

            if(freenodes[i][j]->m_id < m_mesh->m_vertexSet.size())
            {
                ns.back()->SetVertex();
            }
        }
        optiNodes.push_back(ns);
    }

    int nset = optiNodes.size();
    int p = 0;
    int mn = numeric_limits<int>::max();
    int mx = 0;
    for(int i = 0; i < nset; i++)
    {
        p += optiNodes[i].size();
        mn = min(mn, int(optiNodes[i].size()));
        mx = max(mx, int(optiNodes[i].size()));
    }

    res->startInv =0;
    res->worstJac = numeric_limits<double>::max();
    for(int i = 0; i < dataSet.size(); i++)
    {
        dataSet[i]->Evaluate();
    }

    if(m_config["histfile"].beenSet)
    {
        ofstream histFile;
        string name = m_config["histfile"].as<string>() + "_start.txt";
        histFile.open(name.c_str());

        for(int i = 0; i < dataSet.size(); i++)
        {
            histFile << dataSet[i]->scaledJac << endl;
        }
        histFile.close();
    }

    cout << scientific << endl;
    cout << "N elements:\t\t" << m_mesh->m_element[m_mesh->m_expDim].size() - elLock.size() << endl
         << "N elements invalid:\t" << res->startInv << endl
         << "Worst jacobian:\t\t" << res->worstJac << endl
         << "N free nodes:\t\t" << res->n << endl
         << "N Dof:\t\t\t" << res->nDoF << endl
         << "N color sets:\t\t" << nset << endl
         << "Avg set colors:\t\t" << p/nset << endl
         << "Min set:\t\t" << mn << endl
         << "Max set:\t\t" << mx << endl
         << "Residual tolerance:\t" << restol << endl;

    //return;

    int nThreads = m_config["numthreads"].as<int>();

    int ctr = 0;
    Thread::ThreadMaster tms;
    tms.SetThreadingType("ThreadManagerBoost");
    Thread::ThreadManagerSharedPtr tm =
                tms.CreateInstance(Thread::ThreadMaster::SessionJob, nThreads);

    Timer t;
    t.Start();

    ofstream resFile;
    if(m_config["resfile"].beenSet)
    {
        resFile.open(m_config["resfile"].as<string>().c_str());
    }

    while (res->val > restol)
    {
        ctr++;
        res->val = 0.0;
        res->func = 0.0;
        res->nReset = 0;
        for(int i = 0; i < optiNodes.size(); i++)
        {
            vector<Thread::ThreadJob*> jobs(optiNodes[i].size());
            for(int j = 0; j < optiNodes[i].size(); j++)
            {
                jobs[j] = optiNodes[i][j]->GetJob();
            }

            tm->SetNumWorkers(0);
            tm->QueueJobs(jobs);
            tm->SetNumWorkers(nThreads);
            tm->Wait();
            //for(int j = 0; j < jobs.size(); j++)
            //{
            //    jobs[j]->Run();
            //}
        }

        res->startInv = 0;
        res->worstJac = numeric_limits<double>::max();

        vector<Thread::ThreadJob*> elJobs(dataSet.size());
        for(int i = 0; i < dataSet.size(); i++)
        {
            elJobs[i] = dataSet[i]->GetJob();
        }

        tm->SetNumWorkers(0);
        tm->QueueJobs(elJobs);
        tm->SetNumWorkers(nThreads);
        tm->Wait();

        if(m_config["resfile"].beenSet)
        {
            resFile << res->val << " " << res->worstJac << " " << res->func << endl;
        }

        cout << ctr << "\tResidual: " << res->val
                    << "\tMin Jac: " << res->worstJac
                    << "\tInvalid: " << res->startInv
                    << "\tReset nodes: " << res->nReset
                    << "\tFunctional: " << res->func
                    << endl;
        if(ctr >= maxIter)
            break;
    }

    if(m_config["histfile"].beenSet)
    {
        ofstream histFile;
        string name = m_config["histfile"].as<string>() + "_end.txt";
        histFile.open(name.c_str());

        for(int i = 0; i < dataSet.size(); i++)
        {
            histFile << dataSet[i]->scaledJac << endl;
        }
        histFile.close();
    }
    if(m_config["resfile"].beenSet)
    {
        resFile.close();
    }

    t.Stop();
    cout << "Time to compute: " << t.TimePerTest(1) << endl;

    cout << "Invalid at end:\t\t" << res->startInv << endl;
    cout << "Worst at end:\t\t" << res->worstJac << endl;
}

}
}
