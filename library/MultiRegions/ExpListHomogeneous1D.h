///////////////////////////////////////////////////////////////////////////////
//
// File ExpListHomogeneous1D.h
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
// Description: Base class for expansions which are homogeneous in 1
// direction
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EXPLISTHOMO1D_H
#define EXPLISTHOMO1D_H
#include <MultiRegions/MultiRegionsDeclspec.h>
#include <vector>
#include <MultiRegions/MultiRegions.hpp>
#include <MultiRegions/ExpList.h>
#include <LibUtilities/FFT/NektarFFT.h>

namespace Nektar
{
    namespace MultiRegions
    {

        enum Homogeneous1DMatType
        {
            eForwardsCoeffSpace1D,
            eBackwardsCoeffSpace1D,
            eForwardsPhysSpace1D,
            eBackwardsPhysSpace1D
        };

        /// A map between homo matrix keys and their associated block
        /// matrices.
        typedef map< Homogeneous1DMatType, DNekBlkMatSharedPtr> Homo1DBlockMatrixMap;
        /// A shared pointer to a BlockMatrixMap.
        typedef boost::shared_ptr<Homo1DBlockMatrixMap> Homo1DBlockMatrixMapShPtr;

        // Forward declaration for typedefs
        class ExpListHomogeneous1D;

        /// Shared pointer to an ExpList3DHomogeneous1D object.
        typedef boost::shared_ptr<ExpListHomogeneous1D>   ExpListHomogeneous1DSharedPtr;
        /// Vector of pointers to ExpList3DHomogeneous1D objects.
        typedef std::vector< ExpListHomogeneous1DSharedPtr > ExpListHomogeneous1DVector;
        /// Iterator for the vector of ExpList3DHomogeneous1D pointers.
        typedef std::vector< ExpListHomogeneous1DSharedPtr >::iterator ExpListHomogeneous1DVectorIter;

        /// Abstraction of a two-dimensional multi-elemental expansion which
        /// is merely a collection of local expansions.
        class ExpListHomogeneous1D: public ExpList
        {
        public:
            /// Default constructor.
            MULTI_REGIONS_EXPORT ExpListHomogeneous1D();

            MULTI_REGIONS_EXPORT ExpListHomogeneous1D(LibUtilities::CommSharedPtr &pComm,const LibUtilities::BasisKey &HomoBasis, const NekDouble lz, bool useFFT);

            /// Copy constructor.
            MULTI_REGIONS_EXPORT ExpListHomogeneous1D(const ExpListHomogeneous1D &In);

            /// Destructor.
            MULTI_REGIONS_EXPORT ~ExpListHomogeneous1D();

            MULTI_REGIONS_EXPORT void Homogeneous1DTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool IsForwards, bool UseContCoeffs = false);

            inline void HomogeneousFwdTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs = false);

            inline void HomogeneousBwdTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs = false);

            MULTI_REGIONS_EXPORT void ShuffleIntoHomogeneous1DClosePacked(
                              const Array<OneD, const NekDouble> &inarray,
                              Array<OneD, NekDouble> &outarray,
                              bool UseNumModes = false);

            MULTI_REGIONS_EXPORT void UnshuffleFromHomogeneous1DClosePacked(
                              const Array<OneD, const NekDouble> &inarray,
                              Array<OneD, NekDouble> &outarray,
                              bool UseNumModes = false);
			
            LibUtilities::BasisSharedPtr  GetHomogeneousBasis(void)
            {
                return m_homogeneousBasis;
            }
			
			MULTI_REGIONS_EXPORT void PhysDeriv(const Array<OneD, const NekDouble> &inarray,
								                Array<OneD, NekDouble> &out_d0,
								                Array<OneD, NekDouble> &out_d1, 
								                Array<OneD, NekDouble> &out_d2, bool UseContCoeffs);
			
			MULTI_REGIONS_EXPORT void PhysDeriv(Direction edir,
								                const Array<OneD, const NekDouble> &inarray,
								                Array<OneD, NekDouble> &out_d, bool UseContCoeffs);

            ExpListSharedPtr &GetPlane(int n)
            {
                return m_planes[n];
            }

        protected:
            
            /// FFT variables
            bool                                    m_useFFT;
            LibUtilities::NektarFFTSharedPtr        m_FFT;
            Array<OneD,NekDouble>                   m_tmpIN;
            Array<OneD,NekDouble>                   m_tmpOUT;
            
            /// Definition of the total number of degrees of freedom and
            /// quadrature points. Sets up the storage for \a m_coeff and \a
            ///  m_phys.
            LibUtilities::BasisSharedPtr    m_homogeneousBasis;
            NekDouble                       m_lhom;  ///< Width of homogeneous direction
            Homo1DBlockMatrixMapShPtr       m_homogeneous1DBlockMat;
            Array<OneD, ExpListSharedPtr>   m_planes;
			
            DNekBlkMatSharedPtr GenHomogeneous1DBlockMatrix(Homogeneous1DMatType mattype, bool UseContCoeffs = false) const;

            DNekBlkMatSharedPtr GetHomogeneous1DBlockMatrix(Homogeneous1DMatType mattype, bool UseContCoeffs = false) const;

            //  virtual functions
            virtual int v_GetNumElmts(void)
            {
                return m_planes[0]->GetExpSize();
            }

            virtual LibUtilities::BasisSharedPtr  v_GetHomogeneousBasis(void)
            {
                return GetHomogeneousBasis();
            }

            virtual void v_FwdTrans(const Array<OneD,const NekDouble> &inarray,
                                    Array<OneD,      NekDouble> &outarray,
                                    bool  UseContCoeffs);

            virtual void v_BwdTrans(const Array<OneD,const NekDouble> &inarray,
                                    Array<OneD,      NekDouble> &outarray,
                                    bool  UseContCoeffs);

            virtual void v_IProductWRTBase(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs);

            virtual std::vector<SpatialDomains::FieldDefinitionsSharedPtr>
                v_GetFieldDefinitions(void);

            virtual void v_GetFieldDefinitions(std::vector<SpatialDomains::FieldDefinitionsSharedPtr> &fielddef);

            virtual void v_AppendFieldData(SpatialDomains::FieldDefinitionsSharedPtr &fielddef, std::vector<NekDouble> &fielddata);
			
			virtual void v_AppendFieldData(SpatialDomains::FieldDefinitionsSharedPtr &fielddef, std::vector<NekDouble> &fielddata, Array<OneD, NekDouble> &coeffs);

            virtual void v_ExtractDataToCoeffs(SpatialDomains::FieldDefinitionsSharedPtr &fielddef, std::vector<NekDouble> &fielddata, std::string &field);

            virtual void v_WriteTecplotHeader(std::ofstream &outfile,
                                            std::string var = "v");

            virtual void v_WriteTecplotField(std::ofstream &outfile,
                                             int expansion);

            virtual void v_WriteVtkPieceData(std::ofstream &outfile, int expansion,
                                        std::string var);
			
			virtual void v_HomogeneousFwdTrans(const Array<OneD, const NekDouble> &inarray, 
												 Array<OneD, NekDouble> &outarray, 
												 bool UseContCoeffs = false);
			
			virtual void v_HomogeneousBwdTrans(const Array<OneD, const NekDouble> &inarray, 
												 Array<OneD, NekDouble> &outarray, 
												 bool UseContCoeffs = false);
			
			virtual void v_PhysDeriv(const Array<OneD, const NekDouble> &inarray,
									 Array<OneD, NekDouble> &out_d0,
									 Array<OneD, NekDouble> &out_d1, 
									 Array<OneD, NekDouble> &out_d2, bool UseContCoeffs);
			
			virtual void v_PhysDeriv(Direction edir,
									 const Array<OneD, const NekDouble> &inarray,
									 Array<OneD, NekDouble> &out_d, bool UseContCoeffs);

            virtual ExpListSharedPtr &v_GetPlane(int n)
            {
                return GetPlane(n);
            }

            virtual void v_Homogeneous1DFwdTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs = false)
            {
                Homogeneous1DFwdTrans(inarray,outarray,UseContCoeffs);
            }
            
            virtual void v_Homogeneous1DBwdTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs = false)
            {
                Homogeneous1DBwdTrans(inarray,outarray,UseContCoeffs);
            }

        private:
        };

        inline void ExpListHomogeneous1D::HomogeneousFwdTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs)
        {
			v_HomogeneousFwdTrans(inarray,outarray,UseContCoeffs);
        }
		
        inline void ExpListHomogeneous1D::HomogeneousBwdTrans(const Array<OneD, const NekDouble> &inarray, Array<OneD, NekDouble> &outarray, bool UseContCoeffs)
        {
            v_HomogeneousBwdTrans(inarray,outarray,UseContCoeffs);
        }

    } //end of namespace
} //end of namespace

#endif//EXPLISTHOMO1D_H

/**
* $Log: v $
*
**/

