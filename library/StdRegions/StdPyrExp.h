///////////////////////////////////////////////////////////////////////////////
//
// File StdPyrExp.h
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
// Description: Header field for pyramidic routines built upon
// StdExpansion3D
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_LIBS_STDREGIONS_STDPYREXP_H
#define NEKTAR_LIBS_STDREGIONS_STDPYREXP_H

#include <StdRegions/StdExpansion3D.h>


namespace Nektar
{
    namespace StdRegions
    {
    
    class StdPyrExp: public StdExpansion3D
    {
        
    public:
        
        /** \brief Constructor using BasisKey class for quadrature
         *  points and order definition 
         */
        StdPyrExp(const BasisKey &Ba, const BasisKey &Bb, const BasisKey &Bc);
        
        /** \brief Constructor using BasisKey class for quadrature
         *    points and order definition where m_coeffs and m_phys are all
         *  set
         */
        StdPyrExp(const BasisKey &Ba, const BasisKey &Bb, const BasisKey &Bc,
              double *coeffs, double *phys);
        
        /** \brief Copy Constructor */
        StdPyrExp(const StdPyrExp &T);
        
        /** \brief Destructor */
        ~StdPyrExp();
        
        /** \brief Return Shape of region, using  ShapeType enum list. 
         *  i.e. Pyramid
         */
        ShapeType DetShapeType()
        {
        return ePyramid;
        };
        
    protected:
        
        static StdMatrix s_elmtmats;
        
    private:
        
        virtual int v_GetNverts() const
        {
            return 5;
        }
        
        virtual int v_GetNedges() const
        {
            return 8;
        }
        
        virtual int v_GetNfaces() const
        {
            return 5;
        }

        virtual ShapeType v_DetShapeType() const
        {
        return DetShapeType();
        }
    };
    
    } //end of namespace
} //end of namespace

#endif //STDPYREXP_H

/**
 * $Log: StdPyrExp.h,v $
 * Revision 1.4  2007/07/10 21:05:17  kirby
 * even more fixes
 *
 * Revision 1.3  2007/01/17 16:05:40  pvos
 * updated doxygen documentation
 *
 * Revision 1.2  2006/07/02 17:16:18  sherwin
 *
 * Modifications to make MultiRegions work for a connected domain in 2D (Tris)
 *
 * Revision 1.1  2006/05/04 18:58:32  kirby
 * *** empty log message ***
 *
 * Revision 1.23  2006/03/06 17:12:46  sherwin
 *
 * Updated to properly execute all current StdRegions Demos.
 *
 * Revision 1.22  2006/03/04 20:26:55  bnelson
 * Added comments after #endif.
 *
 * Revision 1.21  2006/03/01 08:25:04  sherwin
 *
 * First compiling version of StdRegions
 *
 **/

