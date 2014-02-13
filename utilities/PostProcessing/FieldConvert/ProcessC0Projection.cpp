////////////////////////////////////////////////////////////////////////////////
//
//  File: ProcessC0Projection.cpp
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
//  Description: Computes C0 projection.
//
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
using namespace std;

#include "ProcessC0Projection.h"

#include <LibUtilities/BasicUtils/SharedArray.hpp>
#include <LibUtilities/BasicUtils/ParseUtils.hpp>

namespace Nektar
{
    namespace Utilities
    {
        ModuleKey ProcessC0Projection::className =
        GetModuleFactory().RegisterCreatorFunction(
            ModuleKey(eProcessModule, "C0Projection"),
                                                   ProcessC0Projection::create, "Computes C0 projection.");
        
        ProcessC0Projection::ProcessC0Projection(FieldSharedPtr f) : ProcessModule(f)
        {
            m_config["fields"] = ConfigOption(false,"All","Start field to project");
        }
        
        ProcessC0Projection::~ProcessC0Projection()
        {
        }
        
        void ProcessC0Projection::Process(po::variables_map &vm)
        {
            
            // generate an C0 expansion field with no boundary conditions. 
            bool savedef = m_f->m_declareExpansionAsContField;
            m_f->m_declareExpansionAsContField = true;
            m_c0ProjectExp = m_f->AppendExpList("DefaultVar",true);
            m_f->m_declareExpansionAsContField = savedef;
            
            if (m_f->m_verbose)
            {
                cout << "ProcessC0Projection: Projects fiels into C0 space..." << endl;
            }
            
            int nfields = m_f->m_exp.size();
            
            string fields = m_config["fields"].as<string>();
            vector<unsigned int> processFields; 
            
            if(fields.compare("All") == 0)
            {
                for(int i = 0; i < nfields; ++i)
                {
                    processFields.push_back(i);
                }
            }
            else
            {
                ASSERTL0(ParseUtils::GenerateOrderedVector(fields.c_str(),
                                                           processFields),
                         "Failed to interpret field string in C0Projection");
            }
            
            for (int i = 0; i < processFields.size(); ++i)
            {
                ASSERTL0(processFields[i] < nfields,"Attempt to process field that is larger than then number of fields available");
                
                if (m_f->m_verbose)
                {
                    cout << "\t Processing field: " << processFields[i] << endl;
                }
                m_c0ProjectExp->BwdTrans(m_f->m_exp[processFields[i]]->GetCoeffs(),
                                         m_f->m_exp[processFields[i]]->UpdatePhys());
                m_c0ProjectExp->FwdTrans(m_f->m_exp[processFields[i]]->GetPhys(),
                                         m_f->m_exp[processFields[i]]->UpdateCoeffs());
            }
            
            // reset up FieldData with new values before projecting 
            std::vector<std::vector<NekDouble> > FieldData(m_f->m_fielddef.size());
            
            for(int i = 0; i < nfields; ++i)
            {
                for (int j = 0; j < m_f->m_fielddef.size(); ++j)
                {   
                    m_f->m_exp[i]->AppendFieldData(m_f->m_fielddef[j], FieldData[j]);
                }
            }
            
            m_f->m_data = FieldData;
            
        }
    }
}
