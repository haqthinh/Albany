##*****************************************************************//
##    Albany 2.0:  Copyright 2012 Sandia Corporation               //
##    This Software is released under the BSD license detailed     //
##    in the file "license.txt" in the top-level Albany directory  //
##*****************************************************************//


# Adaptation

SET(SOURCES
    AAdapt_AbstractAdapterT.cpp
    AAdapt_AdaptationFactory.cpp
    AAdapt_CopyRemeshT.cpp
    AAdapt_AdaptiveSolutionManagerT.cpp
    AAdapt_InitialCondition.cpp
    AAdapt_AnalyticFunction.cpp
    AAdapt_SolutionObserver.cpp
    AAdapt_ReferenceConfigurationManager.cpp
   )

SET(HEADERS
    AAdapt_AbstractAdapterT.hpp
    AAdapt_AdaptationFactory.hpp
    AAdapt_CopyRemeshT.hpp
    AAdapt_AdaptiveSolutionManagerT.hpp
    AAdapt_InitialCondition.hpp
    AAdapt_AnalyticFunction.hpp
    AAdapt_SolutionObserver.hpp
    AAdapt_ReferenceConfigurationManager.hpp
   )

IF (ALBANY_EPETRA) 
  SET(SOURCES ${SOURCES} 
    AAdapt_AbstractAdapter.cpp
    AAdapt_CopyRemesh.cpp
    AAdapt_AdaptiveSolutionManager.cpp
    AAdapt_AdaptiveModelFactory.cpp
    AAdapt_ThyraAdaptiveModelEvaluator.cpp
   ) 
  SET(HEADERS ${HEADERS} 
    AAdapt_AbstractAdapter.hpp
    AAdapt_CopyRemesh.hpp
    AAdapt_AdaptiveSolutionManager.hpp
    AAdapt_AdaptiveModelFactory.hpp
    AAdapt_ThyraAdaptiveModelEvaluator.hpp
  )
ENDIF()

# Random Criterion & Stress Fracture depend on LCM
# Speculative features and code, currently
IF(ALBANY_LCM AND LCM_SPECULATIVE)

SET(SOURCES ${SOURCES}
    AAdapt_RandomCriterion.cpp
    AAdapt_StressFracture.cpp
   )

SET(HEADERS ${HEADERS}
    AAdapt_RandomCriterion.hpp
    AAdapt_StressFracture.hpp
   )

IF (ALBANY_EPETRA) 
SET(SOURCES ${SOURCES} 
    AAdapt_RandomFracture.cpp
    )
SET(HEADERS ${HEADERS}
    AAdapt_RandomFracture.hpp
    )
ENDIF()
ENDIF()

# Topology Modification depends on LCM
IF(ALBANY_LCM)

SET(SOURCES ${SOURCES}
    AAdapt_TopologyModificationT.cpp
   )

SET(HEADERS ${HEADERS}
    AAdapt_TopologyModificationT.hpp
   )

IF (ALBANY_EPETRA)
 
SET(SOURCES ${SOURCES} 
    AAdapt_TopologyModification.cpp
    )
SET(HEADERS ${HEADERS}
    AAdapt_TopologyModification.hpp
    )
    
ENDIF()

ENDIF()

IF(ALBANY_LCM AND ALBANY_STK_PERCEPT)

SET(SOURCES ${SOURCES}
    AAdapt_STKAdaptT.cpp
    AAdapt_STKUnifSizeField.cpp
   )

SET(HEADERS ${HEADERS}
    AAdapt_STKAdaptT.hpp
    AAdapt_STKAdaptT_Def.hpp
    AAdapt_STKUnifSizeField.hpp
   )

IF (ALBANY_EPETRA) 
SET(SOURCES ${SOURCES}
    AAdapt_STKAdapt.cpp
    )
SET(HEADERS ${HEADERS}
    AAdapt_STKAdapt.hpp
    AAdapt_STKAdapt_Def.hpp
)
ENDIF()

ENDIF()

# Mesh Adapt depends on SCOREC tools and FMDB
IF(ALBANY_SCOREC)

SET(SOURCES ${SOURCES}
    AAdapt_MeshAdapt.cpp
    AAdapt_UnifSizeField.cpp
    AAdapt_UnifRefSizeField.cpp
   )

SET(HEADERS ${HEADERS}
    AAdapt_MeshAdapt.hpp
    AAdapt_MeshAdapt_Def.hpp
    AAdapt_UnifSizeField.hpp
    AAdapt_UnifRefSizeField.hpp
   )

IF(ENABLE_SPR)
SET(SOURCES ${SOURCES} AAdapt_SPRSizeField.cpp)
SET(HEADERS ${HEADERS} AAdapt_SPRSizeField.hpp)
ENDIF()

include_directories(${ALBANY_FMDB_INCLUDE_DIRS} ${FMDB_INCLUDE_DIR})

ENDIF()

include_directories(${Trilinos_INCLUDE_DIRS}  ${Trilinos_TPL_INCLUDE_DIRS}
                    ${Albany_SOURCE_DIR} / src ${Albany_SOURCE_DIR} / src / evaluators
                    ${Albany_SOURCE_DIR} / src / problems ${Albany_SOURCE_DIR} / src / responses
                    ${Albany_SOURCE_DIR} / src / disc
                    ${Albany_SOURCE_DIR} / src / disc / stk ${Albany_SOURCE_DIR} / src / LCM /
                    ${Albany_SOURCE_DIR} / src / LCM / evaluators ${Albany_SOURCE_DIR} / src / LCM / utils
                   )

add_library(albanyAdapt ${Albany_LIBRARY_TYPE} ${SOURCES} ${HEADERS})

set_target_properties(albanyAdapt PROPERTIES PUBLIC_HEADER "${HEADERS}")

IF(INSTALL_ALBANY)
install(TARGETS albanyAdapt EXPORT albany-export
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}/"
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}/"
        PUBLIC_HEADER DESTINATION "${INCLUDE_INSTALL_DIR}")
ENDIF()
