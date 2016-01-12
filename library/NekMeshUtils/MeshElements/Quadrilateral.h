////////////////////////////////////////////////////////////////////////////////
//
//  File: Mesh.h
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
//  Description: Mesh manipulation objects.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef NekMeshUtils_MESHELEMENTS_QUAD
#define NekMeshUtils_MESHELEMENTS_QUAD

#include <NekMeshUtils/NekMeshUtilsDeclspec.h>

namespace Nektar
{
namespace NekMeshUtils
{
    /**
     * @brief A 2-dimensional four-sided element.
     */
    class Quadrilateral : public Element {
    public:
        /// Creates an instance of this class
        static ElementSharedPtr create(
            ElmtConfig                 pConf,
            std::vector<NodeSharedPtr> pNodeList,
            std::vector<int>           pTagList)
        {
            ElementSharedPtr e = boost::shared_ptr<Element>(
                new Quadrilateral(pConf, pNodeList, pTagList));
            vector<EdgeSharedPtr> m_edges = e->GetEdgeList();
            for (int i = 0; i < m_edges.size(); ++i)
            {
                m_edges[i]->m_elLink.push_back(pair<ElementSharedPtr, int>(e,i));
            }
            return e;
        }
        /// Element type
        static LibUtilities::ShapeType m_type;

        NEKMESHUTILS_EXPORT Quadrilateral(ElmtConfig                 pConf,
                                         std::vector<NodeSharedPtr> pNodeList,
                                         std::vector<int>           pTagList);
        NEKMESHUTILS_EXPORT Quadrilateral(const Quadrilateral& pSrc);
        NEKMESHUTILS_EXPORT virtual ~Quadrilateral() {}

        NEKMESHUTILS_EXPORT virtual SpatialDomains::GeometrySharedPtr GetGeom(int coordDim);
        NEKMESHUTILS_EXPORT virtual void Complete(int order);

        NEKMESHUTILS_EXPORT static unsigned int GetNumNodes(ElmtConfig pConf);
    };

}
}

#endif
