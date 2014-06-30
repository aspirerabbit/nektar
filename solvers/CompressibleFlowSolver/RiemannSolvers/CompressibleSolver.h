///////////////////////////////////////////////////////////////////////////////
//
// File: CompressibleSolver.h
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
// Description: Compressible Riemann solver.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SOLVERS_COMPRESSIBLEFLOWSOLVER_RIEMANNSOLVER_COMPRESSIBLESOLVER
#define NEKTAR_SOLVERS_COMPRESSIBLEFLOWSOLVER_RIEMANNSOLVER_COMPRESSIBLESOLVER

#include <SolverUtils/RiemannSolvers/RiemannSolver.h>

using namespace Nektar::SolverUtils;

namespace Nektar
{
    class CompressibleSolver : public RiemannSolver
    {
    protected:
        bool m_pointSolve;
        
        CompressibleSolver();
        
        virtual void v_Solve(
            const Array<OneD, const Array<OneD, NekDouble> > &Fwd,
            const Array<OneD, const Array<OneD, NekDouble> > &Bwd,
                  Array<OneD,       Array<OneD, NekDouble> > &flux);
        
        virtual void  v_AdjointSolve(
            const Array<OneD, const Array<OneD, NekDouble> > &Fwd,
            const Array<OneD, const Array<OneD, NekDouble> > &Bwd,
            const Array<OneD, const Array<OneD, NekDouble> > &FwdDir,
            const Array<OneD, const Array<OneD, NekDouble> > &BwdDir,
                  Array<OneD,       Array<OneD, NekDouble> > &flux);
        
        virtual void  v_AdjointNSSolve(
            const Array<OneD, const Array<OneD, NekDouble> > &Fwd,
            const Array<OneD, const Array<OneD, NekDouble> > &Bwd,
            const Array<OneD, const Array<OneD, NekDouble> > &FwdDir,
            const Array<OneD, const Array<OneD, NekDouble> > &BwdDir,
            Array<OneD, Array<OneD, Array<OneD, NekDouble > > > &FwdDirDIFF,
            Array<OneD, Array<OneD, Array<OneD, NekDouble > > > &BwdDirDIFF,
                  Array<OneD,       Array<OneD, NekDouble> > &flux);
        
        
        virtual void v_ArraySolve(
            const Array<OneD, const Array<OneD, NekDouble> > &Fwd,
            const Array<OneD, const Array<OneD, NekDouble> > &Bwd,
                  Array<OneD,       Array<OneD, NekDouble> > &flux)
        {
            ASSERTL0(false, "This function should be defined by subclasses.");
        }

        virtual void v_PointSolve(
            NekDouble  rhoL, NekDouble  rhouL, NekDouble  rhovL, NekDouble  rhowL, NekDouble  EL,
            NekDouble  rhoR, NekDouble  rhouR, NekDouble  rhovR, NekDouble  rhowR, NekDouble  ER,
            NekDouble &rhof, NekDouble &rhouf, NekDouble &rhovf, NekDouble &rhowf, NekDouble &Ef)
        {
            ASSERTL0(false, "This function should be defined by subclasses.");
        }
        
        virtual void v_ArrayAdjointSolve(
            const Array<OneD, const Array<OneD, NekDouble> > &Fwd,
            const Array<OneD, const Array<OneD, NekDouble> > &Bwd,
            const Array<OneD, const Array<OneD, NekDouble> > &FwdDir,
            const Array<OneD, const Array<OneD, NekDouble> > &BwdDir,
                  Array<OneD,       Array<OneD, NekDouble> > &flux)
        {
            ASSERTL0(false, "This function should be defined by subclasses.");
        }
        
        virtual void v_ArrayAdjointNSSolve(
            const Array<OneD, const Array<OneD, NekDouble> > &Fwd,
            const Array<OneD, const Array<OneD, NekDouble> > &Bwd,
            const Array<OneD, const Array<OneD, NekDouble> > &FwdDir,
            const Array<OneD, const Array<OneD, NekDouble> > &BwdDir,
            Array<OneD, Array<OneD, Array<OneD, NekDouble > > > &FwdDirDiff,
            Array<OneD, Array<OneD, Array<OneD, NekDouble > > > &BwdDirDiff,
                 Array<OneD,       Array<OneD, NekDouble> > &flux)
        {
            ASSERTL0(false, "This function should be defined by subclasses.");
        }
        
        virtual void v_PointAdjointSolve(
            NekDouble  rhoL, NekDouble  rhouL, NekDouble  rhovL, NekDouble  rhowL, NekDouble  EL,
            NekDouble  rhoR, NekDouble  rhouR, NekDouble  rhovR, NekDouble  rhowR, NekDouble  ER,
            NekDouble  rhoLDir, NekDouble  rhouLDir, NekDouble  rhovLDir, NekDouble  rhowLDir, NekDouble  ELDir,
            NekDouble  rhoRDir, NekDouble  rhouRDir, NekDouble  rhovRDir, NekDouble  rhowRDir, NekDouble  ERDir,
            NekDouble &rhof, NekDouble &rhouf, NekDouble &rhovf, NekDouble &rhowf, NekDouble &Ef)
        {
            ASSERTL0(false, "This function should be defined by subclasses.");
        }
        
        virtual void v_PointAdjointNSSolve(
            NekDouble  rhoL, NekDouble  rhouL, NekDouble  rhovL, NekDouble  rhowL, NekDouble  EL,
            NekDouble  rhoR, NekDouble  rhouR, NekDouble  rhovR, NekDouble  rhowR, NekDouble  ER,
            NekDouble  rhoLDir, NekDouble  rhouLDir, NekDouble  rhovLDir, NekDouble  rhowLDir, NekDouble  ELDir,
            NekDouble  rhoRDir, NekDouble  rhouRDir, NekDouble  rhovRDir, NekDouble  rhowRDir, NekDouble  ERDir,
            NekDouble  DrhoLDirDX, NekDouble  DrhouLDirDX, NekDouble  DrhovLDirDX, NekDouble  DrhowLDirDX, NekDouble  DELDirDX,
            NekDouble  DrhoRDirDX, NekDouble  DrhouRDirDX, NekDouble  DrhovRDirDX, NekDouble  DrhowRDirDX, NekDouble  DERDirDX,
            NekDouble  DrhoLDirDY, NekDouble  DrhouLDirDY, NekDouble  DrhovLDirDY, NekDouble  DrhowLDirDY, NekDouble  DELDirDY,
            NekDouble  DrhoRDirDY, NekDouble  DrhouRDirDY, NekDouble  DrhovRDirDY, NekDouble  DrhowRDirDY, NekDouble  DERDirDY,
            NekDouble  DrhoLDirDZ, NekDouble  DrhouLDirDZ, NekDouble  DrhovLDirDZ, NekDouble  DrhowLDirDZ, NekDouble  DELDirDZ,
            NekDouble  DrhoRDirDZ, NekDouble  DrhouRDirDZ, NekDouble  DrhovRDirDZ, NekDouble  DrhowRDirDZ, NekDouble  DERDirDZ,
            NekDouble &rhof, NekDouble &rhouf, NekDouble &rhovf, NekDouble &rhowf, NekDouble &Ef)
        {
            ASSERTL0(false, "This function should be defined by subclasses.");
        }
    };
}

#endif
