/////////////////////////////////////////////////////////////////////////////
//
// File UnsteadyAdvection.cpp
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
// Description: Unsteady linear advection solve routines
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <ADRSolver/EquationSystems/UnsteadyAdvection.h>

namespace Nektar
{
    string UnsteadyAdvection::className = GetEquationSystemFactory().
        RegisterCreatorFunction("UnsteadyAdvection",
                                UnsteadyAdvection::create,
                                "Unsteady Advection equation.");

    UnsteadyAdvection::UnsteadyAdvection(
            const LibUtilities::SessionReaderSharedPtr& pSession)
        : UnsteadySystem(pSession)
    {
        m_planeNumber = 0;
    }

    /**
     * @brief Initialisation object for the unsteady linear advection equation.
     */
    void UnsteadyAdvection::v_InitObject()
    {
        // Call to the initialisation object of UnsteadySystem
        UnsteadySystem::v_InitObject();
        
        m_session->LoadParameter("wavefreq",   m_waveFreq, 0.0);
        // Read the advection velocities from session file
        
        std::vector<std::string> vel;
        vel.push_back("Vx");
        vel.push_back("Vy");
        vel.push_back("Vz");
        
        // Resize the advection velocities vector to dimension of the problem
        vel.resize(m_spacedim);

        // Store in the global variable m_velocity the advection velocities
        m_velocity = Array<OneD, Array<OneD, NekDouble> >(m_spacedim);
        EvaluateFunction(vel, m_velocity, "AdvectionVelocity");

        // Type of advection class to be used
        switch(m_projectionType)
        {
            // Continuous field 
            case MultiRegions::eGalerkin:
            {
                string advName;
                m_session->LoadSolverInfo(
                    "AdvectionType", advName, "NonConservative");
                m_advection = SolverUtils::
                    GetAdvectionFactory().CreateInstance(advName, advName);
                if (m_specHP_dealiasing)
                {
                    m_advection->SetFluxVector(
                        &UnsteadyAdvection::GetFluxVectorDeAlias, this);
                }
                else 
                {
                    m_advection->SetFluxVector(
                        &UnsteadyAdvection::GetFluxVector, this);
                }
                break;
            }
            // Discontinuous field 
            case MultiRegions::eDiscontinuous:
            {
                // Define the normal velocity fields
                if (m_fields[0]->GetTrace())
                {
                    m_traceVn  = Array<OneD, NekDouble>(GetTraceNpoints());
                }
                
                string advName;
                string riemName;
                m_session->LoadSolverInfo(
                    "AdvectionType", advName, "WeakDG");
                m_advection = SolverUtils::
                    GetAdvectionFactory().CreateInstance(advName, advName);
                if (m_specHP_dealiasing)
                {
                    m_advection->SetFluxVector(
                        &UnsteadyAdvection::GetFluxVectorDeAlias, this);

                }
                else 
                {
                    m_advection->SetFluxVector(
                        &UnsteadyAdvection::GetFluxVector, this);

                }
                m_session->LoadSolverInfo(
                    "UpwindType", riemName, "Upwind");
                m_riemannSolver = SolverUtils::
                    GetRiemannSolverFactory().CreateInstance(riemName);
                m_riemannSolver->AddScalar(
                    "Vn", &UnsteadyAdvection::GetNormalVelocity, this);
                
                m_advection->SetRiemannSolver(m_riemannSolver);
                m_advection->InitObject(m_session, m_fields);
                break;
            }
            default:
            {
                ASSERTL0(false, "Unsupported projection type.");
                break;
            }
        }
        
        // If explicit it computes RHS and PROJECTION for the time integration
        if (m_explicitAdvection)
        {
            m_ode.DefineOdeRhs     (&UnsteadyAdvection::DoOdeRhs,        this);
            m_ode.DefineProjection (&UnsteadyAdvection::DoOdeProjection, this);
        }
        // Otherwise it gives an error (no implicit integration)
        else
        {
            ASSERTL0(false, "Implicit unsteady Advection not set up.");
        }
    }

    /**
     * @brief Unsteady linear advection equation destructor.
     */
    UnsteadyAdvection::~UnsteadyAdvection()
    {
    }

    /**
     * @brief Get the normal velocity for the linear advection equation.
     */
    Array<OneD, NekDouble> &UnsteadyAdvection::GetNormalVelocity()
    {
        // Number of trace (interface) points
        int i;
        int nTracePts = GetTraceNpoints();

        // Auxiliary variable to compute the normal velocity
        Array<OneD, NekDouble> tmp(nTracePts);

        // Reset the normal velocity
        Vmath::Zero(nTracePts, m_traceVn, 1);
    
        // Compute the normal velocity
        // For 3DHomogenoeus1D
        if (m_expdim == 2 && m_HomogeneousType == eHomogeneous1D)
        {
            int nPointsTot       = m_fields[0]->GetTotPoints();
            int nPointsTot_plane = m_fields[0]->GetPlane(0)->GetTotPoints();
            int n_planes         = nPointsTot/nPointsTot_plane;
            
            Array<OneD, Array<OneD, NekDouble> >
                advVel_plane(m_velocity.num_elements());
            
            for (i = 0; i < m_velocity.num_elements(); ++i)
            {
                advVel_plane[i] = Array<OneD, NekDouble>(
                    nPointsTot_plane,
                    m_velocity[i] + m_planeNumber*nPointsTot_plane);
            }

            for (i = 0; i < m_velocity.num_elements(); ++i)
            {
                m_fields[0]->GetPlane(m_planeNumber)->ExtractTracePhys(
                    advVel_plane[i], tmp);
                
                Vmath::Vvtvp(nTracePts,
                             m_traceNormals[i], 1,
                             tmp, 1,
                             m_traceVn, 1,
                             m_traceVn, 1);
            }

            m_planeNumber = (m_planeNumber + 1) % n_planes;
        }
        else  // For general case
        {
            for (i = 0; i < m_velocity.num_elements(); ++i)
            {
                m_fields[0]->ExtractTracePhys(m_velocity[i], tmp);
            
                Vmath::Vvtvp(nTracePts,
                             m_traceNormals[i], 1,
                             tmp, 1,
                             m_traceVn, 1,
                             m_traceVn, 1);
            }
        }
        
        return m_traceVn;
    }
    
    /**
     * @brief Compute the right-hand side for the linear advection equation.
     * 
     * @param inarray    Given fields.
     * @param outarray   Calculated solution.
     * @param time       Time.
     */
    void UnsteadyAdvection::DoOdeRhs(
        const Array<OneD, const  Array<OneD, NekDouble> >&inarray,
              Array<OneD,        Array<OneD, NekDouble> >&outarray,
        const NekDouble time)
    {
        // Counter variable
        int i;
        
        // Number of fields (variables of the problem)
        int nVariables = inarray.num_elements();
        
        // Number of solution points
        int nSolutionPts = GetNpoints();
        
        // RHS computation using the new advection base class
        m_advection->Advect(nVariables, m_fields, m_velocity, inarray, 
                            outarray);
        
        // Negate the RHS
        for (i = 0; i < nVariables; ++i)
        {
            Vmath::Neg(nSolutionPts, outarray[i], 1);
        }
    }

    /**
     * @brief Compute the projection for the linear advection equation.
     * 
     * @param inarray    Given fields.
     * @param outarray   Calculated solution.
     * @param time       Time.
     */
    void UnsteadyAdvection::DoOdeProjection(
        const Array<OneD, const Array<OneD, NekDouble> >&inarray,
              Array<OneD,       Array<OneD, NekDouble> >&outarray,
        const NekDouble time)
    {
        // Counter variable
        int i;
        
        // Number of fields (variables of the problem)
        int nVariables = inarray.num_elements();
        
        // Set the boundary conditions
        SetBoundaryConditions(time);

        // Switch on the projection type (Discontinuous or Continuous)
        switch(m_projectionType)
        {
            // Discontinuous projection
            case MultiRegions::eDiscontinuous:
            {
                // Number of quadrature points
                int nQuadraturePts = GetNpoints();

                // Just copy over array
                for(i = 0; i < nVariables; ++i)
                {
                    Vmath::Vcopy(nQuadraturePts, inarray[i], 1, outarray[i], 1);
                }
                break;
            }
            
        // Continuous projection
        case MultiRegions::eGalerkin:
        case MultiRegions::eMixed_CG_Discontinuous:
            {
                Array<OneD, NekDouble> coeffs(m_fields[0]->GetNcoeffs(),0.0);
                for(i = 0; i < nVariables; ++i)
                {
                    m_fields[i]->FwdTrans(inarray[i], coeffs);
                    m_fields[i]->BwdTrans_IterPerExp(coeffs, outarray[i]);
                }
                break;
            }
                
            default:
                ASSERTL0(false,"Unknown projection scheme");
                break;
        }
    }
    
    /**
     * @brief Return the flux vector for the linear advection equation.
     * 
     * @param i           Component of the flux vector to calculate.
     * @param physfield   Fields.
     * @param flux        Resulting flux.
     */
    void UnsteadyAdvection::GetFluxVector(
        const Array<OneD, Array<OneD, NekDouble> >               &physfield,
              Array<OneD, Array<OneD, Array<OneD, NekDouble> > > &flux)
    {
        ASSERTL1(flux[0].num_elements() == m_velocity.num_elements(),
                 "Dimension of flux array and velocity array do not match");

        int i , j;
        int nq = physfield[0].num_elements();
        
        // For 3DHomogenoeus1D
        /*
        if (m_expdim == 2 && m_HomogeneousType == eHomogeneous1D)
        {
            Array<OneD, Array<OneD, NekDouble> >
                advVel_plane(m_velocity.num_elements());
            
            int nPointsTot = m_fields[0]->GetTotPoints();
            int nPointsTot_plane = m_fields[0]->GetPlane(0)->GetTotPoints();
            int n_planes = nPointsTot/nPointsTot_plane;
            
            for (int i = 0; i < m_velocity.num_elements(); ++i)
            {
                advVel_plane[i] = Array<OneD, NekDouble>(nPointsTot_plane, 0.0);
            
                Vmath::Vcopy(nPointsTot_plane,
                            &m_velocity[i][m_planeNumber*nPointsTot_plane], 1,
                            &advVel_plane[i][0], 1);
            }
            
            for (int i = 0; i < flux.num_elements(); ++i)
            {
                for (j = 0; j < flux[0].num_elements(); ++j)
                {
                    Vmath::Vmul(nq, physfield[i], 1, advVel_plane[j], 1,
                                flux[i][j], 1);
                }
            }
        }
        else // For general case
        {
        */
            for (i = 0; i < flux.num_elements(); ++i)
            {
                for (j = 0; j < flux[0].num_elements(); ++j)
                {
                    Vmath::Vmul(nq, physfield[i], 1, m_velocity[j], 1,
                            flux[i][j], 1);
                }
            }
            //}
    }
    
    /**
     * @brief Return the flux vector for the linear advection equation using
     * the dealiasing technique.
     * 
     * @param i           Component of the flux vector to calculate.
     * @param physfield   Fields.
     * @param flux        Resulting flux.
     */
    void UnsteadyAdvection::GetFluxVectorDeAlias(
        const Array<OneD, Array<OneD, NekDouble> >               &physfield,
              Array<OneD, Array<OneD, Array<OneD, NekDouble> > > &flux)
    {
        ASSERTL1(flux[0].num_elements() == m_velocity.num_elements(),
                 "Dimension of flux array and velocity array do not match");
        
        int i, j;
        int nq = physfield[0].num_elements();
        int nVariables = physfield.num_elements();
        
        // Factor to rescale 1d points in dealiasing
        NekDouble OneDptscale = 2;
        
        Array<OneD, Array<OneD, NekDouble> >
            advVel_plane(m_velocity.num_elements());
        
        // Get number of points to dealias a cubic non-linearity
        // For 3DHomogenoeus1D
        if (m_expdim == 2 && m_HomogeneousType == eHomogeneous1D)
        {
            int nPointsTot = m_fields[0]->GetTotPoints();
            int nPointsTot_plane = m_fields[0]->GetPlane(0)->GetTotPoints();
            int n_planes = nPointsTot/nPointsTot_plane;
            
            nq = m_fields[0]->GetPlane(0)->Get1DScaledTotPoints(OneDptscale);
        
            for (i = 0; i < m_velocity.num_elements(); ++i)
            {
                advVel_plane[i] = Array<OneD, NekDouble>(nPointsTot_plane, 0.0);
                
                Vmath::Vcopy(nPointsTot_plane,
                             &m_velocity[i][m_planeNumber*nPointsTot_plane], 1,
                             &advVel_plane[i][0], 1);
            }
            
        }
        else // For general case
        {
            nq = m_fields[0]->Get1DScaledTotPoints(OneDptscale);
        }
        
        // Initialisation of higher-space variables
        Array<OneD, Array<OneD, NekDouble> >physfieldInterp(nVariables);
        Array<OneD, Array<OneD, NekDouble> >velocityInterp(m_expdim);
        Array<OneD, Array<OneD, Array<OneD, NekDouble> > >fluxInterp(nVariables);
        
        // Interpolation to higher space of physfield
        for (i = 0; i < nVariables; ++i)
        {
            physfieldInterp[i] = Array<OneD, NekDouble>(nq);
            fluxInterp[i] = Array<OneD, Array<OneD, NekDouble> >(m_expdim);
            for (j = 0; j < m_expdim; ++j)
            {
                fluxInterp[i][j] = Array<OneD, NekDouble>(nq);
            }
            
            // For 3DHomogenoeus1D
            if (m_expdim == 2 && m_HomogeneousType == eHomogeneous1D)
            {
                m_fields[0]->GetPlane(0)->PhysInterp1DScaled(
                    OneDptscale, physfield[i], physfieldInterp[i]);
            }
            else // For general case
            {
                m_fields[0]->PhysInterp1DScaled(
                    OneDptscale, physfield[i], physfieldInterp[i]);
            }
        }
        
        // Interpolation to higher space of velocity
        for (j = 0; j < m_expdim; ++j)
        {
            velocityInterp[j] = Array<OneD, NekDouble>(nq);
            
            // For 3DHomogenoeus1D
            if (m_expdim == 2 && m_HomogeneousType == eHomogeneous1D)
            {
                m_fields[0]->GetPlane(0)->PhysInterp1DScaled(
                    OneDptscale, advVel_plane[j], velocityInterp[j]);
            }
            else // For general case
            {
                m_fields[0]->PhysInterp1DScaled(
                    OneDptscale, m_velocity[j], velocityInterp[j]);
            }
        }
        
        // Evaluation of flux vector in the higher space
        // For 3DHomogenoeus1D
        if (m_expdim == 2 && m_HomogeneousType == eHomogeneous1D)
        {
            // Evaluation of flux vector in the higher space
            for (i = 0; i < flux.num_elements(); ++i)
            {
                for (j = 0; j < m_expdim; ++j)
                {
                    Vmath::Vmul(nq, physfieldInterp[i], 1, velocityInterp[j], 1,
                                fluxInterp[i][j], 1);
                }
            }
            
            // Galerkin project solution back to original space
            for (i = 0; i < nVariables; ++i)
            {
                for (j = 0; j < m_expdim; ++j)
                {
                    m_fields[0]->GetPlane(0)->PhysGalerkinProjection1DScaled(
                        OneDptscale, fluxInterp[i][j], flux[i][j]);
                }
                
                Vmath::Vmul(physfield[i].num_elements(),
                            physfield[i], 1,
                            m_velocity[2], 1,
                            flux[i][2], 1);
            }

        }
        else // For general case
        {
            // Evaluation of flux vector in the higher space
            for (i = 0; i < flux.num_elements(); ++i)
            {
                for (j = 0; j < flux[0].num_elements(); ++j)
                {
                    Vmath::Vmul(nq, physfieldInterp[i], 1, velocityInterp[j], 1,
                                fluxInterp[i][j], 1);
                }
            }
            
            // Galerkin project solution back to original space
            for (i = 0; i < nVariables; ++i)
            {
                for (j = 0; j < m_spacedim; ++j)
                {
                    m_fields[0]->PhysGalerkinProjection1DScaled(
                        OneDptscale, fluxInterp[i][j], flux[i][j]);
                }
            }
        }
    }

    void UnsteadyAdvection::v_PrintSummary(std::ostream &out)
    {
        UnsteadySystem::v_PrintSummary(out);
    }
}
