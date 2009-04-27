//////////////////////////////////////////////////////////////////////////////
//
// File DisContField2D.cpp
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
// Description: Field definition for 2D domain with boundary
// conditions using LDG flux
//
///////////////////////////////////////////////////////////////////////////////

#include <MultiRegions/DisContField2D.h>
#include <LocalRegions/GenSegExp.h>

namespace Nektar
{
    namespace MultiRegions
    {

        DisContField2D::DisContField2D(void):
            m_bndCondExpansions(),
            m_bndConditions()
        {
        }

        DisContField2D::DisContField2D(const DisContField2D &In):
            ExpList2D(In),
            m_bndCondExpansions   (In.m_bndCondExpansions),
            m_bndConditions       (In.m_bndConditions),
            m_globalBndMat        (In.m_globalBndMat),
            m_trace               (In.m_trace),  
            m_traceMap            (In.m_traceMap)
        {
        }

        DisContField2D::DisContField2D(SpatialDomains::MeshGraph2D &graph2D,
                                       SpatialDomains::BoundaryConditions &bcs,
                                       const int bc_loc):
            ExpList2D(graph2D),
            m_bndCondExpansions(),
            m_bndConditions()
        {
            GenerateBoundaryConditionExpansion(graph2D,bcs,bcs.GetVariable(bc_loc));
            EvaluateBoundaryConditions();

            // Set up matrix map
            m_globalBndMat = MemoryManager<GlobalLinSysMap>::AllocateSharedPtr();
            map<int,int> periodicEdges;
            map<int,int> periodicVertices;
            GetPeriodicEdges(graph2D,bcs,bcs.GetVariable(bc_loc),periodicVertices,periodicEdges);

            // Set up Trace space
            m_trace = MemoryManager<GenExpList1D>::AllocateSharedPtr(m_bndCondExpansions, m_bndConditions,*m_exp,graph2D, periodicEdges);


            m_traceMap = MemoryManager<LocalToGlobalDGMap>::AllocateSharedPtr(graph2D,m_trace,m_exp,m_bndCondExpansions,m_bndConditions, periodicEdges);
        }

        DisContField2D::DisContField2D(SpatialDomains::MeshGraph2D &graph2D,
                                       SpatialDomains::BoundaryConditions &bcs,
                                       const std::string variable):
            ExpList2D(graph2D),
            m_bndCondExpansions(),
            m_bndConditions()
            
        {
            GenerateBoundaryConditionExpansion(graph2D,bcs,variable);
            EvaluateBoundaryConditions();

            // Set up matrix map
            m_globalBndMat   = MemoryManager<GlobalLinSysMap>::AllocateSharedPtr();

            map<int,int> periodicEdges;
            map<int,int> periodicVertices;
            GetPeriodicEdges(graph2D,bcs,variable,periodicVertices,periodicEdges);

            // Set up Trace space
            m_trace = MemoryManager<GenExpList1D>::AllocateSharedPtr(m_bndCondExpansions,m_bndConditions,*m_exp,graph2D,periodicEdges);

            m_traceMap = MemoryManager<LocalToGlobalDGMap>::AllocateSharedPtr(graph2D,m_trace,m_exp,m_bndCondExpansions,m_bndConditions, periodicEdges);
        }




        void DisContField2D::GenerateBoundaryConditionExpansion(SpatialDomains::MeshGraph2D &graph2D,
                                                                SpatialDomains::BoundaryConditions &bcs, 
                                                                const std::string variable)
        {
            int cnt  = 0;
            SpatialDomains::BoundaryRegionCollection    &bregions = bcs.GetBoundaryRegions();
            SpatialDomains::BoundaryConditionCollection &bconditions = bcs.GetBoundaryConditions();   

            int nbnd = bregions.size();

            // count the number of non-periodic boundary regions
            int cnt2 = 0;
            for(int i = 0; i < nbnd; ++i)
            {
                if( ((*(bconditions[i]))[variable])->GetBoundaryConditionType() != SpatialDomains::ePeriodic )
                {
                    cnt++;
                    if( ((*(bconditions[i]))[variable])->GetBoundaryConditionType() == SpatialDomains::eDirichlet )
                    {
                        cnt2++;
                    }
                }
            }
            m_numDirBndCondExpansions = cnt2;
            m_bndCondExpansions  = Array<OneD,MultiRegions::ExpList1DSharedPtr>(cnt);
            m_bndConditions      = Array<OneD,SpatialDomains::BoundaryConditionShPtr>(cnt);
            
            SetBoundaryConditionExpansion(graph2D,bcs,variable,m_bndCondExpansions,m_bndConditions);
        }
        
        DisContField2D::~DisContField2D()
        {
        }

        GlobalLinSysSharedPtr DisContField2D::GetGlobalBndLinSys(const GlobalLinSysKey &mkey) 
        {
            GlobalLinSysSharedPtr glo_matrix;
            GlobalLinSysMap::iterator matrixIter = m_globalBndMat->find(mkey);

            if(matrixIter == m_globalBndMat->end())
            {
                glo_matrix = GenGlobalBndLinSys(mkey,*m_traceMap);
                (*m_globalBndMat)[mkey] = glo_matrix;
            }
            else
            {
                glo_matrix = matrixIter->second;
            }

            return glo_matrix;
        }


        void DisContField2D::HelmSolve(const Array<OneD, const NekDouble> &inarray,
                                             Array<OneD,       NekDouble> &outarray,
                                             NekDouble lambda,
                                             NekDouble tau)
        {
            int e,i,j,n,cnt,cnt1,nbndry, order_e;
            int nexp = GetExpSize();
            StdRegions::StdExpansionSharedPtr BndExp;

            Array<OneD,NekDouble> f(m_ncoeffs);
            DNekVec F(m_ncoeffs,f,eWrapper);
            Array<OneD,NekDouble> e_f, e_l;

            //----------------------------------
            //  Setup RHS Inner product
            //----------------------------------
            IProductWRTBase(inarray,f);
            Vmath::Neg(m_ncoeffs,f,1);

            //----------------------------------
            //  Solve continuous flux System
            //----------------------------------
            int GloBndDofs   = m_traceMap->GetNumGlobalBndCoeffs();
            int NumDirichlet = m_traceMap->GetNumLocalDirBndCoeffs();
            int e_ncoeffs, loc,id,offset;
            NekDouble sign;

            // linked data
            GlobalMatrixKey HDGLamToUKey(StdRegions::eHybridDGLamToU,lambda,tau);
            const DNekScalBlkMatSharedPtr &HDGLamToU = GetBlockMatrix(HDGLamToUKey);

            Array<OneD,NekDouble> BndSol = m_trace->UpdateCoeffs(); 
            // Zero trace space
            Vmath::Zero(GloBndDofs,BndSol,1);

            int     LocBndCoeffs = m_traceMap->GetNumLocalBndCoeffs();
            Array<OneD, NekDouble> loc_lambda(LocBndCoeffs); 
            DNekVec LocLambda(LocBndCoeffs,loc_lambda,eWrapper);

            //----------------------------------
            // Evaluate Trace Forcing
            //----------------------------------

            // Determing <u_lam,f> terms using HDGLamToU matrix
            for(cnt1 = cnt = n = 0; n < nexp; ++n)
            {
                nbndry = (*m_exp)[n]->NumDGBndryCoeffs(); 		    
                
                e_ncoeffs = (*m_exp)[n]->GetNcoeffs();
                e_f       = f+cnt;
                e_l       = loc_lambda + cnt1;

                // use outarray as tmp space
                DNekVec     Floc    (nbndry, e_l, eWrapper); 
                DNekVec     ElmtFce (e_ncoeffs, e_f, eWrapper);
                Floc = Transpose(*(HDGLamToU->GetBlock(n,n)))*ElmtFce;

                cnt  += e_ncoeffs;
                cnt1 += nbndry;
            }
            // Might be nice if we could do 
            // LocLambda = Transpose(*HDGLamToU)*F;

            // Assemble into global operator
            m_traceMap->AssembleBnd(loc_lambda,BndSol);

            
            cnt = 0;
            // Copy Dirichlet boundary conditions into trace space        
            for(i = 0; i < m_numDirBndCondExpansions; ++i)
            {
                for(j = 0; j < (m_bndCondExpansions[i])->GetNcoeffs(); ++j)
                {
                    id = m_traceMap->GetBndCondCoeffsToGlobalCoeffsMap(cnt++);
                    BndSol[id] = m_bndCondExpansions[i]->GetCoeffs()[j];
                }
            }

            //Add weak boundary condition to trace forcing 
            for(i = m_numDirBndCondExpansions; i < m_bndCondExpansions.num_elements(); ++i)
            {
                for(j = 0; j < (m_bndCondExpansions[i])->GetNcoeffs(); ++j)
                {
                    id = m_traceMap->GetBndCondCoeffsToGlobalCoeffsMap(cnt++);
                    BndSol[id] += m_bndCondExpansions[i]->GetCoeffs()[j];
                }
            }

            // Dirichlet boundary forcing <\tilde{q}_lam,g>  (using Bmatsys)
            Array<OneD, const int> LocToGloBndMap = m_traceMap->GetLocalToGlobalBndMap();
            Array<OneD, const NekDouble> LocToGloBndSign = m_traceMap->GetLocalToGlobalBndSign();
            for(cnt = e = 0; e < nexp; ++e)
            {
                nbndry = (*m_exp)[e]->NumDGBndryCoeffs();
                // check to see if element has Dirichlet boundary
                // Probably could use a quicker check here
                if(Vmath::Vmin(nbndry,&LocToGloBndMap[cnt],1) < NumDirichlet)
                {
                    // Get BndSys Matrix - Could get rid of searching here 
                    LocalRegions::MatrixKey Bmatkey(StdRegions::eHybridDGHelmBndLam,
                                                    (*m_exp)[e]->DetExpansionType(),
                                                    *((*m_exp)[e]), lambda, tau);
                    DNekScalMat &BndSys = *((*m_exp)[e]->GetLocMatrix(Bmatkey));
                    Array<OneD, NekDouble> vout(nbndry,0.0);

                    // Set up Edge Dirichlet Values
                    for(cnt1 = i = 0; i < (*m_exp)[e]->GetNedges(); ++i)
                    {
                        e_ncoeffs = (*m_exp)[e]->GetEdgeNcoeffs(i); 

                        id = LocToGloBndMap[cnt+cnt1];
                        if(id < NumDirichlet)
                        {
                            for(j = 0; j < e_ncoeffs; ++j)
                            {
                                id        = LocToGloBndMap[cnt+cnt1];
                                sign      = LocToGloBndSign[cnt+cnt1];
                                Vmath::Svtvp(nbndry,sign*BndSol[id],
                                             BndSys.GetRawPtr()+cnt1*nbndry,1,
                                             &vout[0],1,&vout[0],1);
                                cnt1++;
                            } 
                        }
                        else
                        {
                            cnt1 += e_ncoeffs;
                        }
                    }
                    
                    // Subtract vout from forcing terms 
                    for(i = 0; i < nbndry; ++i)
                    {
                        id   = LocToGloBndMap [cnt+i];
                        sign = LocToGloBndSign[cnt+i];

                        if(id >= NumDirichlet)
                        {
                            BndSol[id] -= sign*vout[i];
                        }
                    }
                }
                cnt += nbndry;
            }

            //----------------------------------
            // Solve trace problem
            //----------------------------------
            if(GloBndDofs - NumDirichlet > 0)
            {
                GlobalLinSysKey       key(StdRegions::eHybridDGHelmBndLam, 
                                          m_traceMap,
                                          lambda,tau,eDirectFullMatrix);
                GlobalLinSysSharedPtr LinSys = GetGlobalBndLinSys(key);
                
                Array<OneD,NekDouble> sln = BndSol+NumDirichlet;
                
                LinSys->Solve(sln,sln);
            }
            
            //----------------------------------
            // Internal element solves
            //----------------------------------
            GlobalMatrixKey invHDGhelmkey(StdRegions::eInvHybridDGHelmholtz,lambda,tau);
            const DNekScalBlkMatSharedPtr& InvHDGHelm = GetBlockMatrix(invHDGhelmkey);
            DNekVec out(m_ncoeffs,outarray,eWrapper);            
            Vmath::Zero(m_ncoeffs,outarray,1);
        
            // get local trace solution from BndSol
            m_traceMap->GlobalToLocalBnd(BndSol,loc_lambda);

            //  out =  u_f + u_lam = (*InvHDGHelm)*f + (LamtoU)*Lam  
            out = (*InvHDGHelm)*F + (*HDGLamToU)*LocLambda;       
        }

        // Construct the two trace vectors of the inner and outer
        // trace solution from the field contained in m_phys, where
        // the Weak dirichlet boundary conditions are listed in the
        // outer part of the vecotr

        void DisContField2D::GetFwdBwdTracePhys(Array<OneD,NekDouble> &Fwd, 
                                                Array<OneD,NekDouble> &Bwd)
        {
            GetFwdBwdTracePhys(m_phys,Fwd,Bwd);
        }

        void DisContField2D::GetFwdBwdTracePhys(const Array<OneD,const NekDouble>  &field, 
                                                Array<OneD,NekDouble> &Fwd, 
                                                Array<OneD,NekDouble> &Bwd)
        {
            // Loop over elements and collect forward expansion
            int nexp = GetExpSize();
            StdRegions::EdgeOrientation edgedir;
            int nquad_e,cnt,n,e,npts,offset, phys_offset;
            Array<OneD,NekDouble> e_tmp, e_tmp1;

            Array<OneD, Array<OneD, StdRegions::StdExpansion1DSharedPtr> >
                elmtToTrace = m_traceMap->GetElmtToTrace();

            // zero vectors; 
            Vmath::Zero(Fwd.num_elements(),Fwd,1);
            Vmath::Zero(Bwd.num_elements(),Bwd,1);

            for(n  = 0; n < nexp; ++n)
            {
                phys_offset = GetPhys_Offset(n);

                for(e = 0; e < (*m_exp)[n]->GetNedges(); ++e)
                {
                    nquad_e = (*m_exp)[n]->GetEdgeNumPoints(e);
                    edgedir = (*m_exp)[n]->GetEorient(e);
                    if(edgedir == StdRegions::eForwards)
                    {
                        offset = m_trace->GetPhys_Offset(elmtToTrace[n][e]->GetElmtId());
                        (*m_exp)[n]->GetEdgePhysVals(e, elmtToTrace[n][e], 
                                                     e_tmp = field + phys_offset, 
                                                     e_tmp1 = Fwd + offset);
                    }
                }
            }
            
            for(n  = 0; n < nexp; ++n)
            {
                phys_offset = GetPhys_Offset(n);

                for(e = 0; e < (*m_exp)[n]->GetNedges(); ++e)
                {
                    nquad_e = (*m_exp)[n]->GetEdgeNumPoints(e);
                    edgedir = (*m_exp)[n]->GetEorient(e);
                    if(edgedir == StdRegions::eBackwards)
                    {
                        offset = m_trace->GetPhys_Offset(elmtToTrace[n][e]->GetElmtId());
                        (*m_exp)[n]->GetEdgePhysVals(e, elmtToTrace[n][e],
                                                     e_tmp = field + phys_offset,
                                                     e_tmp1 = Bwd+offset);
                    }
                }
            }
            

            // fill boundary conditions into missing elements            
            int id1,id2 = 0;
            for(cnt = n = 0; n < m_bndCondExpansions.num_elements(); ++n)
            {
                
                if(m_bndConditions[n]->GetBoundaryConditionType() == SpatialDomains::eDirichlet)
                {
                    
                    for(e = 0; e < m_bndCondExpansions[n]->GetExpSize(); ++e)
                    {
                        npts = m_bndCondExpansions[n]->GetExp(e)->GetNumPoints(0);
                        
                        if(m_traceMap->GetBndExpAdjacentOrient(cnt+e) == eAdjacentEdgeIsForwards)
                        {
                            id1 = m_bndCondExpansions[n]->GetPhys_Offset(e) ;
                            Vmath::Vcopy(npts,&(m_bndCondExpansions[n]->GetPhys())[id1],1,&Bwd[id2],1);
                            id2 += npts; 
                        }
                        else
                        {
                            id1 = m_bndCondExpansions[n]->GetPhys_Offset(e) ;
                            Vmath::Vcopy(npts,&(m_bndCondExpansions[n]->GetPhys())[id1],1,&Fwd[id2],1);
                            id2 += npts; 
                        }
                    }

                    cnt +=e;
                }
                else
                {
                    ASSERTL0(false,"method not set up for non-Dirichlet conditions");
                }
            }
            
        }
        
        void DisContField2D::ExtractTracePhys(Array<OneD,NekDouble> &outarray)
        {       

            ASSERTL1(m_physState == true,
                     "local physical space is not true ");

            ExtractTracePhys(m_phys, outarray);
        }

        void DisContField2D::ExtractTracePhys(const Array<OneD, const NekDouble> &inarray, Array<OneD,NekDouble> &outarray)
        {
            // Loop over elemente and collect forward expansion
            int nexp = GetExpSize();
            int nquad_e,cnt,n,e,npts,offset,phys_offset;
            Array<OneD,NekDouble> e_tmp,e_tmp1;
            Array<OneD, Array<OneD, StdRegions::StdExpansion1DSharedPtr> >
                elmtToTrace = m_traceMap->GetElmtToTrace();

            ASSERTL1(outarray.num_elements() >= m_trace->GetNpoints(),
                     "input array is of insufficient length");

            // use m_trace tmp space in element to fill values
            for(n  = 0; n < nexp; ++n)
            {
                phys_offset = GetPhys_Offset(n);

                for(e = 0; e < (*m_exp)[n]->GetNedges(); ++e)
                {
                    nquad_e = (*m_exp)[n]->GetEdgeNumPoints(e);
                    offset = m_trace->GetPhys_Offset(elmtToTrace[n][e]->GetElmtId());
                    (*m_exp)[n]->GetEdgePhysVals(e,  elmtToTrace[n][e], 
                                                 e_tmp  = inarray + phys_offset,
                                                 e_tmp1 = outarray + offset);
                }
            }
        }
            

        /// Note this routine changes m_trace->m_coeffs space; 
        void DisContField2D::AddTraceIntegral(const Array<OneD, const NekDouble> &Fx, 
                                              const Array<OneD, const NekDouble> &Fy, 
                                              Array<OneD, NekDouble> &outarray)
        {
            int e,n,offset, t_offset;
            Array<OneD, NekDouble> e_outarray;
            Array<OneD, Array<OneD, StdRegions::StdExpansion1DSharedPtr> >
                elmtToTrace = m_traceMap->GetElmtToTrace();

            for(n = 0; n < GetExpSize(); ++n)
            {
                offset = GetCoeff_Offset(n);
                for(e = 0; e < (*m_exp)[n]->GetNedges(); ++e)
                {
                    t_offset = GetTrace()->GetPhys_Offset(elmtToTrace[n][e]->GetElmtId());

                    (*m_exp)[n]->AddEdgeNormBoundaryInt(e,elmtToTrace[n][e],
                                                        Fx + t_offset,
                                                        Fy + t_offset,
                                                        e_outarray = outarray+offset);
                }    
            }
        }

        /// Note this routine changes m_trace->m_coeffs space; 
        void DisContField2D::AddTraceIntegral(const Array<OneD, const NekDouble> &Fn, Array<OneD, NekDouble> &outarray)
        {
            int e,n,offset, t_offset;
            Array<OneD, NekDouble> e_outarray;
            Array<OneD, Array<OneD, StdRegions::StdExpansion1DSharedPtr> >
                elmtToTrace = m_traceMap->GetElmtToTrace();

            for(n = 0; n < GetExpSize(); ++n)
            {
                offset = GetCoeff_Offset(n);
                for(e = 0; e < (*m_exp)[n]->GetNedges(); ++e)
                {
                    t_offset = GetTrace()->GetPhys_Offset(elmtToTrace[n][e]->GetElmtId());

                    (*m_exp)[n]->AddEdgeNormBoundaryInt(e,elmtToTrace[n][e],
                                                        Fn + t_offset,
                                                        e_outarray = outarray+offset);
                }    
            }
        }

    } // end of namespace
} //end of namespace
