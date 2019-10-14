# Some common macros for the clients
file(WRITE ${CMAKE_BINARY_DIR}/replace_defines.cmake "
file(READ \${INFILE} _EIBLOADRESULT)
# Grep the define statements
string(REGEX MATCHALL \"#define (IMG_[A-Z_0-9]+)  +([0-9]+)\\n\" _EIBLOADRESULT_GREP \"\${_EIBLOADRESULT}\")
# Replace with const
string(REGEX REPLACE \"#define (IMG_[A-Z_0-9]+)  +([0-9]+)\\n\" \"\${REPLACE}\" _RESULTINC \"\${_EIBLOADRESULT_GREP}\")
if(SEMICOLON)
  file(WRITE \${OUTFILE} \"\${_RESULTINC};\\n\")
else()
  file(WRITE \${OUTFILE} \${_RESULTINC} \"\\n\")
endif()
")
macro(REPLACE_DEFINES REPLACE OUTFILE SEMICOLON)
  add_custom_command(OUTPUT ${OUTFILE}
    COMMAND ${CMAKE_COMMAND}
    -D INFILE=${PROJECT_SOURCE_DIR}/src/common/eibloadresult.h
    -D OUTFILE=${OUTFILE}
    -D REPLACE=${REPLACE}
    -D SEMICOLON=${SEMICOLON}
    -P ${CMAKE_BINARY_DIR}/replace_defines.cmake
    DEPENDS ${PROJECT_SOURCE_DIR}/src/common/eibloadresult.h #${CMAKE_BINARY_DIR}/replace_defines.cmake
    VERBATIM
    )
endmacro()

macro(PREPROCESS SOURCEFILE TARGETFILE DEPENDS)
  add_custom_command(OUTPUT ${TARGETFILE}
    COMMAND ${CMAKE_C_COMPILER} -E -I${CMAKE_CURRENT_SOURCE_DIR}/.. -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_BINARY_DIR}/.. -I${PROJECT_SOURCE_DIR}/src/include -P -x c
    -o ${TARGETFILE}
    ${SOURCEFILE}
    MAIN_DEPENDENCY ${SOURCEFILE}
    DEPENDS ${DEPENDS}
    )
endmacro()
macro(PREPROCESS_DEFAULT)
  preprocess(${CMAKE_CURRENT_SOURCE_DIR}/EIBConnection.pre ${CMAKE_CURRENT_BINARY_DIR}/EIBConnection.post ${CMAKE_CURRENT_BINARY_DIR}/result.inc)
endmacro()

# Concatenate
file(WRITE ${CMAKE_BINARY_DIR}/concat.cmake "
foreach(_I \${INFILES})
  file(READ \${_I} _DATA)
  set(OUTPUT \${OUTPUT}\${_DATA})
endforeach()
file(WRITE \${OUTFILE} \"\${OUTPUT}\")
")
macro(CONCATENATE RESULTFILE)
  add_custom_command(OUTPUT ${RESULTFILE}
    COMMAND ${CMAKE_COMMAND}
    -D "INFILES=${ARGN}"
    -D OUTFILE=${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE}
    -P ${CMAKE_BINARY_DIR}/concat.cmake
    DEPENDS ${ARGN} #${CMAKE_BINARY_DIR}/concat.cmake
    VERBATIM
    )
endmacro()

# For client generator building
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#add_subdirectory(def)
add_subdirectory(c)
add_subdirectory(php)
add_subdirectory(perl)
add_subdirectory(cs)
add_subdirectory(python)
add_subdirectory(pascal)
add_subdirectory(ruby)
add_subdirectory(lua)
add_subdirectory(go)

if(BUILD_JAVA)
  add_subdirectory(java)
endif()
