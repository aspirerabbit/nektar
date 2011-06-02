///////////////////////////////////////////////////////////////////////////////
//
// File SessionReader.cpp
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
// Description: Session reader
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TIXML_USE_STL
#define TIXML_USE_STL
#endif

#include <iostream>
#include <string>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <tinyxml/tinyxml.h>
#include <LibUtilities/BasicUtils/ErrorUtil.hpp>
#include <SpatialDomains/Equation.h>

#include <LibUtilities/BasicUtils/SessionReader.h>

namespace Nektar
{
    namespace LibUtilities
    {
        SessionReader::SessionReader(std::string& pFilename)
            : m_filename(pFilename)
        {
            m_xmlDoc = new TiXmlDocument(pFilename);
            ASSERTL0(m_xmlDoc, "Failed to create XML document object.");

            bool loadOkay = m_xmlDoc->LoadFile();
            ASSERTL0(loadOkay, (std::string("Unable to load file: ") +
                pFilename).c_str());

            TiXmlHandle docHandle(m_xmlDoc);

            TiXmlNode* n = NULL;
            TiXmlElement* e = NULL;

            /// Look for all data in CONDITIONS block.
            e = docHandle.FirstChildElement("NEKTAR").FirstChildElement("CONDITIONS").Element();
            ASSERTL0(e, "Unable to find CONDITIONS tag in file.");

            ReadParameters(e);
            ReadSolverInfo(e);

            e = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();

            ReadGeometricInfo(e);
        }

        SessionReader::SessionReader(const SessionReader& pSrc)
        {
            m_filename = pSrc.m_filename;
            m_xmlDoc   = pSrc.m_xmlDoc;
            m_solverInfo = pSrc.m_solverInfo;
            m_parameters = pSrc.m_parameters;
        }

        SessionReader::~SessionReader()
        {

        }

        TiXmlDocument& SessionReader::GetDocument()
        {
            return *m_xmlDoc;
        }

        TiXmlElement* SessionReader::GetElement(const string& pPath)
        {
            std::string vPath = boost::to_upper_copy(pPath);
            std::vector<std::string> strs;
            boost::split(strs, vPath, boost::is_any_of("\\/ "));
            ASSERTL0(strs.size() > 0, "No path given in XML element request.");

            TiXmlElement* vReturn = m_xmlDoc->FirstChildElement(strs[0].c_str());
            ASSERTL0(vReturn, std::string("Cannot find element '")
                              + strs[0] + std::string("'."));
            for (int i = 1; i < strs.size(); ++i)
            {
                vReturn = vReturn->FirstChildElement(strs[i].c_str());
                ASSERTL0(vReturn, std::string("Cannot find element '")
                                  + strs[i] + std::string("'."));
            }
            return vReturn;
        }

        bool SessionReader::DefinesElement(const std::string& pPath)
        {
            std::string vPath = boost::to_upper_copy(pPath);
            std::vector<std::string> strs;
            boost::split(strs, vPath, boost::is_any_of("\\/ "));
            ASSERTL0(strs.size() > 0, "No path given in XML element request.");

            TiXmlElement* vReturn = m_xmlDoc->FirstChildElement(strs[0].c_str());
            ASSERTL0(vReturn, std::string("Cannot find element '")
                              + strs[0] + std::string("'."));
            for (int i = 1; i < strs.size(); ++i)
            {
                vReturn = vReturn->FirstChildElement(strs[i].c_str());
                if (!vReturn) return false;
            }
            return true;
        }

        const std::string& SessionReader::GetFilename()
        {
            return m_filename;
        }

        const std::string& SessionReader::GetSolverInfo(const std::string &pProperty)
        {
            std::string vProperty = boost::to_upper_copy(pProperty);
            SolverInfoMap::iterator slvIter = m_solverInfo.find(vProperty);

            ASSERTL1(slvIter != m_solverInfo.end(),
                (std::string("Unable to find requested property: ") + pProperty).c_str());

            return slvIter->second;
        }

        NekDouble SessionReader::GetParameter(std::string pName)
        {
            std::string vName = boost::to_upper_copy(pName);
            ParameterMap::iterator paramMapIter = m_parameters.find(vName);

            ASSERTL0(paramMapIter != m_parameters.end(),
                (std::string("Unable to find requested parameter: ") + pName).c_str());

            return paramMapIter->second;
        }

        void SessionReader::LoadParameter(const std::string pName, int &pVar, int pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            ParameterMap::iterator paramMapIter = m_parameters.find(vName);
            if(paramMapIter != m_parameters.end())
            {
                pVar = paramMapIter->second;
            }
            else
            {
                pVar  = pDefault;
            }
        }

        void SessionReader::LoadParameter(const std::string pName, NekDouble& pVar, const NekDouble pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            ParameterMap::iterator paramMapIter = m_parameters.find(vName);
            if(paramMapIter != m_parameters.end())
            {
                pVar = paramMapIter->second;
            }
            else
            {
                pVar  = pDefault;
            }
        }

        bool SessionReader::DefinesParameter(const std::string pName)
        {
            std::string vName = boost::to_upper_copy(pName);
            ParameterMap::iterator paramMapIter = m_parameters.find(vName);
            return (paramMapIter != m_parameters.end());
        }

        void SessionReader::LoadSolverInfo(const std::string pName, std::string& pVar, const std::string pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            SolverInfoMap::iterator solverInfoMapIter = m_solverInfo.find(vName);
            if(solverInfoMapIter != m_solverInfo.end())
            {
                pVar = solverInfoMapIter->second;
            }
            else
            {
                pVar  = pDefault;
            }
        }

        void SessionReader::MatchSolverInfo(const std::string pName, const std::string pTrueVal, bool& pVar, const bool pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            SolverInfoMap::iterator solverInfoMapIter = m_solverInfo.find(vName);
            if(solverInfoMapIter != m_solverInfo.end())
            {
                pVar = (NoCaseStringCompare(solverInfoMapIter->second, pTrueVal) == 0);
            }
            else
            {
                pVar  = pDefault;
            }
        }

        bool SessionReader::DefinesSolverInfo(const std::string pName)
        {
            std::string vName = boost::to_upper_copy(pName);
            SolverInfoMap::iterator solverInfoMapIter = m_solverInfo.find(vName);
            return (solverInfoMapIter != m_solverInfo.end());
        }

        void SessionReader::LoadGeometricInfo(const std::string pName,
                                std::string& pVar, const std::string pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            GeometricInfoMap::iterator geometricInfoMapIter = m_geometricInfo.find(vName);
            if(geometricInfoMapIter != m_geometricInfo.end())
            {
                pVar = geometricInfoMapIter->second;
            }
            else
            {
                pVar  = pDefault;
            }
        }

        void SessionReader::LoadGeometricInfo(const std::string pName, bool& pVar,
                                const bool pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            GeometricInfoMap::iterator geometricInfoMapIter = m_geometricInfo.find(vName);
            if(geometricInfoMapIter != m_geometricInfo.end())
            {
                std::string s = geometricInfoMapIter->second;
                if (s == "TRUE")
                {
                    pVar = true;
                }
                else
                {
                    pVar = false;
                }
            }
            else
            {
                pVar  = pDefault;
            }
        }

        void SessionReader::MatchGeometricInfo(const std::string pName,
                                const std::string pTrueVal, bool& pVar,
                                const bool pDefault)
        {
            std::string vName = boost::to_upper_copy(pName);
            GeometricInfoMap::iterator geometricInfoMapIter = m_geometricInfo.find(vName);
            if(geometricInfoMapIter != m_geometricInfo.end())
            {
                pVar = (NoCaseStringCompare(geometricInfoMapIter->second, pTrueVal) == 0);
            }
            else
            {
                pVar  = pDefault;
            }
        }

        bool SessionReader::DefinesGeometricInfo(const std::string pName)
        {
            std::string vName = boost::to_upper_copy(pName);
            GeometricInfoMap::iterator geometricInfoMapIter = m_geometricInfo.find(vName);
            return (geometricInfoMapIter != m_geometricInfo.end());
        }

        void SessionReader::ReadSolverInfo(TiXmlElement *conditions)
        {
            TiXmlElement *solverInfoElement = conditions->FirstChildElement("SOLVERINFO");

            if (solverInfoElement)
            {
                TiXmlElement *solverInfo = solverInfoElement->FirstChildElement("I");

                while (solverInfo)
                {
                    std::string solverProperty = solverInfo->Attribute("PROPERTY");
                    ASSERTL0(!solverProperty.empty(), "Unable to find PROPERTY value.");

                    // make sure that solver property is capitalised
                    boost::to_upper(solverProperty);

                    // check property has not already been defined
                    SolverInfoMap::iterator solverInfoIter = m_solverInfo.find(solverProperty);
                    ASSERTL0(solverInfoIter == m_solverInfo.end(),
                             (std::string("SolverInfo value: ") + solverProperty
                              + std::string(" already specified.")).c_str());

                    // read the value
                    std::string solverValue    = solverInfo->Attribute("VALUE");
                    ASSERTL0(!solverValue.empty(),"Unable to find VALUE string");

                    // Set Variable
                    m_solverInfo[solverProperty] = solverValue;
                    solverInfo = solverInfo->NextSiblingElement("I");
                }
            }
        }


        void SessionReader::ReadParameters(TiXmlElement *conditions)
        {
            TiXmlElement *parametersElement = conditions->FirstChildElement("PARAMETERS");

            // See if we have parameters defined.  They are optional so we go on if not.
            if (parametersElement)
            {
                TiXmlElement *parameter = parametersElement->FirstChildElement("P");
                LibUtilities::ExpressionEvaluator expEvaluator;

                // Multiple nodes will only occur if there is a comment in between
                // definitions.
                while (parameter)
                {
                    TiXmlNode *node = parameter->FirstChild();

                    while (node && node->Type() != TiXmlNode::TEXT)
                    {
                        node = node->NextSibling();
                    }

                    if (node)
                    {
                        // Format is "paramName = value"
                        std::string line = node->ToText()->Value();

                        /// Pull out lhs and rhs and eliminate any spaces.
                        int beg=line.find_first_not_of(" ");
                        int end=line.find_first_of("=");
                        std::string lhs = line.substr(line.find_first_not_of(" "), end-beg-1);
                        lhs = lhs.substr(0, lhs.find_last_not_of(" ")+1);

                        std::string rhs = line.substr(line.find_last_of("=")+1);
                        rhs = rhs.substr(rhs.find_first_not_of(" "));
                        rhs = rhs.substr(0, rhs.find_last_not_of(" ")+1);

                        /// We want the list of parameters to have their RHS evaluated,
                        /// so we use the expression evaluator to do the dirty work.
                        if (!lhs.empty() && !rhs.empty())
                        {
                            NekDouble value=0.0;
                            expEvaluator.DefineFunction("", rhs);
                            value =  expEvaluator.Evaluate();
                            expEvaluator.SetParameter(lhs, value);
                            boost::to_upper(lhs);
                            m_parameters[lhs] = value;

                        }
                    }

                    parameter = parameter->NextSiblingElement();
                }

                // Set ourselves up for evaluation later.
                //SpatialDomains::Equation::SetConstParameters(mParameters);
            }
        }

        void SessionReader::ReadGeometricInfo(TiXmlElement *geometry)
        {
            TiXmlElement *geometricInfoElement = geometry->FirstChildElement("GEOMINFO");

            if (geometricInfoElement)
            {
                TiXmlElement *geometricInfo = geometricInfoElement->FirstChildElement("I");

                while (geometricInfo)
                {
                    std::string geometricProperty = geometricInfo->Attribute("PROPERTY");
                    ASSERTL0(!geometricProperty.empty(), "Unable to find PROPERTY value.");

                    // make sure that geometric property is capitalised
                    boost::to_upper(geometricProperty);

                    // check the property has not already been defined
                    GeometricInfoMap::iterator geometricInfoIter = m_geometricInfo.find(geometricProperty);
                    ASSERTL0(geometricInfoIter == m_geometricInfo.end(),
                             (std::string("geometricInfo value: ") + geometricProperty
                              + std::string(" already specified.")).c_str());

                    // read the property value
                    std::string geometricValue    = geometricInfo->Attribute("VALUE");
                    ASSERTL0(!geometricValue.empty(),"Unable to find VALUE string");

                    // Set Variable
                    m_geometricInfo[geometricProperty] = geometricValue;
                    geometricInfo = geometricInfo->NextSiblingElement("I");
                }
            }
        }


        int SessionReader::NoCaseStringCompare(const std::string & s1, const std::string& s2)
        {
            //if (s1.size() < s2.size()) return -1;
            //if (s1.size() > s2.size()) return 1;

            std::string::const_iterator it1=s1.begin();
            std::string::const_iterator it2=s2.begin();

            //stop when either string's end has been reached
            while ( (it1!=s1.end()) && (it2!=s2.end()) )
            {
                if(::toupper(*it1) != ::toupper(*it2)) //letters differ?
                {
                    // return -1 to indicate smaller than, 1 otherwise
                    return (::toupper(*it1)  < ::toupper(*it2)) ? -1 : 1;
                }

                //proceed to the next character in each string
                ++it1;
                ++it2;
            }

            size_t size1=s1.size();
            size_t size2=s2.size();// cache lengths

            //return -1,0 or 1 according to strings' lengths
            if (size1==size2)
            {
                return 0;
            }

            return (size1 < size2) ? -1 : 1;
        }
    }
}