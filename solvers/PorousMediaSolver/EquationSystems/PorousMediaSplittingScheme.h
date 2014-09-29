///////////////////////////////////////////////////////////////////////////////
//
// File PorousMediaSplittingScheme.h
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
// Description: PorousMedia solver routine 
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SOLVERS_POROUSMEDIASPLITTINGSCHEME_H
#define NEKTAR_SOLVERS_POROUSMEDIASPLITTINGSCHEME_H

#include <PorousMediaSolver/EquationSystems/PorousMedia.h>

namespace Nektar
{
    class PorousMediaSplittingScheme: public PorousMedia
    {
    public:

        /// Creates an instance of this class
        static SolverUtils::EquationSystemSharedPtr create(
                const LibUtilities::SessionReaderSharedPtr& pSession) {
            SolverUtils::EquationSystemSharedPtr p =
                                MemoryManager<PorousMediaSplittingScheme>::
                                            AllocateSharedPtr(pSession);
            p->InitObject();
            return p;
        }

        /// Name of class
        static std::string className;


        /// Constructor.
        PorousMediaSplittingScheme(const LibUtilities::SessionReaderSharedPtr& pSession);

        virtual ~PorousMediaSplittingScheme();

        virtual void v_InitObject();

        void SetUpPressureForcing(const Array<OneD, const Array<OneD, NekDouble> > &fields,
								  Array<OneD, Array<OneD, NekDouble> > &Forcing,
								  const NekDouble aii_Dt);

        void SetUpViscousForcing(const Array<OneD, const Array<OneD, NekDouble> > &inarray,
								 Array<OneD, Array<OneD, NekDouble> > &Forcing,
								 const NekDouble aii_Dt);

        void SolveUnsteadyStokesSystem(const Array<OneD, const Array<OneD, NekDouble> > &inarray,
									   Array<OneD, Array<OneD, NekDouble> > &outarray,
									   const NekDouble time,
									   const NekDouble a_iixDt);

        void EvaluateAdvection_SetPressureBCs(const Array<OneD, const Array<OneD, NekDouble> > &inarray,
											  Array<OneD, Array<OneD, NekDouble> > &outarray,
											  const NekDouble time);

    protected:

    private:
        // Virtual functions
        virtual void v_GenerateSummary(SolverUtils::SummaryList& s);

        virtual void v_TransCoeffToPhys(void);

        virtual void v_TransPhysToCoeff(void);

        virtual void v_DoInitialise(void);

        virtual Array<OneD, bool> v_GetSystemSingularChecks();

        virtual int v_GetForceDimension();
    };

    typedef boost::shared_ptr<PorousMediaSplittingScheme>
                PorousMediaSplittingSchemeSharedPtr;

} //end of namespace


#endif //VELOCITY_CORRECTION_SCHEME_H
