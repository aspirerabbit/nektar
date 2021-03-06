SET(NEKMESHUTILS_SOURCES
    Module/Module.cpp
    MeshElements/Mesh.cpp
    MeshElements/Element.cpp
    MeshElements/Edge.cpp
    MeshElements/Face.cpp
    MeshElements/BooleanOperators.cpp
    MeshElements/Point.cpp
    MeshElements/Line.cpp
    MeshElements/Triangle.cpp
    MeshElements/Quadrilateral.cpp
    MeshElements/Tetrahedron.cpp
    MeshElements/Pyramid.cpp
    MeshElements/Prism.cpp
    MeshElements/Hexahedron.cpp
    CADSystem/CADSystem.cpp
    CADSystem/CADSurf.cpp
    CADSystem/ProcessLoadCAD.cpp
    )

IF(NEKTAR_USE_MESHGEN)
    SET(NEKMESHUTILS_SOURCES ${NEKMESHUTILS_SOURCES}
        SurfaceMeshing/CurveMesh.cpp
        Octree/Octant.cpp
        Octree/Octree.cpp
        Octree/ProcessLoadOctree.cpp
        SurfaceMeshing/SurfaceMesh.cpp
        SurfaceMeshing/HOSurfaceMesh.cpp
        SurfaceMeshing/FaceMesh.cpp
        SurfaceMeshing/OptimiseFunctions.cpp
        ExtLibInterface/TriangleInterface.cpp
        ExtLibInterface/TetGenInterface.cpp
        VolumeMeshing/VolumeMesh.cpp
        VolumeMeshing/TetMeshing/TetMesh.cpp
        VolumeMeshing/BLMeshing/BLMesh.cpp
        Optimisation/BGFS-B.cpp
        Triangle/Triangle.cpp
        CADSystem/OCE/CADSystemOCE.cpp
        CADSystem/OCE/CADVertOCE.cpp
        CADSystem/OCE/CADCurveOCE.cpp
        CADSystem/OCE/CADSurfOCE.cpp
        2DGenerator/2DGenerator.cpp
    )
ENDIF()

SET(NEKMESHUTILS_HEADERS
    NekMeshUtilsDeclspec.h
    Module/Module.h
    MeshElements/Node.h
    MeshElements/Edge.h
    MeshElements/Face.h
    MeshElements/Element.h
    MeshElements/Composite.h
    MeshElements/Mesh.h
    MeshElements/Point.h
    MeshElements/Line.h
    MeshElements/Triangle.h
    MeshElements/Quadrilateral.h
    MeshElements/Tetrahedron.h
    MeshElements/Pyramid.h
    MeshElements/Prism.h
    MeshElements/Hexahedron.h
    MeshElements/HOAlignment.h
    MeshElements/ElementConfig.h
    CADSystem/CADObject.h
    CADSystem/CADSystem.h
    CADSystem/CADVert.h
    CADSystem/CADCurve.h
    CADSystem/CADSurf.h
    CADSystem/ProcessLoadCAD.h
    )

IF(NEKTAR_USE_MESHGEN)
    SET(NEKMESHUTILS_HEADERS ${NEKMESHUTILS_HEADERS}
        Octree/SourcePoint.hpp
        SurfaceMeshing/CurveMesh.h
        Octree/Octant.h
        Octree/Octree.h
        Octree/ProcessLoadOctree.h
        SurfaceMeshing/SurfaceMesh.h
        SurfaceMeshing/HOSurfaceMesh.h
        SurfaceMeshing/FaceMesh.h
        SurfaceMeshing/OptimiseFunctions.h
        ExtLibInterface/TriangleInterface.h
        ExtLibInterface/TetGenInterface.h
        VolumeMeshing/VolumeMesh.h
        VolumeMeshing/TetMeshing/TetMesh.h
        VolumeMeshing/BLMeshing/BLMesh.h
        CADSystem/OCE/OpenCascade.h
        CADSystem/OCE/CADSystemOCE.h
        CADSystem/OCE/CADVertOCE.h
        CADSystem/OCE/CADCurveOCE.h
        CADSystem/OCE/CADSurfOCE.h
        Optimisation/BGFS-B.h
        Optimisation/OptimiseObj.h
        Triangle/Triangle.h
        2DGenerator/2DGenerator.h
    )
ENDIF()

ADD_DEFINITIONS(-DNEKMESHUTILS_EXPORTS)

ADD_NEKTAR_LIBRARY(NekMeshUtils lib ${NEKTAR_LIBRARY_TYPE}
    ${NEKMESHUTILS_SOURCES} ${NEKMESHUTILS_HEADERS})
TARGET_LINK_LIBRARIES(NekMeshUtils LINK_PUBLIC SpatialDomains)

IF(NEKTAR_USE_MESHGEN)
    TARGET_LINK_LIBRARIES(NekMeshUtils LINK_PRIVATE ${TETGEN_LIBRARY})
    TARGET_LINK_LIBRARIES(NekMeshUtils LINK_PRIVATE ${OCC_LIBRARIES})

    ADD_DEPENDENCIES(NekMeshUtils oce-0.17 tetgen-1.5)

ENDIF()

IF(NEKTAR_USE_CFI)
    TARGET_LINK_LIBRARIES(NekMeshUtils LINK_PRIVATE ${CFI_LIBRARY})
ENDIF()

INSTALL(DIRECTORY ./
        DESTINATION ${NEKTAR_INCLUDE_DIR}/NekMeshUtils
        COMPONENT dev
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
