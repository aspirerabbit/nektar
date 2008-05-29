////////////////////////////////////////////////////////////////////////////////
//
//  File:  ParseUtils.hpp
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
//  Description:  This file contains various parsing utilities, primarily used
//                by SpatialDomains to process input files.
//
//
////////////////////////////////////////////////////////////////////////////////
#ifndef NEKTAR_LIBUTILITIES_PARSEUTILS_HPP
#define NEKTAR_LIBUTILITIES_PARSEUTILS_HPP

#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>

using namespace boost::spirit;

namespace Nektar
{
    class ParseUtils
    {
    public:
        static bool ParseRealAssignment(const char *const str, std::string &symbol, double &value)
        {
            SymbolFunctor symbolFunctor(&symbol);
            ValueFunctor valueFunctor(&value);
            
            return parse(str,
                // Begin grammar
                (
                lexeme_d[alpha_p >> *alnum_p][symbolFunctor] >> "=" >> real_p[valueFunctor]
                )
                ,
                // End grammar

                space_p).full;
        }

        static bool GenerateSeqVector(const char *const str, vector<unsigned int> &vec)
        {
            // Functors used to parse the sequence.
            fctor1 functor1(&vec);
            fctor2 functor2(&vec);

            return parse(str,
                //  Begin grammar
                (
                uint_p[functor1] >> !('-' >> uint_p[functor2]) >>
                *(',' >> uint_p[functor1] >> !('-' >> uint_p[functor2]))
                )
                ,
                //  End grammar

                space_p).full;
        }

        static bool GenerateOrderedVector(const char *const str, vector<unsigned int> &vec)
        {
            // Functors used to parse the sequence.
            fctor1 functor1(&vec);

            return parse(str,
                //  Begin grammar
                (
                uint_p[functor1] >> *(',' >> uint_p[functor1])
                )
                ,
                //  End grammar

                space_p).full;
        }

		static bool GenerateOrderedStringVector(const char *const str, vector<std::string> &vec)
        {
            // Functors used to parse the sequence.
            fctor3 functor3(&vec);

            return parse(str,
                //  Begin grammar
                (
                (+(print_p - ','))[functor3] >> *(',' >> (+(print_p - ','))[functor3])
                )
                ,
                //  End grammar

                space_p).full;
        }

    private:

        struct SymbolFunctor
        {
            SymbolFunctor(std::string *symbol):
                m_symbol(symbol)
            {
            }

            void operator()(const char *beg, const char *end) const
            {
                m_symbol->assign(beg, end-beg);
            }

        private:
            std::string *m_symbol;
        };

        struct ValueFunctor
        {
            ValueFunctor(double *value):
                m_value(value)
            {
            }
            
            void operator()(double val) const
            {
                *m_value = val;
            }

        private:
            double *m_value;
        };

        struct fctor1
        {
            fctor1(vector<unsigned int> *vec):
            m_vector(vec)
            {
            }

            void operator()(unsigned int n) const
            {
                if (!m_vector->empty())
                {
                    unsigned int prevElem = m_vector->back();

                    if (n > prevElem)
                    {
                        m_vector->push_back(n);
                    }
                }
                else
                {
                    m_vector->push_back(n);
                }
            }

        private:
            vector<unsigned int> *m_vector;
            fctor1();
        };

        struct fctor2
        {
            fctor2(vector<unsigned int> *vec):
            m_vector(vec)
            {
            }

            void operator()(unsigned int n) const
            {
                unsigned int prevElem = m_vector->back();

                for (unsigned int i=prevElem+1; i<=n; ++i)
                {
                    m_vector->push_back(i);
                }
            }

        private:
            vector<unsigned int> *m_vector;
        };

        struct fctor3
        {
			fctor3(vector<std::string> *vec):
            m_vector(vec)
            {
            }

            void operator()(char const* first, char const* last) const
            {
				m_vector->push_back(std::string(first, last));
            }

        private:
			vector<std::string> *m_vector;
        };
    };
}

#endif //NEKTAR_LIBUTILITIES_PARSEUTILS_HPP
