////////////////////////////////////////////////////////////////////////////////
//
//  File:  $Source: /usr/sci/projects/Nektar/cvs/Nektar++/library/SpatialDomains/MeshGraph2D.cpp,v $
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

#include <SpatialDomains/MeshGraph2D.h>
#include <SpatialDomains/SegGeom.h>

namespace Nektar
{
    namespace SpatialDomains
    {
        MeshGraph2D::MeshGraph2D()
            //m_geofac_defined(false)
        {
        }

        MeshGraph2D::~MeshGraph2D()
        {
        }

        void MeshGraph2D::ReadGeometry(std::string &infilename)
        {
            TiXmlDocument doc(infilename);
            bool loadOkay = doc.LoadFile();

            ASSERTL0(loadOkay, (std::string("Unable to load file: ") + 
                infilename).c_str());

            ReadGeometry(doc);
        }

        // \brief Read segments (and general MeshGraph) given TiXmlDocument.
        void MeshGraph2D::ReadGeometry(TiXmlDocument &doc)
        {
            // Read mesh first
            MeshGraph::ReadGeometry(doc);
            TiXmlHandle docHandle(&doc);

            TiXmlNode* node = NULL;
            TiXmlElement* mesh = NULL;

            /// Look for all geometry related data in GEOMETRY block.
            mesh = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();

            ASSERTL0(mesh, "Unable to find GEOMETRY tag in file.");

            ReadEdges(doc);
            ReadElements(doc);
            ReadComposites(doc);
            ReadDomain(doc);
        }

        void MeshGraph2D::ReadEdges(TiXmlDocument &doc)
        {
            /// We know we have it since we made it this far.
            TiXmlHandle docHandle(&doc);
            TiXmlElement* mesh = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();
            TiXmlElement* field = NULL;

            /// Look for elements in ELEMENT block.
            field = mesh->FirstChildElement("EDGE");

            ASSERTL0(field, "Unable to find EDGE tag in file.");

            /// All elements are of the form: "<E ID="#"> ... </E>", with
            /// ? being the element type.
            /// Read the ID field first.
            TiXmlElement *edge = field->FirstChildElement("E");

            /// Since all edge data is one big text block, we need to accumulate
            /// all TEXT data and then parse it.  This approach effectively skips
            /// all comments or other node types since we only care about the
            /// edge list.  We cannot handle missing edge numbers as we could
            /// with missing element numbers due to the text block format.
            std::string edgeStr;
            int indx;
            int err = 0;
            int nextEdgeNumber = -1;

            while(edge)
            {
                nextEdgeNumber++;

                int err = edge->QueryIntAttribute("ID",&indx);
                ASSERTL0(err == TIXML_SUCCESS, "Unable to read edge attribute ID.");
                ASSERTL0(indx == nextEdgeNumber, "Edge IDs must begin with zero and be sequential.");

                TiXmlNode *child = edge->FirstChild();
                edgeStr.clear();
                if (child->Type() == TiXmlNode::TEXT)
                {
                    edgeStr += child->ToText()->ValueStr();
                }

                /// Now parse out the edges, three fields at a time.
                int vertex1, vertex2;
                std::istrstream edgeDataStrm(edgeStr.c_str());

                try
                {
                    while (!edgeDataStrm.fail())
                    {
                        edgeDataStrm >> vertex1 >> vertex2;

                        // Must check after the read because we may be at the end and not know it.
                        // If we are at the end we will add a duplicate of the last entry if we
                        // don't check here.
                        if (!edgeDataStrm.fail())
                        {
                            VertexComponentSharedPtr vertices[2] = {GetVertex(vertex1), GetVertex(vertex2)};
                            EdgeComponentSharedPtr edge(MemoryManager<EdgeComponent>::AllocateSharedPtr(indx, m_MeshDimension, vertices));

                            m_ecomps.push_back(edge);
                        }
                    }
                }
                catch(...)
                {
                    NEKERROR(ErrorUtil::efatal, (std::string("Unable to read edge data: ") + edgeStr).c_str());
                }

                edge = edge->NextSiblingElement("E");
            }
        }

        void MeshGraph2D::ReadElements(TiXmlDocument &doc)
        {
            /// We know we have it since we made it this far.
            TiXmlHandle docHandle(&doc);
            TiXmlElement* mesh = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();
            TiXmlElement* field = NULL;

            /// Look for elements in ELEMENT block.
            field = mesh->FirstChildElement("ELEMENT");

            ASSERTL0(field, "Unable to find ELEMENT tag in file.");

            int nextElementNumber = -1;

            /// All elements are of the form: "<? ID="#"> ... </?>", with
            /// ? being the element type.

            TiXmlElement *element = field->FirstChildElement();

            while (element)
            {
                std::string elementType(element->ValueStr());

                ASSERTL0(elementType == "Q" || elementType == "T",
                    (std::string("Unknown 2D element type: ") + elementType).c_str());

                /// These should be ordered.
                nextElementNumber++;

                /// Read id attribute.
                int indx;
                int err = element->QueryIntAttribute("ID", &indx);
                ASSERTL0(err == TIXML_SUCCESS, "Unable to read element attribute ID.");
                ASSERTL0(indx == nextElementNumber, "Element IDs must begin with zero and be sequential.");

                /// Read text element description.
                TiXmlNode* elementChild = element->FirstChild();
                std::string elementStr;
                while(elementChild)
                {
                    if (elementChild->Type() == TiXmlNode::TEXT)
                    {
                        elementStr += elementChild->ToText()->ValueStr();
                    }
                    elementChild = elementChild->NextSibling();
                }

                ASSERTL0(!elementStr.empty(), "Unable to read element description body.");

                /// Parse out the element components corresponding to type of element.
                if (elementType == "T")
                {
                    // Read three edge numbers
                    int edge1, edge2, edge3;
                    std::istrstream elementDataStrm(elementStr.c_str());

                    try
                    {
                        elementDataStrm >> edge1;
                        elementDataStrm >> edge2;
                        elementDataStrm >> edge3;

                        ASSERTL0(!elementDataStrm.fail(), (std::string("Unable to read element data for TRIANGLE: ") + elementStr).c_str());

                        /// Create a TriGeom to hold the new definition.
                        EdgeComponentSharedPtr edges[TriGeom::kNedges] = 
                        {
                            GetEdgeComponent(edge1),
                            GetEdgeComponent(edge2),
                            GetEdgeComponent(edge3)
                        };

                        StdRegions::EdgeOrientation edgeorient[TriGeom::kNedges] = 
                        {
                            EdgeComponent::GetEdgeOrientation(*edges[0], *edges[1]),
                            EdgeComponent::GetEdgeOrientation(*edges[1], *edges[2]), 
                            EdgeComponent::GetEdgeOrientation(*edges[2], *edges[0])
                        };

                        TriGeomSharedPtr trigeom(MemoryManager<TriGeom>::AllocateSharedPtr(edges, edgeorient));

                        m_trigeoms.push_back(trigeom);
                    }
                    catch(...)
                    {
                        NEKERROR(ErrorUtil::efatal,
                            (std::string("Unable to read element data for TRIANGLE: ") + elementStr).c_str());
                    }
                }
                else if (elementType == "Q")
                {
                    // Read four edge numbers
                    int edge1, edge2, edge3, edge4;
                    std::istrstream elementDataStrm(elementStr.c_str());

                    try
                    {
                        elementDataStrm >> edge1;
                        elementDataStrm >> edge2;
                        elementDataStrm >> edge3;
                        elementDataStrm >> edge4;

                        ASSERTL0(!elementDataStrm.fail(), (std::string("Unable to read element data for QUAD: ") + elementStr).c_str());

                        /// Create a QuadGeom to hold the new definition.
                        EdgeComponentSharedPtr edges[QuadGeom::kNedges] = 
                        {GetEdgeComponent(edge1),GetEdgeComponent(edge2),
                        GetEdgeComponent(edge3),GetEdgeComponent(edge4)};

                        StdRegions::EdgeOrientation edgeorient[QuadGeom::kNedges] =
                        {
                            EdgeComponent::GetEdgeOrientation(*edges[0], *edges[1]),
                            EdgeComponent::GetEdgeOrientation(*edges[1], *edges[2]),
                            EdgeComponent::GetEdgeOrientation(*edges[2], *edges[3]),
                            EdgeComponent::GetEdgeOrientation(*edges[3], *edges[0])
                        };

                        QuadGeomSharedPtr quadgeom(new QuadGeom(edges, edgeorient));

                        m_quadgeoms.push_back(quadgeom);

                    }
                    catch(...)
                    {
                        NEKERROR(ErrorUtil::efatal,(std::string("Unable to read element data for QUAD: ") + elementStr).c_str());
                    }
                }

                /// Keep looking
                element = element->NextSiblingElement();
            }
        }

        void MeshGraph2D::ReadComposites(TiXmlDocument &doc)
        {
            TiXmlHandle docHandle(&doc);

            /// We know we have it since we made it this far.
            TiXmlElement* mesh = docHandle.FirstChildElement("NEKTAR").FirstChildElement("GEOMETRY").Element();
            TiXmlElement* field = NULL;

            ASSERTL0(mesh, "Unable to find GEOMETRY tag in file.");

            /// Look for elements in ELEMENT block.
            field = mesh->FirstChildElement("COMPOSITE");

            ASSERTL0(field, "Unable to find COMPOSITE tag in file.");

            int nextCompositeNumber = -1;

            /// All elements are of the form: "<C ID = N> ... </C>".

            /// Read the ID field first.
            TiXmlElement *composite = field->FirstChildElement("C");

            while (composite)
            {
                nextCompositeNumber++;

                int indx;
                int err = composite->QueryIntAttribute("ID", &indx);
                ASSERTL0(err == TIXML_SUCCESS, "Unable to read attribute ID.");
                ASSERTL0(indx == nextCompositeNumber, "Composite IDs must begin with zero and be sequential.");

                // Read the body of the composite definition.
                TiXmlNode* compositeChild = composite->FirstChild();
                // This is primarily to skip comments that may be present.
                // Comments appear as nodes just like elements.
                // We are specifically looking for text in the body
                // of the definition.
                std::string compositeStr = compositeChild->ToText()->ValueStr();
                while(compositeChild)
                {
                    if (compositeChild->Type() == TiXmlNode::TEXT)
                    {
                        compositeStr += compositeChild->ToText()->ValueStr();
                        compositeStr += " ";    // Just in case there is no space between them.
                    }

                    compositeChild = compositeChild->NextSibling();
                }

                ASSERTL0(!compositeStr.empty(), "Unable to read composite description body.");

                /// Parse out the element components corresponding to type of element.

                std::istrstream compositeDataStrm(compositeStr.c_str());

                try
                {
                    bool first = true;
                    std::string prevCompositeElementStr;

                    while (!compositeDataStrm.fail())
                    {
                        std::string compositeElementStr;
                        compositeDataStrm >> compositeElementStr;

                        if (!compositeDataStrm.fail())
                        {
                            if (first)
                            {
                                first = false;

                                Composite curVector(MemoryManager<GeometryVector>::AllocateSharedPtr());
                                m_MeshCompositeVector.push_back(curVector);
                            }

                            if (compositeElementStr.length() > 0)
                            {
                                ResolveGeomRef(prevCompositeElementStr, compositeElementStr);
                            }
                            prevCompositeElementStr = compositeElementStr;
                        }
                    }
                }
                catch(...)
                {
                    NEKERROR(ErrorUtil::efatal,
                        (std::string("Unable to read COMPOSITE data for composite: ") + compositeStr).c_str());
                }

                /// Keep looking
                composite = composite->NextSiblingElement("C");
            }
        }


        EdgeComponentSharedPtr MeshGraph2D::GetEdgeComponent(int eID)
        {
            EdgeComponentSharedPtr returnval;

            if (eID >= 0 && eID < int(m_ecomps.size()))
            {
                returnval = m_ecomps[eID];
            }

            return returnval;
        };

        void MeshGraph2D::Write(std::string &outfilename)
        {
        }

#ifdef OLD
        // generate geometric factors based on MeshGraph information. 
        void MeshGraph2D::GenXGeoFac()
        {
            TriGeomVector::const_iterator defT; 

            for(defT = m_trigeoms.begin(); defT != m_trigeoms.end(); ++defT)
            {
                (*defT)->SetXGeoFac((*defT)->GenXGeoFac());
            }


            QuadGeomVector::const_iterator defQ; 
            for(defQ = m_quadgeoms.begin(); defQ != m_quadgeoms.end(); ++defQ)
            {
                (*defQ)->SetXGeoFac((*defQ)->GenXGeoFac());
            }


            m_geofac_defined = true;
        }
#endif

        // Take the string that is the composite reference and find the
        // pointer to the Geometry object corresponding to it.

        // The only allowable combinations of previous and current items
        // are V (0D); E (1D); and T and Q (2D).  Only elements of the same
        // dimension are allowed to be grouped.
        void MeshGraph2D::ResolveGeomRef(const std::string &prevToken, const std::string &token)
        {
            try
            {
                std::istringstream tokenStream(token);
                std::istringstream prevTokenStream(prevToken);

                char type;
                char prevType;

                tokenStream >> type;

                std::string::size_type indxBeg = token.find_first_of('[') + 1;
                std::string::size_type indxEnd = token.find_last_of(']') - 1;

                ASSERTL0(indxBeg <= indxEnd, (std::string("Error reading index definition:") + token).c_str());

                std::string indxStr = token.substr(indxBeg, indxEnd - indxBeg + 1);

                std::istringstream indexStrm(indxStr);
                int indx1=-1, indx2=-1;

                // Should read either [a] where a is a nonnegative integer, or
                // [a-b] where a and b are nonnegative integers, b>a.
                // Easiest way to know is if a '-' is present we have the latter
                // case.

                indexStrm >> indx1;
                ASSERTL0(indx1 >= 0, (std::string("Error reading range: ") + indxStr).c_str());
                indx2 = indx1;

                if (!indexStrm.fail())
                {
                    std::string::size_type dashLoc=indxStr.find('-');
                    if (dashLoc != std::string::npos)
                    {
                        // Skip up to and including the '-' character, then read
                        // the other index.  We are safe in doing this because we
                        // already know it is there...somewhere.
                        indexStrm.seekg(dashLoc+1);
                        indexStrm >> indx2;

                        ASSERTL0(indx1 < indx2 && indx2 >= 0,
                            (std::string("Error reading collection range: ") + indxStr).c_str());
                    }
                }

                if (!indexStrm.fail())
                {
                    prevTokenStream >> prevType;

                    // All composites must be of the same dimension.
                    bool validSequence = (prevToken.empty() ||         // No previous, then current is just fine.
                        (type == 'V' && prevType == 'V') ||
                        (type == 'E' && prevType == 'E') ||
                        ((type == 'T' || type == 'Q') &&
                        (prevType == 'T' || prevType == 'Q')));

                    ASSERTL0(validSequence, (std::string("Invalid combination of composite items: ")
                        + type + " and " + prevType + ".").c_str()); 

                    switch(type)
                    {
                    case 'E':   // Edge
                        for (int i=indx1; i<=indx2; ++i)
                        {
                            m_MeshCompositeVector.back()->push_back(m_ecomps[i]);
                        }
                        break;

                    case 'T':   // Triangle
                        for (int i=indx1; i<=indx2; ++i)
                        {
                            m_MeshCompositeVector.back()->push_back(m_trigeoms[i]);
                        }
                        break;

                    case 'Q':   // Quad
                        for (int i=indx1; i<=indx2; ++i)
                        {
                            m_MeshCompositeVector.back()->push_back(m_quadgeoms[i]);
                        }
                        break;

                    case 'V':   // Vertex
                        for (int i=indx1; i<=indx2; ++i)
                        {
                            m_MeshCompositeVector.back()->push_back(m_vertset[i]);
                        }
                        break;

                    default:
                        NEKERROR(ErrorUtil::efatal, (std::string("Unrecognized composite token: ") + token).c_str());
                    }
                }
            }
            catch(...)
            {
                NEKERROR(ErrorUtil::efatal, (std::string("Problem processing composite token: ") + token).c_str());
            }

            return;
        }

    }; //end of namespace
}; //end of namespace

//
// $Log: MeshGraph2D.cpp,v $
// Revision 1.19  2007/07/26 01:38:33  jfrazier
// Cleanup of some attribute reading code.
//
// Revision 1.18  2007/07/24 16:52:09  jfrazier
// Added domain code.
//
// Revision 1.17  2007/07/05 04:21:10  jfrazier
// Changed id format and propagated from 1d to 2d.
//
// Revision 1.16  2007/06/10 02:27:11  jfrazier
// Another checkin with an incremental completion of the boundary conditions reader.
//
// Revision 1.15  2007/06/07 23:55:24  jfrazier
// Intermediate revisions to add parsing for boundary conditions file.
//
// Revision 1.14  2007/05/28 21:48:42  sherwin
// Update for 2D functionality
//
// Revision 1.13  2006/10/17 22:26:01  jfrazier
// Added capability to specify ranges in composite definition.
//
// Revision 1.12  2006/10/17 18:42:54  jfrazier
// Removed "NUMBER" attribute in items.
//
// Revision 1.11  2006/09/26 23:41:53  jfrazier
// Updated to account for highest level NEKTAR tag and changed the geometry tag to GEOMETRY.
//
// Revision 1.10  2006/08/24 18:50:00  jfrazier
// Completed error checking on permissable composite item combinations.
//
// Revision 1.9  2006/08/18 19:37:17  jfrazier
// *** empty log message ***
//
// Revision 1.8  2006/08/17 22:55:00  jfrazier
// Continued adding code to process composites in the mesh2d.
//
// Revision 1.7  2006/08/16 23:34:42  jfrazier
// *** empty log message ***
//
// Revision 1.6  2006/06/02 18:48:40  sherwin
// Modifications to make ProjectLoc2D run bit there are bus errors for order > 3
//
// Revision 1.5  2006/06/01 14:15:30  sherwin
// Added typdef of boost wrappers and made GeoFac a boost shared pointer.
//
// Revision 1.4  2006/05/30 14:00:04  sherwin
// Updates to make MultiRegions and its Demos work
//
// Revision 1.3  2006/05/23 19:56:33  jfrazier
// These build and run, but the expansion pieces are commented out
// because they would not run.
//
// Revision 1.2  2006/05/09 13:37:01  jfrazier
// Removed duplicate definition of shared vertex pointer.
//
// Revision 1.1  2006/05/04 18:59:01  kirby
// *** empty log message ***
//
// Revision 1.11  2006/04/11 23:18:11  jfrazier
// Completed MeshGraph2D for tri's and quads.  Not thoroughly tested.
//
// Revision 1.10  2006/04/09 02:08:35  jfrazier
// Added precompiled header.
//
// Revision 1.9  2006/04/04 23:12:37  jfrazier
// More updates to readers.  Still work to do on MeshGraph2D to store tris and quads.
//
// Revision 1.8  2006/03/25 00:58:29  jfrazier
// Many changes dealing with fundamental structure and reading/writing.
//
// Revision 1.7  2006/03/12 14:20:43  sherwin
//
// First compiling version of SpatialDomains and associated modifications
//
// Revision 1.6  2006/03/12 07:42:03  sherwin
//
// Updated member names and StdRegions call. Still has not been compiled
//
// Revision 1.5  2006/02/26 21:19:43  bnelson
// Fixed a variety of compiler errors caused by updates to the coding standard.
//
// Revision 1.4  2006/02/19 01:37:34  jfrazier
// Initial attempt at bringing into conformance with the coding standard.  Still more work to be done.  Has not been compiled.
//
//
