///////////////////////////////////////////////////////////////////////////////
//
// File: MappingXYofT.h
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
// Description: Mapping of the type X = x + f(t), Y = y + g(t)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_GLOBALMAPPING_MAPPINGXYOFT
#define NEKTAR_GLOBALMAPPING_MAPPINGXYOFT

#include <string>

#include <LibUtilities/BasicUtils/NekFactory.hpp>
#include <LibUtilities/BasicUtils/SharedArray.hpp>
#include <MultiRegions/ExpList.h>
#include <GlobalMapping/GlobalMappingDeclspec.h>
#include <GlobalMapping/MappingIdentity.h>

namespace Nektar
{
namespace GlobalMapping
{

    class MappingXYofT: public MappingIdentity
    {
    public:

        friend class MemoryManager<MappingXYofT> ;

        /// Creates an instance of this class
        GLOBAL_MAPPING_EXPORT
        static MappingSharedPtr create(
            const LibUtilities::SessionReaderSharedPtr        &pSession,
            const Array<OneD, MultiRegions::ExpListSharedPtr> &pFields,
            const TiXmlElement                                *pMapping)
        {
            MappingSharedPtr p =
                    MemoryManager<MappingXYofT>::AllocateSharedPtr(pSession, 
                                                                   pFields);
            p->InitObject(pFields, pMapping);
            return p;
        }

        ///Name of the class
        static std::string className;

    protected:
        // Name of the function containing the coordinates velocity
        string                                      m_velFuncName;    

        // Constructor
        MappingXYofT(const LibUtilities::SessionReaderSharedPtr   &pSession,
                const Array<OneD, MultiRegions::ExpListSharedPtr> &pFields);
        
        // Virtual functions
        GLOBAL_MAPPING_EXPORT
        virtual void v_InitObject(
            const Array<OneD, MultiRegions::ExpListSharedPtr> &pFields,
            const TiXmlElement                                *pMapping);

        GLOBAL_MAPPING_EXPORT virtual void v_GetCartesianCoordinates(
                Array<OneD, NekDouble>               &out0,
                Array<OneD, NekDouble>               &out1,
                Array<OneD, NekDouble>               &out2);

        GLOBAL_MAPPING_EXPORT virtual void v_GetCoordVelocity(
            Array<OneD, Array<OneD, NekDouble> >              &outarray); 

        GLOBAL_MAPPING_EXPORT virtual bool v_IsTimeDependent();  

        GLOBAL_MAPPING_EXPORT virtual void v_UpdateMapping(const NekDouble time);

    private:

    };

}
}

#endif
