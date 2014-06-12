////////////////////////////////////////////////////////////////////////////////
//
//  File: ProcessDisplacement.cpp
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
//  Description: Computes Q Criterion field.
//
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
using namespace std;

#include "ProcessDisplacement.h"

#include <StdRegions/StdSegExp.h>
#include <StdRegions/StdQuadExp.h>
#include <StdRegions/StdTriExp.h>
#include <LocalRegions/SegExp.h>
#include <LocalRegions/TriExp.h>
#include <LibUtilities/BasicUtils/SharedArray.hpp>
#include <LibUtilities/BasicUtils/ParseUtils.hpp>

namespace Nektar
{
    namespace Utilities
    {
        ModuleKey ProcessDisplacement::className =
        GetModuleFactory().RegisterCreatorFunction(
            ModuleKey(eProcessModule, "displacement"), ProcessDisplacement::create,
            "Deform a mesh given an input field defining displacement");

        ProcessDisplacement::ProcessDisplacement(FieldSharedPtr f) :
            ProcessModule(f)
        {
            m_config["to"] = ConfigOption(
                false, "", "Name of file containing high order boundary");
            m_config["id"] = ConfigOption(
                false, "", "Boundary ID to calculate displacement for");
            f->m_declareExpansionAsContField = true;
            f->m_writeBndFld = true;
        }

        ProcessDisplacement::~ProcessDisplacement()
        {
        }

        void ProcessDisplacement::Process(po::variables_map &vm)
        {
            if (m_f->m_verbose)
            {
                cout << "ProcessDisplacement: Calculating displacement..."
                     << endl;
            }

            string toFile = m_config["to"].as<string>();

            if (toFile == "")
            {
                cout << "ProcessDisplacement: you must provide a file" << endl;
                return;
            }

            vector<string> files;
            files.push_back(toFile);
            LibUtilities::SessionReaderSharedPtr bndSession =
                LibUtilities::SessionReader::CreateInstance(0, NULL, files);
            SpatialDomains::MeshGraphSharedPtr bndGraph =
                SpatialDomains::MeshGraph::Read(bndSession);

            // Try to find boundary condition expansion.
            int bndCondId = m_config["id"].as<int>();
            // FIXME: We should be storing boundary condition IDs
            // somewhere...
            m_f->m_bndRegionsToWrite.push_back(bndCondId);

            if (bndGraph->GetMeshDimension() == 1)
            {
                m_f->m_exp.push_back(m_f->AppendExpList(0, "v"));

                MultiRegions::ExpListSharedPtr bndCondExpU =
                    m_f->m_exp[0]->GetBndCondExpansions()[bndCondId];
                MultiRegions::ExpListSharedPtr bndCondExpV =
                    m_f->m_exp[1]->GetBndCondExpansions()[bndCondId];

                map<int, int> bndCondIds;
                for (int i = 0; i < bndCondExpU->GetExpSize(); ++i)
                {
                    bndCondIds[bndCondExpU->GetExp(i)->GetGeom()->GetGlobalID()]
                        = i;
                }

                const SpatialDomains::SegGeomMap &tmp =
                    bndGraph->GetAllSegGeoms();
                SpatialDomains::SegGeomMap::const_iterator sIt;

                for (sIt = tmp.begin(); sIt != tmp.end(); ++sIt)
                {
                    map<int, int>::iterator mIt = bndCondIds.find(sIt->first);

                    if (mIt == bndCondIds.end())
                    {
                        cout << "Warning: couldn't find element "
                             << sIt->first << endl;
                        continue;
                    }

                    int e = mIt->second;

                    SpatialDomains::SegGeomSharedPtr from =
                        boost::dynamic_pointer_cast<SpatialDomains::SegGeom>(
                            bndCondExpU->GetExp(e)->GetGeom());

                    SpatialDomains::SegGeomSharedPtr to = sIt->second;

                    // Create temporary SegExp
                    LocalRegions::SegExpSharedPtr toSeg = MemoryManager<
                        LocalRegions::SegExp>::AllocateSharedPtr(
                            bndCondExpU->GetExp(e)->GetBasis(0)->GetBasisKey(),
                            to);

                    const int offset = bndCondExpU->GetPhys_Offset(e);
                    const int nq     = toSeg->GetTotPoints();

                    Array<OneD, NekDouble> xL(nq), xC(nq), yL(nq), yC(nq), tmp;

                    bndCondExpU->GetExp(e)->GetCoords(xC, yC);
                    toSeg->GetCoords(xL, yL);

                    Vmath::Vsub(nq, xL, 1, xC, 1, tmp = bndCondExpU->UpdatePhys() + offset, 1);
                    Vmath::Vsub(nq, yL, 1, yC, 1, tmp = bndCondExpV->UpdatePhys() + offset, 1);
                }

                // bndconstrained?
                bndCondExpU->FwdTrans_BndConstrained(
                    bndCondExpU->GetPhys(), bndCondExpU->UpdateCoeffs());
                bndCondExpV->FwdTrans_BndConstrained(
                    bndCondExpV->GetPhys(), bndCondExpV->UpdateCoeffs());
            }
            else if (bndGraph->GetMeshDimension() == 2)
            {
                m_f->m_exp.push_back(m_f->AppendExpList(0, "v"));
                m_f->m_exp.push_back(m_f->AppendExpList(0, "w"));

                MultiRegions::ExpListSharedPtr bndCondExpU =
                    m_f->m_exp[0]->GetBndCondExpansions()[bndCondId];
                MultiRegions::ExpListSharedPtr bndCondExpV =
                    m_f->m_exp[1]->GetBndCondExpansions()[bndCondId];
                MultiRegions::ExpListSharedPtr bndCondExpW =
                    m_f->m_exp[2]->GetBndCondExpansions()[bndCondId];

                map<int, int> bndCondIds;
                for (int i = 0; i < bndCondExpU->GetExpSize(); ++i)
                {
                    bndCondIds[bndCondExpU->GetExp(i)->GetGeom()->GetGlobalID()]
                        = i;
                }

                const SpatialDomains::TriGeomMap &tmp =
                    bndGraph->GetAllTriGeoms();
                SpatialDomains::TriGeomMap::const_iterator sIt;

                for (sIt = tmp.begin(); sIt != tmp.end(); ++sIt)
                {
                    map<int, int>::iterator mIt = bndCondIds.find(sIt->first);

                    if (mIt == bndCondIds.end())
                    {
                        cout << "Warning: couldn't find element "
                             << sIt->first << endl;
                        continue;
                    }

                    int e = mIt->second;

                    SpatialDomains::TriGeomSharedPtr from =
                        boost::dynamic_pointer_cast<SpatialDomains::TriGeom>(
                            bndCondExpU->GetExp(e)->GetGeom());

                    SpatialDomains::TriGeomSharedPtr to = sIt->second;

                    // Create temporary SegExp
                    LocalRegions::TriExpSharedPtr toSeg = MemoryManager<
                        LocalRegions::TriExp>::AllocateSharedPtr(
                            bndCondExpU->GetExp(e)->GetBasis(0)->GetBasisKey(),
                            bndCondExpV->GetExp(e)->GetBasis(1)->GetBasisKey(),
                            to);

                    const int offset = bndCondExpU->GetPhys_Offset(e);
                    const int nq     = toSeg->GetTotPoints();

                    Array<OneD, NekDouble> xL(nq), xC(nq), yL(nq), yC(nq), tmp;
                    Array<OneD, NekDouble> zL(nq), zC(nq);

                    bndCondExpU->GetExp(e)->GetCoords(xC, yC, zC);
                    toSeg->GetCoords(xL, yL, zL);

                    Vmath::Vsub(nq, xL, 1, xC, 1, tmp = bndCondExpU->UpdatePhys() + offset, 1);
                    Vmath::Vsub(nq, yL, 1, yC, 1, tmp = bndCondExpV->UpdatePhys() + offset, 1);
                    Vmath::Vsub(nq, zL, 1, zC, 1, tmp = bndCondExpW->UpdatePhys() + offset, 1);
                }

                // bndconstrained?
                bndCondExpU->FwdTrans_BndConstrained(
                    bndCondExpU->GetPhys(), bndCondExpU->UpdateCoeffs());
                bndCondExpV->FwdTrans_BndConstrained(
                    bndCondExpV->GetPhys(), bndCondExpV->UpdateCoeffs());
                bndCondExpW->FwdTrans_BndConstrained(
                    bndCondExpW->GetPhys(), bndCondExpW->UpdateCoeffs());
            }
        }
    }
}
