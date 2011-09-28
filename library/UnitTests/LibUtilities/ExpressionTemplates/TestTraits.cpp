///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_USE_EXPRESSION_TEMPLATES
#define NEKTAR_USE_EXPRESSION_TEMPLATES
#endif //NEKTAR_USE_EXPRESSION_TEMPLATES

#include <boost/test/auto_unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>
#include <LibUtilities/BasicUtils/OperatorGenerators.hpp>
#include <UnitTests/CountedObject.h>
#include <LibUtilities/LinearAlgebra/NekMatrix.hpp>
#include <LibUtilities/LinearAlgebra/NekVector.hpp>

namespace Nektar
{
    
    BOOST_AUTO_TEST_CASE(TestVectorUnrolling)
    {
        typedef expt::Node< NekVector<double> > ConstantNode;
        BOOST_MPL_ASSERT(( expt::NodeCanUnroll<ConstantNode> ));

        typedef expt::Node< double > ConstantDoubleNode;
        BOOST_MPL_ASSERT(( boost::mpl::not_<expt::NodeCanUnroll<ConstantDoubleNode> > ));

        typedef expt::Node<ConstantNode, expt::AddOp, ConstantNode> AddNode;
        BOOST_MPL_ASSERT(( expt::NodeCanUnroll<AddNode> ));

        typedef expt::Node<ConstantNode, expt::SubtractOp, ConstantNode> SubtractNode;
        BOOST_MPL_ASSERT(( expt::NodeCanUnroll<SubtractNode> ));

        typedef expt::Node<AddNode, expt::SubtractOp, AddNode> TwoLevelNode;
        BOOST_MPL_ASSERT(( expt::NodeCanUnroll<TwoLevelNode> ));

        typedef expt::Node<ConstantNode, expt::MultiplyOp, ConstantNode> MultiplyNode;
        BOOST_MPL_ASSERT(( boost::mpl::not_<expt::NodeCanUnroll<MultiplyNode> > ));
    }
}













