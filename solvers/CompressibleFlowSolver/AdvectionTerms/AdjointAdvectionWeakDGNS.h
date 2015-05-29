///////////////////////////////////////////////////////////////////////////////
//
// File AdjointAdvectionWeakDGNS.h
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
// Description: TBA
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SOLVERS_AdjointAdvectionWeakDGNS_H
#define NEKTAR_SOLVERS_AdjointAdvectionWeakDGNS_H
#include <SolverUtils/Advection/Advection.h>
#include <LibUtilities/FFT/NektarFFT.h>

namespace Nektar
{


/// Advection for the adjoint form of the linearised Navier-Stokes equations
class AdjointAdvectionWeakDGNS: public SolverUtils::Advection
{
        enum FloquetMatType
        {
            eForwardsCoeff,
            eForwardsPhys
        };

        /// A map between  matrix keys and their associated block matrices.
        typedef map< FloquetMatType, DNekBlkMatSharedPtr> FloquetBlockMatrixMap;
        /// A shared pointer to a BlockMatrixMap.
        typedef boost::shared_ptr<FloquetBlockMatrixMap> FloquetBlockMatrixMapShPtr;

    public:
        friend class MemoryManager<AdjointAdvectionWeakDGNS>;

        /// Creates an instance of this class
        static SolverUtils::AdvectionSharedPtr create(std::string) {
            return MemoryManager<AdjointAdvectionWeakDGNS>::AllocateSharedPtr();
        }

        static std::string type[];
    
        std::string m_advType;

        /// Name of class
        static std::string className;

    protected:
        LibUtilities::SessionReaderSharedPtr m_session;

        MultiRegions::ProjectionType m_projectionType;
        int m_spacedim;
        int m_expdim;

        /// Storage for base flow
        Array<OneD, Array<OneD, NekDouble> >            m_baseflow;
        Array<OneD, MultiRegions::ExpListSharedPtr>     m_basefields;
        //number of slices
        int                                             m_slices;
        //period length
        NekDouble                                       m_chksp;
        NekDouble                                       m_chkf;
        //interpolation vector
        Array<OneD, Array<OneD, NekDouble> >            m_interp;
        Array<OneD, Array<OneD, NekDouble> >            m_traceNormals;
        //auxiliary variables
        Array<OneD,NekDouble>                           m_tmpIN;
        Array<OneD,NekDouble>                           m_tmpOUT;
        bool                                            m_useFFTW;

        /// flag to determine if use single mode or not
        bool                                            m_SingleMode;
        /// flag to determine if use half mode or not
        bool                                            m_HalfMode;
        /// flag to determine if use multiple mode or not
        bool                                            m_MultipleModes;
        bool                                            m_homogen_dealiasing;
        MultiRegions::CoeffState                        m_CoeffState;
        AdjointAdvectionWeakDGNS();

        virtual ~AdjointAdvectionWeakDGNS();

        virtual void v_InitObject(
            LibUtilities::SessionReaderSharedPtr        pSession,
            Array<OneD, MultiRegions::ExpListSharedPtr> pFields);

        virtual void v_Advect(
            const int nConvectiveFields,
            const Array<OneD, MultiRegions::ExpListSharedPtr> &fields,
            const Array<OneD, Array<OneD, NekDouble> >        &advVel,
            const Array<OneD, Array<OneD, NekDouble> >        &inarray,
                  Array<OneD, Array<OneD, NekDouble> >        &outarray,
            const NekDouble                                   &time);

        virtual void v_SetBaseFlow(
                const Array<OneD, Array<OneD, NekDouble> >    &inarray);
    
        virtual void v_ImportFldBase(std::string pInfile,
                     Array<OneD, MultiRegions::ExpListSharedPtr>& pFields);
    
        virtual void v_GetBaseFlow(
            Array<OneD, Array<OneD, NekDouble> > &baseflow);
    
        //Initialise m_base in order to store the base flow from a file
        SOLVER_UTILS_EXPORT void SetUpBaseFields(SpatialDomains::MeshGraphSharedPtr &mesh);
    
        private:
        ///Parameter for homogeneous expansions
        enum HomogeneousType
        {
            eHomogeneous1D,
            eHomogeneous2D,
            eHomogeneous3D,
            eNotHomogeneous
        };

        /// flag to determine if use or not the FFT for transformations
        bool                                            m_useFFT;

        enum HomogeneousType                            m_HomogeneousType;

        NekDouble m_LhomX; ///< physical length in X direction (if homogeneous)
        NekDouble m_LhomY; ///< physical length in Y direction (if homogeneous)
        NekDouble m_LhomZ; ///< physical length in Z direction (if homogeneous)

        int m_npointsX;    ///< number of points in X direction (if homogeneous)
        int m_npointsY;    ///< number of points in Y direction (if homogeneous)
        int m_npointsZ;    ///< number of points in Z direction (if homogeneous)

        int m_HomoDirec;   ///< number of homogenous directions

        int m_NumMode;     ///< Mode to use in case of single mode analysis

        SpatialDomains::BoundaryConditionsSharedPtr m_boundaryConditions;
};

}

#endif //NEKTAR_SOLVERS_INCNAVIERSTOKES_H