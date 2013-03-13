SET(THIRDPARTY_BUILD_METIS ON CACHE BOOL
    "Build ModMetis library from ThirdParty")

IF (THIRDPARTY_BUILD_METIS)
    INCLUDE(ExternalProject)
    EXTERNALPROJECT_ADD(
        modmetis-5.0.2
        PREFIX ${TPSRC}
        URL ${TPURL}/modmetis-5.0.2.tar.bz2
        URL_MD5 "ffbdc6a50283934389a0b3b0c32b62c0"
        DOWNLOAD_DIR ${TPSRC}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX:PATH=${TPSRC}/dist -DCMAKE_C_FLAGS:STRING=-fPIC -DGKLIB_PATH:PATH=${TPSRC}/src/modmetis-5.0.2/GKlib ${TPSRC}/src/modmetis-5.0.2
    )
    SET(METIS_LIB metis CACHE FILEPATH
        "METIS library" FORCE)
    MARK_AS_ADVANCED(METIS_LIB)
    LINK_DIRECTORIES(${TPSRC}/dist/lib)
    INCLUDE_DIRECTORIES(${TPSRC}/dist/include)
ELSE (THIRDPARTY_BUILD_METIS)
    INCLUDE (FindMetis)
ENDIF (THIRDPARTY_BUILD_METIS)

