///////////////////////////////////////////////////////////////////////////////
//
// File IterativeElasticSystem.h
//
// For more information, please see: http://www.nektar.info
//
// The MIT License
//
// Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
// Department of Aeronautics, Imperial College London (UK), and Scientific
// Computing and Imaging Institute, University of Utah (USA).
//
// License for the specific language governing rights and limitations under
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Description: Linear elasticity equation system
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SOLVERS_LINEARELASTICSOLVER_EQUATIONSYSTEMS_ITERELASSYSTEM_H
#define NEKTAR_SOLVERS_LINEARELASTICSOLVER_EQUATIONSYSTEMS_ITERELASSYSTEM_H

#include <SolverUtils/EquationSystem.h>
#include <LinearElasticSolver/EquationSystems/LinearElasticSystem.h>

using namespace Nektar::SolverUtils;

namespace Nektar
{
    class IterativeElasticSystem : public LinearElasticSystem
    {
    public:
        /// Class may only be instantiated through the MemoryManager.
        friend class MemoryManager<IterativeElasticSystem>;

        /// Creates an instance of this class
        static EquationSystemSharedPtr create(
                const LibUtilities::SessionReaderSharedPtr& pSession)
        {
            EquationSystemSharedPtr p = MemoryManager<
                IterativeElasticSystem>::AllocateSharedPtr(pSession);
            p->InitObject();
            return p;
        }

        /// Name of class
        static std::string className;

    protected:
        int m_numSteps;
        bool m_repeatBCs;

        Array<OneD, Array<OneD, Array<OneD, NekDouble> > > m_savedBCs;
        vector<int> m_toDeform;

        IterativeElasticSystem(
            const LibUtilities::SessionReaderSharedPtr& pSession);

        virtual void v_InitObject();
        virtual void v_GenerateSummary(SolverUtils::SummaryList& s);
        virtual void v_DoSolve();

        void WriteGeometry(const int i);
    };
}

#endif