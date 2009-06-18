////////////////////////////////////////////////////////////////////////////////
//
//  File:  $Source: /usr/sci/projects/Nektar/cvs/Nektar++/library/SpatialDomains/MeshGraph.cpp,v $
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
//  Description:  
//
//
////////////////////////////////////////////////////////////////////////////////
#include "pchSpatialDomains.h"

#include <boost/foreach.hpp>

#include <SpatialDomains/MeshGraph.h>
#include <SpatialDomains/ParseUtils.hpp>

// Use the stl version, primarily for string.
#ifndef TIXML_USE_STL
#define TIXML_USE_STL
#endif

#include <tinyxml/tinyxml.h>
#include <cstring>
#include <sstream>
#include <cmath>

#include <SpatialDomains/MeshGraph1D.h>
#include <SpatialDomains/MeshGraph2D.h>
#include <SpatialDomains/MeshGraph3D.h>

// These are required for the Write(...) and Import(...) functions.
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp> 
#include <boost/iostreams/filtering_stream.hpp>

namespace Nektar
{
    namespace SpatialDomains
    {

        MeshGraph::MeshGraph():
            m_MeshDimension(3),
            m_SpaceDimension(3)
        {
        }

        boost::shared_ptr<MeshGraph> MeshGraph::Read(std::string &infilename)
        {
            boost::shared_ptr<MeshGraph> returnval;
            
            MeshGraph mesh;
            
            mesh.ReadGeometry(infilename);
            int meshDim = mesh.GetMeshDimension();
            
            switch(meshDim)
            {
            case 1:
                returnval = MemoryManager<MeshGraph1D>::AllocateSharedPtr();
                break;
                
            case 2:
                returnval = MemoryManager<MeshGraph2D>::AllocateSharedPtr();
                break;
                
            case 3:
                returnval = MemoryManager<MeshGraph3D>::AllocateSharedPtr();
                break;
                
            default:
                std::string err = "Invalid mesh dimension: ";
                std::stringstream strstrm;
                strstrm << meshDim;
                err += strstrm.str();
                NEKERROR(ErrorUtil::efatal, err.c_str());
            }
            
            if (returnval)
            {
                returnval->ReadGeometry(infilename);
                returnval->ReadExpansions(infilename);
            }

            return returnval;
        }


        void MeshGraph::SetExpansions(std::vector<SpatialDomains::FieldDefinitionsSharedPtr> &fielddef, std::vector< std::vector<LibUtilities::PointsType> > &pointstype)
        {
            int i,j,k,cnt,id;
            int num_elmts = 0;
            GeometrySharedPtr geom;
            
            // Set up list of ExpansionVectors with dummy values
            if(!m_ExpansionVector.size())
            {
                
                LibUtilities::BasisKeyVector def; 

                for(i = 0; i < fielddef.size(); ++i)
                {
                    num_elmts += fielddef[i]->m_ElementIDs.size();        

                    for(j = 0; j < fielddef[i]->m_ElementIDs.size(); ++j)
                    {
                        ExpansionShPtr tmpexp =
                            MemoryManager<Expansion>::AllocateSharedPtr(geom, def);
                        m_ExpansionVector.push_back(tmpexp);
                    }
                }
            }
            else
            {

                for(i = 0; i < fielddef.size(); ++i)
                {
                    num_elmts += fielddef[i]->m_ElementIDs.size();        
                }
                
                ASSERTL0(m_ExpansionVector.size() == num_elmts,"Existing graph size does not match new Expansion length");
            }
            
            
            // loop over all elements find the geometry shared ptr and
            // set up basiskey vector
            for(cnt = i = 0; i < fielddef.size(); ++i)
            {
                std::vector<unsigned int> nmodes = fielddef[i]->m_NumModes;
                std::vector<LibUtilities::BasisType> basis = fielddef[i]->m_Basis;
                bool UniOrder =  fielddef[i]->m_UniOrder;
                
                for(j = 0; j < fielddef[i]->m_ElementIDs.size(); ++j)
                {
                   
                    LibUtilities::BasisKeyVector bkeyvec;
                    id = fielddef[i]->m_ElementIDs[j];
                    
                    switch(fielddef[i]->m_ShapeType)
                    {
                    case eSegment:
                        {
                            for(k = 0; k < m_seggeoms.size();++k)
                            {
                                if(m_seggeoms[k]->GetGlobalID() == fielddef[i]->m_ElementIDs[j])
                                {
                                    geom = m_seggeoms[k];
                                    break;
                                }
                            }
                            ASSERTL0(k != m_seggeoms.size(),"Failed to find geometry with same global id");
                            
                            const LibUtilities::PointsKey pkey(nmodes[cnt],pointstype[i][0]);
                            LibUtilities::BasisKey bkey(basis[0],nmodes[cnt],pkey);
                            if(!UniOrder)
                            {
                                cnt++;
                            }
                            bkeyvec.push_back(bkey);
                        }
                        break;
                    case eTriangle:
                        {
                            for(k = 0; k < m_trigeoms.size();++k)
                            {
                                if(m_trigeoms[k]->GetGlobalID() == fielddef[i]->m_ElementIDs[j])
                                {
                                    geom = m_trigeoms[k];
                                    break;
                                }
                        }
                            ASSERTL0(k != m_trigeoms.size(),"Failed to find geometry with same global id");
                            for(int b = 0; b < 2; ++b)
                            {
                                const LibUtilities::PointsKey pkey(nmodes[cnt+b],pointstype[i][b]);
                                LibUtilities::BasisKey bkey(basis[b],nmodes[cnt+b],pkey);
                                bkeyvec.push_back(bkey);
                            }
                            
                            if(!UniOrder)
                            {
                                cnt += 2;
                            }
                        }
                        break;
                    case eQuadrilateral:
                        {
                            for(k = 0; k < m_quadgeoms.size();++k)
                            {
                                if(m_quadgeoms[k]->GetGlobalID() == fielddef[i]->m_ElementIDs[j])
                                {
                                    geom = m_quadgeoms[k];
                                    break;
                                }
                            }
                            ASSERTL0(k != m_quadgeoms.size(),"Failed to find geometry with same global id");
                            
                            for(int b = 0; b < 2; ++b)
                            {
                                const LibUtilities::PointsKey pkey(nmodes[cnt+b],pointstype[i][b]);
                                LibUtilities::BasisKey bkey(basis[b],nmodes[cnt+b],pkey);
                                bkeyvec.push_back(bkey);
                            }
                            
                            if(!UniOrder)
                            {
                                cnt += 2;
                            }
                        }
                        break;
                    default:
                        ASSERTL0(false,"Need to set up for 3D Expansions");
                        break;                        
                    }
                    
                    m_ExpansionVector[id]->m_GeomShPtr = geom;
                    m_ExpansionVector[id]->m_BasisKeyVector = bkeyvec;
                }
            }
            
        }
        
        
    // \brief Read will read the meshgraph vertices given a filename.
    void MeshGraph::ReadGeometry(std::string &infilename)
    {
        TiXmlDocument doc(infilename);
        bool loadOkay = doc.LoadFile();
        
        std::string errstr = "Unable to load file: ";
        errstr += infilename;
        ASSERTL0(loadOkay, errstr.c_str());

        ReadGeometry(doc);
    }

    // \brief Read will read the meshgraph vertices given a TiXmlDocument.
    void MeshGraph::ReadGeometry(TiXmlDocument &doc)
    {
        TiXmlHandle docHandle(&doc);
        TiXmlNode* node = NULL;
        TiXmlElement* mesh = NULL;
        TiXmlElement* master = NULL;    // Master tag within which all data is contained.

        int err;    /// Error value returned by TinyXML.

        master = doc.FirstChildElement("NEKTAR");
        ASSERTL0(master, "Unable to find NEKTAR tag in file.");

        // Find the Mesh tag and same the dim and space attributes
        mesh = master->FirstChildElement("GEOMETRY");

        ASSERTL0(mesh, "Unable to find GEOMETRY tag in file.");
        TiXmlAttribute *attr = mesh->FirstAttribute();

        // Initialize the mesh and space dimensions to 3 dimensions.
        // We want to do this each time we read a file, so it should
        // be done here and not just during class initialization.
        m_MeshDimension = 3;
        m_SpaceDimension = 3;

        while (attr)
        {
            std::string attrName(attr->Name());
            if (attrName == "DIM")
            {
                err = attr->QueryIntValue(&m_MeshDimension);
                ASSERTL1(err==TIXML_SUCCESS, "Unable to read mesh dimension.");
            }
            else if (attrName == "SPACE")
            {
                err = attr->QueryIntValue(&m_SpaceDimension);
                ASSERTL1(err==TIXML_SUCCESS, "Unable to read space dimension.");
            }
            else
            {
                std::string errstr("Unknown attribute: ");
                errstr += attrName;
                ASSERTL1(false, errstr.c_str());
            }

            // Get the next attribute.
            attr = attr->Next();
        }

        ASSERTL1(m_MeshDimension<=m_SpaceDimension, "Mesh dimension greater than space dimension");

        // Now read the vertices
        TiXmlElement* element = mesh->FirstChildElement("VERTEX");
        ASSERTL0(element, "Unable to find mesh VERTEX tag in file.");

        TiXmlElement *vertex = element->FirstChildElement("V");

        int indx;
        int nextVertexNumber = -1;

        while (vertex)
        {
            nextVertexNumber++;

            TiXmlAttribute *vertexAttr = vertex->FirstAttribute();
            std::string attrName(vertexAttr->Name());

            ASSERTL0(attrName == "ID", (std::string("Unknown attribute name: ") + attrName).c_str());

            err = vertexAttr->QueryIntValue(&indx);
            ASSERTL0(err == TIXML_SUCCESS, "Unable to read attribute ID.");
            ASSERTL0(indx == nextVertexNumber, "Element IDs must begin with zero and be sequential.");

            // Now read body of vertex
            std::string vertexBodyStr;

            TiXmlNode *vertexBody = vertex->FirstChild();

            while (vertexBody)
            {
                // Accumulate all non-comment body data.
                if (vertexBody->Type() == TiXmlNode::TEXT)
                {
                    vertexBodyStr += vertexBody->ToText()->Value();
                    vertexBodyStr += " ";
                }

                vertexBody = vertexBody->NextSibling();
            }

            ASSERTL0(!vertexBodyStr.empty(), "Vertex definitions must contain vertex data.");

            // Get vertex data from the data string.
            double xval, yval, zval;
            std::istringstream vertexDataStrm(vertexBodyStr.c_str());

            try
            {
                while(!vertexDataStrm.fail())
                {
                    vertexDataStrm >> xval >> yval >> zval;

                    // Need to check it here because we may not be good after the read
                    // indicating that there was nothing to read.
                    if (!vertexDataStrm.fail())
                    {
                        VertexComponentSharedPtr vert(MemoryManager<VertexComponent>::AllocateSharedPtr(m_MeshDimension, indx, xval, yval, zval));
                        m_vertset.push_back(vert);
                    }
                }
            }
            catch(...)
            {
                ASSERTL0(false, "Unable to read VERTEX data.");
            }

            vertex = vertex->NextSiblingElement("V");
        }
    }
        
        // \brief Read the expansions given the XML file path.
        void MeshGraph::ReadExpansions(std::string &infilename)
        {
            TiXmlDocument doc(infilename);
            bool loadOkay = doc.LoadFile();
            
            std::string errstr = "Unable to load file: ";
            errstr += infilename;
            ASSERTL0(loadOkay, errstr.c_str());
            
            ReadExpansions(doc);
        }
        
        LibUtilities::BasisKeyVector DefineBasisKeyFromExpansionType(GeometrySharedPtr in, 
                                                                     ExpansionType type,
                                                                     const int order)
        {
            LibUtilities::BasisKeyVector returnval;

            GeomShapeType shape= in->GetGeomShapeType();
            
            switch(type)
            {
            case eModified:
                switch (shape)
                {
                case eSegment:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eQuadrilateral:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eHexahedron:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eTriangle:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        
                        const LibUtilities::PointsKey pkey1(order,LibUtilities::eGaussRadauMAlpha1Beta0);
                        LibUtilities::BasisKey bkey1(LibUtilities::eModified_B,order,pkey1);
                        
                        returnval.push_back(bkey1);
                    }
                    break;
                case eTetrahedron:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        
                        const LibUtilities::PointsKey pkey1(order,LibUtilities::eGaussRadauMAlpha1Beta0);
                        LibUtilities::BasisKey bkey1(LibUtilities::eModified_B,order,pkey1);
                        returnval.push_back(bkey1);
                        
                        const LibUtilities::PointsKey pkey2(order,LibUtilities::eGaussRadauMAlpha2Beta0);
                        LibUtilities::BasisKey bkey2(LibUtilities::eModified_C,order,pkey2);
                        returnval.push_back(bkey2);
                    }
                    break;
                default:
                    ASSERTL0(false,"Expansion not defined in switch  for this shape");
                    break;
                }
                break;
		
	     case eModifiedConsistent:
	       switch (shape)
                {
                case eSegment:
                    {
		      const LibUtilities::PointsKey pkey(ceil(3*order/2)+2,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eQuadrilateral:
                    {
                        const LibUtilities::PointsKey pkey(ceil(3*order/2)+2,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eHexahedron:
                    {
                        const LibUtilities::PointsKey pkey(ceil(3*order/2)+2,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eTriangle:
                    {
                        const LibUtilities::PointsKey pkey(ceil(3*order/2)+2,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        
                        const LibUtilities::PointsKey pkey1(ceil(3*order/2)+1,LibUtilities::eGaussRadauMAlpha1Beta0);
                        LibUtilities::BasisKey bkey1(LibUtilities::eModified_B,order,pkey1);
                        
                        returnval.push_back(bkey1);
                    }
                    break;
		default:
                    ASSERTL0(false,"Expansion not defined in switch  for this shape");
                    break;
                }
                break;

	    case eModifiedGaussKronrod:
	      switch (shape)
                {
                case eSegment:
                    {
		       const LibUtilities::PointsKey pkey(2*(order+1)+1,LibUtilities::eGaussLobattoKronrodLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eQuadrilateral:
                    {
		      const LibUtilities::PointsKey pkey(2*(order+1)+1,LibUtilities::eGaussLobattoKronrodLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eHexahedron:
                    {
		      const LibUtilities::PointsKey pkey(2*(order+1)+1,LibUtilities::eGaussLobattoKronrodLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eTriangle:
                    {
		      const LibUtilities::PointsKey pkey(2*(order+1)+1,LibUtilities::eGaussLobattoKronrodLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eModified_A,order,pkey);
                        returnval.push_back(bkey);
                        
                        const LibUtilities::PointsKey pkey1(2*(order),LibUtilities::eGaussRadauKronrodMAlpha1Beta0);
                        LibUtilities::BasisKey bkey1(LibUtilities::eModified_B,order,pkey1);
                        
                        returnval.push_back(bkey1);
                    }
                    break;
		default:
                    ASSERTL0(false,"Expansion not defined in switch  for this shape");
                    break;
                }
                break;
            case eGLL_Lagrange:
                {
                    
                    switch(shape)
                    {
                    case eSegment:
                        {
                            const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                            LibUtilities::BasisKey bkey(LibUtilities::eGLL_Lagrange,order,pkey);
                            returnval.push_back(bkey);
                        }
                        break;
                    case eQuadrilateral: 
                        {
                            const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                            LibUtilities::BasisKey bkey(LibUtilities::eGLL_Lagrange,order,pkey);
                            returnval.push_back(bkey);
                            returnval.push_back(bkey);
                        }
                        break;
                    case eTriangle: // define with corrects points key
                                    // and change to Ortho on
                                    // construction
                        {
                            
                            const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                            LibUtilities::BasisKey bkey(LibUtilities::eGLL_Lagrange,order,pkey);
                            
                            returnval.push_back(bkey);
                            
                            const LibUtilities::PointsKey pkey1(order,LibUtilities::eGaussRadauMAlpha1Beta0);
                            LibUtilities::BasisKey bkey1(LibUtilities::eOrtho_B,order,pkey1);
                            
                            returnval.push_back(bkey1);
                        }
                        break;
                    default:
                        ASSERTL0(false,"Expansion not defined in switch  for this shape");
                        break;
                    }
                }
                break;
            case eOrthogonal:
                switch (shape)
                {
                case eSegment:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eOrtho_A,order,pkey);

                        returnval.push_back(bkey);
                    }
                    break;
                case eTriangle:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eOrtho_A,order,pkey);

                        returnval.push_back(bkey);

                        const LibUtilities::PointsKey pkey1(order,LibUtilities::eGaussRadauMAlpha1Beta0);
                        LibUtilities::BasisKey bkey1(LibUtilities::eOrtho_B,order,pkey1);

                        returnval.push_back(bkey1);
                    }
                    break;
                case eQuadrilateral:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eOrtho_A,order,pkey);

                        returnval.push_back(bkey);
                        returnval.push_back(bkey);
                    }
                    break;
                case eTetrahedron:
                    {
                        const LibUtilities::PointsKey pkey(order+1,LibUtilities::eGaussLobattoLegendre);
                        LibUtilities::BasisKey bkey(LibUtilities::eOrtho_A,order,pkey);

                        returnval.push_back(bkey);

                        const LibUtilities::PointsKey pkey1(order,LibUtilities::eGaussRadauMAlpha1Beta0);
                        LibUtilities::BasisKey bkey1(LibUtilities::eOrtho_B,order,pkey1);

                        returnval.push_back(bkey1);

                        const LibUtilities::PointsKey pkey2(order,LibUtilities::eGaussRadauMAlpha2Beta0);
                        LibUtilities::BasisKey bkey2(LibUtilities::eOrtho_C,order,pkey2);
                    }
                    break;
                default:
                    ASSERTL0(false,"Expansion not defined in switch  for this shape");
                    break;
                }
                break;
            case eGLL_Lagrange_SEM:
                {   
                    switch (shape)
                    {
                    case eSegment:
                        {
                            const LibUtilities::PointsKey pkey(order,LibUtilities::eGaussLobattoLegendre);
                            LibUtilities::BasisKey bkey(LibUtilities::eGLL_Lagrange,order,pkey);
                            
                            returnval.push_back(bkey);
                        }
                        break;
                    case eQuadrilateral:
                        {
                            const LibUtilities::PointsKey pkey(order,LibUtilities::eGaussLobattoLegendre);
                            LibUtilities::BasisKey bkey(LibUtilities::eGLL_Lagrange,order,pkey);
                            
                            returnval.push_back(bkey);
                            returnval.push_back(bkey);
                        }
                        break;
                    case eHexahedron:
                        {
                            const LibUtilities::PointsKey pkey(order,LibUtilities::eGaussLobattoLegendre);
                            LibUtilities::BasisKey bkey(LibUtilities::eGLL_Lagrange,order,pkey);
                            
                            returnval.push_back(bkey);
                            returnval.push_back(bkey);
                            returnval.push_back(bkey);
                        }
                        break;
                    default:
                        ASSERTL0(false,"Expansion not defined in switch  for this shape");
                        break;
                    }
                }
                break;
            default:
                break;
            }
               
            return returnval;
        }

        // \brief Read the expansions given the XML document reference.
        void MeshGraph::ReadExpansions(TiXmlDocument &doc)
        {
            TiXmlElement *master = doc.FirstChildElement("NEKTAR");
            ASSERTL0(master, "Unable to find NEKTAR tag in file.");
            
            // Find the Expansions tag
            TiXmlElement *expansionTypes = master->FirstChildElement("EXPANSIONS");
            ASSERTL0(expansionTypes, "Unable to find EXPANSIONS tag in file.");
            
            if (expansionTypes)
            {
                /// Expansiontypes will contain composite, nummodes, and
                /// expansiontype (eModified, or eOrthogonal)
                /// Or a full list of data of basistype, nummodes, piontstype, numpoints;
                
                // Need a vector of all elements and their associated
                // expansion information.  
                const CompositeVector &domain = this->GetDomain();
                CompositeVector::const_iterator compIter;
                
                for (compIter = domain.begin(); compIter != domain.end(); ++compIter)
                {
                    boost::shared_ptr<GeometryVector> geomVectorShPtr = *compIter;
                    GeometryVectorIter geomIter;
                    for (geomIter = geomVectorShPtr->begin(); geomIter != geomVectorShPtr->end(); ++geomIter)
                    {
                        // Make sure we only have one instance of the
                        // GeometrySharedPtr stored in the list.
                        ExpansionVector::iterator elemIter;
                        for (elemIter = m_ExpansionVector.begin(); elemIter != m_ExpansionVector.end(); ++elemIter)
                        {
                            if ((*elemIter)->m_GeomShPtr == *geomIter)
                            {
                                break;
                            }
                        }
                        
                        // Not found in list.
                        if (elemIter == m_ExpansionVector.end())
                        {
                            LibUtilities::BasisKeyVector def; 
                            ExpansionShPtr expansionElementShPtr =
                                MemoryManager<Expansion>::AllocateSharedPtr(*geomIter, def);
                            m_ExpansionVector.push_back(expansionElementShPtr);
                        }
                    }
                }
                
                // Clear the default linear expansion over the domain.
                TiXmlElement *expansion = expansionTypes->FirstChildElement("E");
                
                while (expansion)
                {
                    /// Mandatory components...optional are to follow later.
                    std::string compositeStr = expansion->Attribute("COMPOSITE");
                    ASSERTL0(compositeStr.length() > 3, "COMPOSITE must be specified in expansion definition");
                    int beg = compositeStr.find_first_of("[");
                    int end = compositeStr.find_first_of("]");
                    std::string compositeListStr = compositeStr.substr(beg+1,end-beg-1);
                    
                    CompositeVector compositeVector;
                    GetCompositeList(compositeListStr, compositeVector);

                    bool          useExpansionType = false;
                    ExpansionType expansion_type;
                    int           expansion_order;
                        
                    LibUtilities::BasisKeyVector basiskeyvec;
                    const char * tStr = expansion->Attribute("TYPE");
                    
                    if(tStr) // use type string to define expansion
                    {
                        std::string typeStr = tStr; 
                        const std::string* begStr = kExpansionTypeStr;
                        const std::string* endStr = kExpansionTypeStr+eExpansionTypeSize;
                        const std::string* expStr = std::find(begStr, endStr, typeStr);
                        
                        ASSERTL0(expStr != endStr, "Invalid expansion type.");
                        expansion_type = (ExpansionType)(expStr - begStr);
                        
                        const char *nStr = expansion->Attribute("NUMMODES");
                        ASSERTL0(nStr,"NUMMODES was not defined in EXPANSION section of input");                     
                        std::string nummodesStr = nStr;
                        
                        Equation nummodesEqn(nummodesStr);                    
                        
                        expansion_order = (int) nummodesEqn.Evaluate();
                        
                        useExpansionType = true;
                    }
                    else // assume expansion is defined individually
                    {
                        // Extract the attributes.
                        const char *bTypeStr = expansion->Attribute("BASISTYPE");
                        ASSERTL0(bTypeStr,"TYPE or BASISTYPE was not defined in EXPANSION section of input");                     
                        std::string basisTypeStr = bTypeStr;

                        // interpret the basis type string. 
                        std::vector<std::string> basisStrings;
                        std::vector<LibUtilities::BasisType> basis;
                        bool valid = ParseUtils::GenerateOrderedStringVector(basisTypeStr.c_str(), basisStrings);
                        ASSERTL0(valid, "Unable to correctly parse the basis types.");
                        for (vector<std::string>::size_type i = 0; i < basisStrings.size(); i++)
                        {
                            valid = false;
                            for (unsigned int j = 0; j < LibUtilities::SIZE_BasisType; j++)
                            {
                                if (LibUtilities::BasisTypeMap[j] == basisStrings[i])
                                {
                                    basis.push_back((LibUtilities::BasisType) j);
                                    valid = true;
                                    break;
                                }
                            }
                            ASSERTL0(valid, std::string("Unable to correctly parse the basis type: ").append(basisStrings[i]).c_str());
                        }
                        const char *nModesStr = expansion->Attribute("NUMMODES");
                        ASSERTL0(nModesStr,"NUMMODES was not defined in EXPANSION section of input");                     

                        std::string numModesStr = nModesStr;
                        std::vector<unsigned int> numModes;
                        valid = ParseUtils::GenerateOrderedVector(numModesStr.c_str(), numModes);
                        ASSERTL0(valid, "Unable to correctly parse the number of modes.");
                        ASSERTL0(numModes.size() == basis.size(),"information for num modes does not match the number of basis");
                        
                        const char *pTypeStr =  expansion->Attribute("POINTSTYPE");
                        ASSERTL0(pTypeStr,"POINTSTYPE was not defined in EXPANSION section of input");                     
                        std::string pointsTypeStr = pTypeStr;
                        // interpret the points type string. 
                        std::vector<std::string> pointsStrings;
                        std::vector<LibUtilities::PointsType> points;
                        valid = ParseUtils::GenerateOrderedStringVector(pointsTypeStr.c_str(), pointsStrings);
                        ASSERTL0(valid, "Unable to correctly parse the points types.");
                        for (vector<std::string>::size_type i = 0; i < pointsStrings.size(); i++)
                        {
                            valid = false;
                            for (unsigned int j = 0; j < LibUtilities::SIZE_PointsType; j++)
                            {
                                if (LibUtilities::kPointsTypeStr[j] == pointsStrings[i])
                                {
                                    points.push_back((LibUtilities::PointsType) j);
                                    valid = true;
                                    break;
                                }
                            }
                            ASSERTL0(valid, std::string("Unable to correctly parse the points type: ").append(pointsStrings[i]).c_str());
                        }
                        
                        const char *nPointsStr = expansion->Attribute("NUMPOINTS");
                        ASSERTL0(nPointsStr,"NUMPOINTS was not defined in EXPANSION section of input");
                        std::string numPointsStr = nPointsStr;
                        std::vector<unsigned int> numPoints;
                        valid = ParseUtils::GenerateOrderedVector(numPointsStr.c_str(), numPoints);
                        ASSERTL0(valid, "Unable to correctly parse the number of points.");
                        ASSERTL0(numPoints.size() == numPoints.size(),"information for num points does not match the number of basis");
                        
                        for(int i = 0; i < basis.size(); ++i)
                        {
                            //Generate Basis key  using information
                            const LibUtilities::PointsKey pkey(numPoints[i],points[i]);
                            basiskeyvec.push_back(LibUtilities::BasisKey(basis[i],numModes[i],pkey));
                        }
                    }                
                    
                    // Now have composite and basiskeys.  Cycle through
                    // all composites for the geomShPtrs and set the modes
                    // and types for the elements contained in the element
                    // list.
                    CompositeVectorIter compVecIter;
                    for (compVecIter = compositeVector.begin(); compVecIter != compositeVector.end(); ++compVecIter)
                    {
                        GeometryVectorIter geomVecIter;
                        for (geomVecIter = (*compVecIter)->begin(); geomVecIter != (*compVecIter)->end(); ++geomVecIter)
                        {
                            ExpansionVectorIter expVecIter;
                            for (expVecIter = m_ExpansionVector.begin(); expVecIter != m_ExpansionVector.end(); ++expVecIter)
                            {
                                if (*geomVecIter == (*expVecIter)->m_GeomShPtr)
                                {
                                    if(useExpansionType)
                                    {
                                        (*expVecIter)->m_BasisKeyVector = DefineBasisKeyFromExpansionType(*geomVecIter,expansion_type,expansion_order);
                                    }
                                    else
                                    {
                                        ASSERTL0((*geomVecIter)->GetShapeDim() == basiskeyvec.size()," There is an incompatible expansion dimension with geometry dimension");
                                        (*expVecIter)->m_BasisKeyVector = basiskeyvec;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    
                    expansion = expansion->NextSiblingElement("E");
                }
            }
        }


    void MeshGraph::ReadCurves(TiXmlDocument &doc)
    {
        /// We know we have it since we made it this far.
        TiXmlHandle docHandle(&doc);
        TiXmlElement* mesh = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();
        TiXmlElement* field = NULL;

        int err;

        /// Look for elements in CURVE block.
        field = mesh->FirstChildElement("CURVED");

        if(!field) //return if no curved entities
        {
            return; 
        }

        /// All curves are of the form: "<? ID="#" TYPE="GLL OR other
        /// points type" NUMPOINTS="#"> ... </?>", with ? being an
        /// element type (either E or F).

        TiXmlElement *edgelement = field->FirstChildElement("E");

        int edgeindx, edgeid;
        int nextEdgeNumber = -1;

        while(edgelement)
        {
            /// These should be ordered.
            nextEdgeNumber++;

            std::string edge(edgelement->ValueStr());
            ASSERTL0(edge == "E", (std::string("Unknown 3D curve type:") + edge).c_str());

            /// Read id attribute.
            err = edgelement->QueryIntAttribute("ID", &edgeindx);
            ASSERTL0(err == TIXML_SUCCESS, "Unable to read curve attribute ID.");
            ASSERTL0(edgeindx == nextEdgeNumber, "Curve IDs must begin with zero and be sequential.");

            /// Read edge id attribute.
            err = edgelement->QueryIntAttribute("EDGEID", &edgeid);
            ASSERTL0(err == TIXML_SUCCESS, "Unable to read curve attribute EDGEID.");

            /// Read text edgelement description.
            std::string elementStr;
            TiXmlNode* elementChild = edgelement->FirstChild();

            while(elementChild)
            {
                // Accumulate all non-comment element data
                if (elementChild->Type() == TiXmlNode::TEXT)
                {
                    elementStr += elementChild->ToText()->ValueStr();
                    elementStr += " ";
                }
                elementChild = elementChild->NextSibling();
            }

            ASSERTL0(!elementStr.empty(), "Unable to read curve description body.");

            /// Parse out the element components corresponding to type of element.
           if (edge == "E")
            {
                int numPts=0;
                // Determine the points type
                std::string typeStr = edgelement->Attribute("TYPE");
                ASSERTL0(!typeStr.empty(), "TYPE must be specified in " "points definition");

                LibUtilities::PointsType type;
                const std::string* begStr = LibUtilities::kPointsTypeStr;
                const std::string* endStr = LibUtilities::kPointsTypeStr + LibUtilities::SIZE_PointsType;
                const std::string* ptsStr = std::find(begStr, endStr, typeStr);

                ASSERTL0(ptsStr != endStr, "Invalid points type.");
                type = (LibUtilities::PointsType)(ptsStr - begStr);

                //Determine the number of points
                err = edgelement->QueryIntAttribute("NUMPOINTS", &numPts);
                ASSERTL0(err == TIXML_SUCCESS, "Unable to read curve attribute NUMPOINTS.");
                CurveSharedPtr curve(MemoryManager<Curve>::AllocateSharedPtr(edgeid, type));
                
                // Read points (x, y, z)
                double xval, yval, zval;
                std::istringstream elementDataStrm(elementStr.c_str());
                try
                {
                    while(!elementDataStrm.fail())
                    {
                        elementDataStrm >> xval >> yval >> zval;

                        // Need to check it here because we may not be
                        // good after the read indicating that there
                        // was nothing to read.
                        if (!elementDataStrm.fail())
                        {
                            VertexComponentSharedPtr vert(MemoryManager<VertexComponent>::AllocateSharedPtr(m_MeshDimension, edgeindx, xval, yval, zval));

                            curve->m_points.push_back(vert);
                        }

                    }
                }
                catch(...)
                {
                    NEKERROR(ErrorUtil::efatal,
                    (std::string("Unable to read curve data for EDGE: ") + elementStr).c_str());

                }

                ASSERTL0(curve->m_points.size() == numPts,"Number of points specificed by attribute NUMPOINTS is different from number of points in list");

                m_curvededges.push_back(curve);

                edgelement = edgelement->NextSiblingElement("E");

            } // end if-loop

        } // end while-loop


        TiXmlElement *facelement = field->FirstChildElement("F");
        int faceindx, faceid;
        int nextFaceNumber = -1;
        
        while(facelement)
        {
            cout << "facelement = " << *facelement << endl;
             
            /// These should be ordered.
            nextFaceNumber++;

            std::string face(facelement->ValueStr());
            ASSERTL0(face == "F", (std::string("Unknown 3D curve type: ") + face).c_str());

            /// Read id attribute.
            err = facelement->QueryIntAttribute("ID", &faceindx);

            ASSERTL0(err == TIXML_SUCCESS, "Unable to read curve attribute ID.");
            ASSERTL0(faceindx == nextFaceNumber, "Face IDs must begin with zero and be sequential.");


            /// Read edge id attribute.
            err = edgelement->QueryIntAttribute("FACEID", &faceid);
            ASSERTL0(err == TIXML_SUCCESS, "Unable to read curve attribute FACEID.");

            /// Read text face element description.
            std::string elementStr;
            TiXmlNode* elementChild = facelement->FirstChild();
          
            while(elementChild)
            {
                // Accumulate all non-comment element data
                if (elementChild->Type() == TiXmlNode::TEXT)
                {
                    elementStr += elementChild->ToText()->ValueStr();
                    elementStr += " ";
                }
                elementChild = elementChild->NextSibling();
            }

            ASSERTL0(!elementStr.empty(), "Unable to read curve description body.");
            
            
            /// Parse out the element components corresponding to type of element.
            
            if(face == "F") 
            {
                std::string typeStr = facelement->Attribute("TYPE");
                ASSERTL0(!typeStr.empty(), "TYPE must be specified in " "points definition");
                LibUtilities::PointsType type;
                const std::string* begStr = LibUtilities::kPointsTypeStr;
                const std::string* endStr = LibUtilities::kPointsTypeStr + LibUtilities::SIZE_PointsType;
                const std::string* ptsStr = std::find(begStr, endStr, typeStr);
                
                ASSERTL0(ptsStr != endStr, "Invalid points type.");
                type = (LibUtilities::PointsType)(ptsStr - begStr);
                
                std::string numptsStr = facelement->Attribute("NUMPOINTS");
                ASSERTL0(!numptsStr.empty(), "NUMPOINTS must be specified in points definition");
                int numPts=0;
                std::strstream s;
                s << numptsStr;
                s >> numPts;
                
                
                CurveSharedPtr curve(MemoryManager<Curve>::AllocateSharedPtr(faceid, type));
                
                cout << "numPts = " << numPts << endl;
                ASSERTL0(numPts >= 3, "NUMPOINTS for face must be greater than 2");
                
                if(numPts == 3)
                {
                    ASSERTL0(ptsStr != endStr, "Invalid points type.");
                }
                
                // Read points (x, y, z)
                double xval, yval, zval;
                std::istringstream elementDataStrm(elementStr.c_str());
                try
                {
                    while(!elementDataStrm.fail())
                    {
                        elementDataStrm >> xval >> yval >> zval;
                        
                        // Need to check it here because we may not be good after the read
                        // indicating that there was nothing to read.
                        if (!elementDataStrm.fail())
                        {
                            VertexComponentSharedPtr vert(MemoryManager<VertexComponent>::AllocateSharedPtr(m_MeshDimension, faceindx, xval, yval, zval));
                            curve->m_points.push_back(vert);
                            
                        }
                        
                        cout << "xval = " << xval << "  yval = " << yval <<"  zval = " << zval << endl;
                        cout << endl;
                        
                    }
                }
                catch(...)
                {
                    NEKERROR(ErrorUtil::efatal,
                             (std::string("Unable to read curve data for FACE: ") + elementStr).c_str());
                    
                }
                m_curvedfaces.push_back(curve);
                
                facelement = facelement->NextSiblingElement("F");
                
            } // end if-loop            
        } // end while-loop
        
    } // end of ReadCurves()

        
    void MeshGraph::ReadCurves(std::string &infilename)
    {
        TiXmlDocument doc(infilename);
        bool loadOkay = doc.LoadFile();

        std::string errstr = "Unable to load file: ";
        errstr += infilename;
        ASSERTL0(loadOkay, errstr.c_str());

        ReadCurves(doc);
    }

    GeometrySharedPtr MeshGraph::GetCompositeItem(int whichComposite, int whichItem)
    {
        GeometrySharedPtr returnval;
        bool error = false;

        if (whichComposite >= 0 && whichComposite < int(m_MeshCompositeVector.size()))
        {
            if (whichItem >= 0 && whichItem < int(m_MeshCompositeVector[whichComposite]->size()))
            {
                returnval = m_MeshCompositeVector[whichComposite]->at(whichItem);
            }
            else
            {
                error = true;
            }
        }
        else
        {
            error = true;
        }

        if (error)
        {
            std::ostringstream errStream;
            errStream << "Unable to access composite item [" << whichComposite << "][" << whichItem << "].";

            std::string testStr = errStream.str();

            NEKERROR(ErrorUtil::efatal, testStr.c_str());
        }

        return returnval;
    }

    void MeshGraph::ReadDomain(TiXmlDocument &doc)
    {
        TiXmlHandle docHandle(&doc);

        TiXmlElement* mesh = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();
        TiXmlElement* domain = NULL;

        ASSERTL0(mesh, "Unable to find GEOMETRY tag in file.");

        /// Look for data in DOMAIN block.
        domain = mesh->FirstChildElement("DOMAIN");

        ASSERTL0(domain, "Unable to find DOMAIN tag in file.");

        // find the non comment portion of the body.
        TiXmlNode* elementChild = domain->FirstChild();
        while(elementChild && elementChild->Type() != TiXmlNode::TEXT)
        {
            elementChild = elementChild->NextSibling();
        }

        ASSERTL0(elementChild, "Unable to read DOMAIN body.");
        std::string elementStr = elementChild->ToText()->ValueStr();

        elementStr = elementStr.substr(elementStr.find_first_not_of(" "));

        std::string::size_type indxBeg = elementStr.find_first_of('[') + 1;
        std::string::size_type indxEnd = elementStr.find_last_of(']') - 1;
        std::string indxStr = elementStr.substr(indxBeg, indxEnd - indxBeg + 1);

        ASSERTL0(!indxStr.empty(), "Unable to read domain's composite index (index missing?).");

        // Read the domain composites.
        // Parse the composites into a list.
        GetCompositeList(indxStr, m_Domain);
        ASSERTL0(!m_Domain.empty(), (std::string("Unable to obtain domain's referenced composite: ") + indxStr).c_str());
    }

    void MeshGraph::GetCompositeList(const std::string &compositeStr, CompositeVector &compositeVector) const
    {
        // Parse the composites into a list.
        typedef vector<unsigned int> SeqVector;
        SeqVector seqVector;
        bool parseGood = ParseUtils::GenerateSeqVector(compositeStr.c_str(), seqVector);

        ASSERTL0(parseGood && !seqVector.empty(), (std::string("Unable to read composite index range: ") + compositeStr).c_str());

        SeqVector addedVector;    // Vector of those composites already added to compositeVector;
        for (SeqVector::iterator iter = seqVector.begin(); iter != seqVector.end(); ++iter)
        {
            // Only add a new one if it does not already exist in vector.
            // Can't go back and delete with a vector, so prevent it from
            // being added in the first place.
            if (std::find(addedVector.begin(), addedVector.end(), *iter) == addedVector.end())
            {
                addedVector.push_back(*iter);
                Composite composite = GetComposite(*iter);
                CompositeVector::iterator compIter;
                if (composite)
                {
                    compositeVector.push_back(composite);
                }
                else
                {
                    char str[64];
                    ::sprintf(str, "%d", *iter);
                    NEKERROR(ErrorUtil::ewarning, (std::string("Undefined composite: ") + str).c_str());
                    
                }
            }
        }
    }
        
	int MeshGraph::CheckFieldDefinition(FieldDefinitionsSharedPtr &fielddefs)
        {
            int i;
            ASSERTL0(fielddefs->m_ElementIDs.size() > 0, "Fielddefs vector must contain at least one element of data .");
            
            unsigned int numbasis = 0; 
            
            // Determine nummodes vector lists are correct length
            switch(fielddefs->m_ShapeType)
            {
            case eSegment:
                numbasis = 1;
                break;
            case eTriangle:  case eQuadrilateral:
                numbasis = 2;
                break;
            case eTetrahedron:
            case ePyramid:
            case ePrism:
            case eHexahedron:
                numbasis = 3;
                break;
            }
            
            unsigned int datasize = 0;             
            
            ASSERTL0(fielddefs->m_Basis.size() == numbasis, "Length of basis vector is incorrect");
            if(fielddefs->m_UniOrder == true)
            {
                unsigned int cnt = 0; 
                // calculate datasize
                switch(fielddefs->m_ShapeType)
                {
                case eSegment:
                    datasize += fielddefs->m_NumModes[cnt++];
                    break;
                case eTriangle:
                    {
                        int l = fielddefs->m_NumModes[cnt++];
                        int m = fielddefs->m_NumModes[cnt++];
                        datasize += StdRegions::StdTriData::getNumberOfCoefficients(l,m); 
                    }
                    break;
                case eQuadrilateral:
                    datasize += fielddefs->m_NumModes[cnt++]*
                        fielddefs->m_NumModes[cnt++];
                    break;
                case eTetrahedron:
                    {
                        int l = fielddefs->m_NumModes[cnt++];
                        int m = fielddefs->m_NumModes[cnt++];
                        int n = fielddefs->m_NumModes[cnt++];
                        datasize += StdRegions::StdTetData::getNumberOfCoefficients(l,m,n);
                    }
                    break;
                case ePyramid:
                    {
                        int l = fielddefs->m_NumModes[cnt++];
                        int m = fielddefs->m_NumModes[cnt++];
                        int n = fielddefs->m_NumModes[cnt++];
                        datasize += StdRegions::StdPyrData::getNumberOfCoefficients(l,m,n);
                    }
                    break;
                case  ePrism:
                    {
                        int l = fielddefs->m_NumModes[cnt++];
                        int m = fielddefs->m_NumModes[cnt++];
                        int n = fielddefs->m_NumModes[cnt++];
                        datasize += StdRegions::StdPrismData::getNumberOfCoefficients(l,m,n);
                    }
                    break;
                case eHexahedron:
                    datasize += fielddefs->m_NumModes[cnt++]*
                        fielddefs->m_NumModes[cnt++]*
                        fielddefs->m_NumModes[cnt++];
                    break;
                }
                
                datasize *= fielddefs->m_ElementIDs.size();
            }
            else
            {
                unsigned int cnt = 0; 
                // calculate data length
                for(i = 0; i < fielddefs->m_ElementIDs.size(); ++i)
                {
                    switch(fielddefs->m_ShapeType)
                    {
                    case eSegment:
                        datasize += fielddefs->m_NumModes[cnt++];
                        break;
                    case eTriangle:
                        {
                            int l = fielddefs->m_NumModes[cnt++];
                            int m = fielddefs->m_NumModes[cnt++];
                            datasize += StdRegions::StdTriData::getNumberOfCoefficients(l,m); 
                        }
                        break;
                    case eQuadrilateral:
                        datasize += fielddefs->m_NumModes[cnt++]*
                            fielddefs->m_NumModes[cnt++];
                        break;
                    case eTetrahedron:
                        {
                            int l = fielddefs->m_NumModes[cnt++];
                            int m = fielddefs->m_NumModes[cnt++];
                            int n = fielddefs->m_NumModes[cnt++];
                            datasize += StdRegions::StdTetData::getNumberOfCoefficients(l,m,n);
                        }
                        break;
                    case ePyramid:
                        {
                            int l = fielddefs->m_NumModes[cnt++];
                            int m = fielddefs->m_NumModes[cnt++];
                            int n = fielddefs->m_NumModes[cnt++];
                            datasize += StdRegions::StdPyrData::getNumberOfCoefficients(l,m,n);
                        }
                        break;
                    case  ePrism:
                        {
                            int l = fielddefs->m_NumModes[cnt++];
                            int m = fielddefs->m_NumModes[cnt++];
                            int n = fielddefs->m_NumModes[cnt++];
                            datasize += StdRegions::StdPrismData::getNumberOfCoefficients(l,m,n);
                        }
                        break;
                    case eHexahedron:
                        datasize += fielddefs->m_NumModes[cnt++]*
                            fielddefs->m_NumModes[cnt++]*
                            fielddefs->m_NumModes[cnt++];
                        break;
                    }
                }
            }
            
            return datasize;
        }
            
	void MeshGraph::Write(std::string         &outfilename, 
                              std::vector<FieldDefinitionsSharedPtr> &fielddefs,
                              std::vector<std::vector<double> >      &fielddata)
        {

            // Create the XML output.
            std::ofstream xmlFile(outfilename.c_str(), std::ios::out | std::ios::trunc);
            ASSERTL0(xmlFile, std::string("Unable to save file: ").append(outfilename));

            Write(xmlFile,fielddefs,fielddata);
        }

	void MeshGraph::Write(std::ofstream                          &xmlFile, 
                              std::vector<FieldDefinitionsSharedPtr> &fielddefs,
                              std::vector<std::vector<double> >      &fielddata)
        {
            ASSERTL1(fielddefs.size() == fielddata.size(),"Length of fielddefs and fielddata incompatible");
            
            xmlFile << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << std::endl;
            xmlFile << "<nektar>" << std::endl;
            
            for(int f = 0; f < fielddefs.size(); ++f)
            {
                WriteElements(xmlFile,fielddefs[f],fielddata[f]);
            }

            xmlFile << "</nektar>" << std::endl;
            xmlFile.flush();
            xmlFile.close();
        }

        void MeshGraph::WriteElements(std::ofstream              &xmlFile, 
                                      FieldDefinitionsSharedPtr  &fielddefs, 
                                      std::vector<double>        &fielddata)
        {
                
            ASSERTL1(fielddata.size() > 0, "Fielddata vector must contain at least one value.");
            
            int datasize = CheckFieldDefinition(fielddefs);
            
            ASSERTL1(fielddata.size() == fielddefs->m_Fields.size()*datasize, "Invalid size of fielddata vector.");
            
            std::string fieldsString;
            {
                std::stringstream fieldsStringStream;
                bool first = true;
                for (std::vector<int>::size_type i = 0; i < fielddefs->m_Fields.size(); i++)
                {
                    if (!first) fieldsStringStream << ",";
                    fieldsStringStream << fielddefs->m_Fields[i];
                    first = false;
                }
                fieldsString = fieldsStringStream.str();
            }
            
            std::string   shapeString; 
            {
                std::stringstream shapeStringStream;
                
                shapeStringStream << GeomShapeTypeMap[fielddefs->m_ShapeType]; 
                
                shapeString = shapeStringStream.str();
            }
            
            std::string basisString;
            {
                std::stringstream basisStringStream;
                bool first = true;
                for (std::vector<LibUtilities::BasisType>::size_type i = 0; i < fielddefs->m_Basis.size(); i++)
                {
                    if (!first) basisStringStream << ",";
                    basisStringStream << LibUtilities::BasisTypeMap[fielddefs->m_Basis[i]];
                        first = false;
                }       
                basisString = basisStringStream.str();
            }
            
            std::string numModesString;
            {
                std::stringstream numModesStringStream;
                
                if(fielddefs->m_UniOrder)
                {
                    numModesStringStream << "UniOrder:";
                    // Just dump single definition
                    bool first = true;
                    for (std::vector<int>::size_type i = 0; i < fielddefs->m_Basis.size(); i++)
                    {
                        if (!first) numModesStringStream << ",";
                        numModesStringStream << fielddefs->m_NumModes[i];
                        first = false;
                    }
                }
                else
                {
                    numModesStringStream << "MixOrder:";
                    bool first = true;
                    for (std::vector<int>::size_type i = 0; i < fielddefs->m_NumModes.size(); i++)
                    {
                        if (!first) numModesStringStream << ",";
                        numModesStringStream << fielddefs->m_NumModes[i];
                        first = false;
                    }
                }
                
                numModesString = numModesStringStream.str();
            }
            
            
            // Should ideally look at ways of compressing this stream
            // if just sequential;
            std::string   idString;
            {
                std::stringstream idStringStream;
                bool first = true;
                for (std::vector<Geometry>::size_type i = 0; i < fielddefs->m_ElementIDs.size(); i++)
                {
                    if (!first) idStringStream << ",";
                    idStringStream << fielddefs->m_ElementIDs[i];
                    first = false;
                }
                idString = idStringStream.str();
            }
            
            std::string compressedDataString;
            {
                // Serialize the fielddata vector to the stringstream.
                std::stringstream archiveStringStream(std::string((char*)&fielddata[0], sizeof(fielddata[0])/sizeof(char)*fielddata.size()));
                
                // Compress the serialized data.
                std::stringstream compressedData;
                {
                    boost::iostreams::filtering_streambuf<boost::iostreams::input> out; 
                    out.push(boost::iostreams::zlib_compressor());
                    out.push(archiveStringStream);
                    boost::iostreams::copy(out, compressedData);
                }
                
                // If the string length is not divisible by 3,
                // pad it. There is a bug in transform_width
                // that will make it reference past the end
                // and crash.
                switch (compressedData.str().length() % 3)
                {
                case 1:
                    compressedData << '\0';
                case 2:
                    compressedData << '\0';
                    break;
                }
                compressedDataString = compressedData.str();
            }
            
            // Convert from binary to base64.
            typedef boost::archive::iterators::base64_from_binary<
            boost::archive::iterators::transform_width<
            std::string::const_iterator, 6, 8> > base64_t;
            std::string base64string(base64_t(compressedDataString.begin()), base64_t(compressedDataString.end()));
            
            xmlFile << "    <elements ";
		xmlFile << "fields=\"" << fieldsString << "\" ";
		xmlFile << "shape=\"" << shapeString << "\" ";
		xmlFile << "basis=\"" << basisString << "\" ";
		xmlFile << "numModesPerDir=\"" << numModesString << "\" ";
		xmlFile << "id=\"" << idString << "\">";
		xmlFile << base64string << "</elements>" << std::endl;
        }

        // Allow global id string 
        // Use singe shape per block, single expansion per block        
        // Change numModes to numModesPerDir, add initial identifier 
        // to be "UniOrder" or "MultiOrder"

        
	void MeshGraph::Import(std::string &infilename, std::vector<FieldDefinitionsSharedPtr> &fielddefs, std::vector<std::vector<NekDouble> > &fielddata)
	{
            int cntdumps = 0;
            ASSERTL1(fielddefs.size() == 0, "Expected an empty fielddefs vector.");
            ASSERTL1(fielddata.size() == 0, "Expected an empty fielddata vector.");
            
            TiXmlDocument doc(infilename);
            bool loadOkay = doc.LoadFile();
            
            std::stringstream errstr;
            errstr << "Unable to load file: " << infilename << std::endl;
            errstr << "Reason: " << doc.ErrorDesc() << std::endl;
            errstr << "Position: Line " << doc.ErrorRow() << ", Column " << doc.ErrorCol() << std::endl;
            ASSERTL0(loadOkay, errstr.str().c_str());
            
            TiXmlHandle docHandle(&doc);
            TiXmlElement* master = NULL;    // Master tag within which all data is contained.
            
            master = doc.FirstChildElement("nektar");
            ASSERTL0(master, "Unable to find nektar tag in file.");
            
            // Loop through all nektar tags, finding all of the element tags.
            while (master)
            {
                TiXmlElement* element = master->FirstChildElement("elements");
                ASSERTL0(element, "Unable to find element tag within nektar tag.");
                while (element)
                {
                    // Extract the attributes.
                    std::string idString;
                    std::string shapeString;
                    std::string basisString;
                    std::string numModesString;
                    std::string fieldsString;
                    TiXmlAttribute *attr = element->FirstAttribute();
                    while (attr)
                    {
                        std::string attrName(attr->Name());
                        if (attrName == "fields")
                            fieldsString.insert(0, attr->Value());
                        else if (attrName == "shape")
                            shapeString.insert(0, attr->Value());
                        else if (attrName == "basis")
                            basisString.insert(0, attr->Value());
                        else if (attrName == "numModesPerDir")
                            numModesString.insert(0, attr->Value());
                        else if (attrName == "id")
                            idString.insert(0, attr->Value());
                        else
                        {
                            std::string errstr("Unknown attribute: ");
                            errstr += attrName;
                            ASSERTL1(false, errstr.c_str());
                        }
                        
                        // Get the next attribute.
                        attr = attr->Next();
                    }
                    
                    // Extract the body, which the "data".
                    TiXmlNode* elementChild = element->FirstChild();
                    ASSERTL0(elementChild, "Unable to extract the data from the element tag.");
                    std::string elementStr;
                    while(elementChild)
                    {
                        if (elementChild->Type() == TiXmlNode::TEXT)
                        {
                            elementStr += elementChild->ToText()->ValueStr();
                        }
                        elementChild = elementChild->NextSibling();
                    }
                    
                    // Convert from base64 to binary.
                    typedef boost::archive::iterators::transform_width<
                    boost::archive::iterators::binary_from_base64<
                    std::string::const_iterator>, 8, 6 > binary_t;
                    std::stringstream elementCompressedData(std::string(binary_t(elementStr.begin()), binary_t(elementStr.end())));
                    
                    // Decompress the binary data.
                    std::stringstream elementDecompressedData;
                    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
                    in.push(boost::iostreams::zlib_decompressor());
                    in.push(elementCompressedData);
                    try
                    {
                        boost::iostreams::copy(in, elementDecompressedData);
                    }
                    catch (boost::iostreams::zlib_error e)
                    {
                        if (e.error() == boost::iostreams::zlib::stream_end)
                        {
                            ASSERTL0(false, "Stream end zlib error");
                        }
                        else if (e.error() == boost::iostreams::zlib::stream_error)
                        {
                            ASSERTL0(false, "Stream zlib error");
                        }
                        else if (e.error() == boost::iostreams::zlib::version_error)
                        {
                            ASSERTL0(false, "Version zlib error");
                        }
                        else if (e.error() == boost::iostreams::zlib::data_error)
                        {
                            ASSERTL0(false, "Data zlib error");
                        }
                        else if (e.error() == boost::iostreams::zlib::mem_error)
                        {
                            ASSERTL0(false, "Memory zlib error");
                        }
                        else if (e.error() == boost::iostreams::zlib::buf_error)
                        {
                            ASSERTL0(false, "Buffer zlib error");
                        }
                        else
                        {
                            ASSERTL0(false, "Unknown zlib error");
                        }
                    }
                    
                    // Deserialize the array.
                    double* readFieldData = (double*) elementDecompressedData.str().c_str();
                    std::vector<double> elementFieldData(readFieldData, readFieldData + elementDecompressedData.str().length() * sizeof(*elementDecompressedData.str().c_str()) / sizeof(double));
                    fielddata.push_back(elementFieldData);
                    
                    // Reconstruct the fielddefs.
                    std::vector<unsigned int> elementIds;
                    {
                        bool valid = ParseUtils::GenerateSeqVector(idString.c_str(), elementIds);
                        ASSERTL0(valid, "Unable to correctly parse the element ids.");
                    }

                    SpatialDomains::GeomShapeType shape;
                    bool valid = false;
                    for (unsigned int j = 0; j < SpatialDomains::SIZE_GeomShapeType; j++)
                    {
                        if (SpatialDomains::GeomShapeTypeMap[j] == shapeString)
                        {
                            shape = (SpatialDomains::GeomShapeType) j;
                            valid = true;
                            break;
                        }
                    }
                    ASSERTL0(valid, std::string("Unable to correctly parse the shape type: ").append(shapeString).c_str());
                
                    std::vector<std::string> basisStrings;
                    std::vector<LibUtilities::BasisType> basis;
                    valid = ParseUtils::GenerateOrderedStringVector(basisString.c_str(), basisStrings);
                    ASSERTL0(valid, "Unable to correctly parse the basis types.");
                    for (std::vector<std::string>::size_type i = 0; i < basisStrings.size(); i++)
                    {
                        valid = false;
                        for (unsigned int j = 0; j < LibUtilities::SIZE_BasisType; j++)
                        {
                            if (LibUtilities::BasisTypeMap[j] == basisStrings[i])
                            {
                                basis.push_back((LibUtilities::BasisType) j);
                                valid = true;
                                break;
                            }
                        }
                        ASSERTL0(valid, std::string("Unable to correctly parse the basis type: ").append(basisStrings[i]).c_str());
                    }
                    
                    std::vector<unsigned int> numModes;
                    bool UniOrder = false;
                    
                    if(strstr(numModesString.c_str(),"UniOrder:"))
                    {
                        UniOrder  = true;
                    }

                    valid = ParseUtils::GenerateOrderedVector(numModesString.c_str()+9, numModes);
                    ASSERTL0(valid, "Unable to correctly parse the number of modes.");

                    std::vector<std::string> Fields;
                    valid = ParseUtils::GenerateOrderedStringVector(fieldsString.c_str(), Fields);
                    ASSERTL0(valid, "Unable to correctly parse the number of fields.");
                    
                    SpatialDomains::FieldDefinitionsSharedPtr fielddef  = MemoryManager<SpatialDomains::FieldDefinitions>::AllocateSharedPtr(shape, elementIds, basis, UniOrder, numModes, Fields);
                    int datasize = CheckFieldDefinition(fielddef);
                    
                    fielddefs.push_back(fielddef);
                    
                    ASSERTL0(fielddata[cntdumps++].size() == datasize*Fields.size(),"Input data is not the same length as header information");
                    
                    element = element->NextSiblingElement("elements");
                }
                master = master->NextSiblingElement("nektar");
            }
	}
        
        MeshGraph::~MeshGraph()
        {
        }
    }; //end of namespace
}; //end of namespace

//
// $Log: MeshGraph.cpp,v $
// Revision 1.31  2009/06/15 01:59:21  claes
// Gauss-Kronrod updates
//
// Revision 1.30  2009/04/29 10:55:17  pvos
// made demos working with nodal expansions
//
// Revision 1.29  2009/04/27 21:33:35  sherwin
// Added SolverInfoExists and others
//
// Revision 1.28  2009/04/20 16:13:23  sherwin
// Modified Import and Write functions and redefined how Expansion is used
//
// Revision 1.27  2009/01/12 10:26:59  pvos
// Added input tags for nodal expansions
//
// Revision 1.26  2008/10/04 19:32:46  sherwin
// Added SharedPtr Typedef and replaced MeshDimension with SpaceDimension
//
// Revision 1.25  2008/09/09 14:20:30  sherwin
// Updated to handle curved edges (first working version)
//
// Revision 1.24  2008/08/18 20:54:02  ehan
// Changed name CURVE to CURVED.
//
// Revision 1.23  2008/07/14 21:04:26  ehan
// Added ASSERTL0 to check valid points type and number of points.
//
// Revision 1.22  2008/07/09 23:41:20  ehan
// Added edge component and face component to the curve reader.
//
// Revision 1.21  2008/07/08 18:58:07  ehan
// Added curve reader.
//
// Revision 1.20  2008/06/30 19:34:46  ehan
// Fixed infinity recursive-loop error.
//
// Revision 1.19  2008/06/11 16:10:12  delisi
// Added the 3D reader.
//
// Revision 1.18  2008/05/30 00:33:48  delisi
// Renamed StdRegions::ShapeType to StdRegions::ExpansionType.
//
// Revision 1.17  2008/05/29 21:19:23  delisi
// Added the Write(...) and Import(...) functions which write and read XML files for output.
//
// Revision 1.16  2008/03/18 14:14:49  pvos
// Update for nodal triangular helmholtz solver
//
// Revision 1.15  2007/12/11 18:59:58  jfrazier
// Updated meshgraph so that a generic read could be performed and the proper type read (based on dimension) will be returned.
//
// Revision 1.14  2007/12/04 03:29:56  jfrazier
// Changed to stringstream.
//
// Revision 1.13  2007/11/07 20:31:03  jfrazier
// Added new expansion list to replace the expansion composite list.
//
// Revision 1.12  2007/09/25 04:45:14  jfrazier
// Added default linear expansions for the entire domain.
//
// Revision 1.11  2007/09/20 22:25:05  jfrazier
// Added expansion information to meshgraph class.
//
// Revision 1.10  2007/09/20 02:06:15  jfrazier
// General cleanup.
//
// Revision 1.9  2007/09/03 17:05:01  jfrazier
// Cleanup and addition of composite range in domain specification.
//
// Revision 1.8  2007/07/24 16:52:08  jfrazier
// Added domain code.
//
// Revision 1.7  2007/06/10 02:27:10  jfrazier
// Another checkin with an incremental completion of the boundary conditions reader.
//
// Revision 1.6  2007/03/29 19:25:10  bnelson
// *** empty log message ***
//
// Revision 1.5  2006/10/17 18:42:54  jfrazier
// Removed "NUMBER" attribute in items.
//
// Revision 1.4  2006/09/26 23:41:52  jfrazier
// Updated to account for highest level NEKTAR tag and changed the geometry tag to GEOMETRY.
//
// Revision 1.3  2006/08/24 18:50:00  jfrazier
// Completed error checking on permissable composite item combinations.
//
// Revision 1.2  2006/05/09 13:37:01  jfrazier
// Removed duplicate definition of shared vertex pointer.
//
// Revision 1.1  2006/05/04 18:59:02  kirby
// *** empty log message ***
//
// Revision 1.14  2006/04/09 02:08:35  jfrazier
// Added precompiled header.
//
// Revision 1.13  2006/04/04 23:12:37  jfrazier
// More updates to readers.  Still work to do on MeshGraph2D to store tris and quads.
//
// Revision 1.12  2006/03/25 00:58:29  jfrazier
// Many changes dealing with fundamental structure and reading/writing.
//
// Revision 1.11  2006/03/12 14:20:43  sherwin
//
// First compiling version of SpatialDomains and associated modifications
//
// Revision 1.10  2006/02/26 21:19:43  bnelson
// Fixed a variety of compiler errors caused by updates to the coding standard.
//
// Revision 1.9  2006/02/19 01:37:33  jfrazier
// Initial attempt at bringing into conformance with the coding standard.  Still more work to be done.  Has not been compiled.
//
//
