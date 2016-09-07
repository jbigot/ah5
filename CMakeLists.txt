################################################################################
# Copyright (c) 2013-2014, Julien Bigot - CEA (julien.bigot@cea.fr)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
################################################################################

cmake_minimum_required(VERSION 2.8)
project(Ah5 C Fortran)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(POLICY CMP0063)
cmake_policy(SET CMP0063 NEW)
endif()

include(AddCCompilerFlags)
include(AppendProperty)
include(FortranCInterface)


# Build options

option(ENABLE_FORTRAN
	"Enable compilation of the Fortran version of the library"
	ON)
option(ENABLE_OPENMP
	"Enables support for parallel copy with OpenMP"
	ON)
option(ENABLE_TESTING
	"Enable compilation of the (example) tests"
	OFF)

get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
if("${ENABLE_FORTRAN}" AND NOT "${ENABLED_LANGUAGES}" MATCHES Fortran)
	message(FATAL_ERROR "Fortran language support disabled can not build Fortran lib, set ENABLE_FORTRAN to OFF")
endif()


# Find dependancies

find_package(HDF5 REQUIRED)
if("${ENABLE_OPENMP}")
	find_package(OMP REQUIRED)
endif()


# Version

file(READ VERSION Ah5_VERSION)
string(STRIP "${Ah5_VERSION}" Ah5_VERSION)
string(REPLACE "." ";" Ah5_VERSION_list ${Ah5_VERSION})
list(GET Ah5_VERSION_list 0 Ah5_VERSION_MAJOR)
list(GET Ah5_VERSION_list 1 Ah5_VERSION_MINOR)
list(GET Ah5_VERSION_list 2 Ah5_VERSION_PATCH)


# Libraries

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
include_directories("include/" ${HDF5_Fortran_INCLUDE_PATH} ${HDF5_C_INCLUDE_PATH})
install(DIRECTORY "include/" DESTINATION include)

#C static version
add_libr
)
add_compiler_flags(ah5.static PRIVATE -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -pedantic-errors)
set_property(TARGET ah5.static PROPERTY OUTPUT_NAME ah5)
set_property(TARGET ah5.static PROPERTY VERSION ${Ah5_VERSION})
set_property(TARGET ah5.static PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.static PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.static APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
target_link_libraries(ah5.static ${HDF5_C_LIBRARIES})
string(REPLACE ";" " " Ah5_STATIC_COMPILE_FLAGS "${HDF5_C_COMPILE_FLAGS} ${OMP_C_COMPILE_FLAGS}")
append_property(TARGET ah5.static PROPERTY COMPILE_FLAGS " ${Ah5_STATIC_COMPILE_FLAGS}")
string(REPLACE ";" " " Ah5_STATIC_LINK_FLAGS "${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS}")
append_property(TARGET ah5.static PROPERTY LINK_FLAGS " ${Ah5_STATIC_LINK_FLAGS}")
install(TARGETS ah5.static EXPORT Ah5_export
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
# 	INCLUDES DESTINATION include
)

#C shared version
add_libr
)
add_compiler_flags(ah5.shared PRIVATE -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -pedantic-errors)
set_property(TARGET ah5.shared PROPERTY OUTPUT_NAME ah5)
set_property(TARGET ah5.shared PROPERTY VERSION ${Ah5_VERSION})
set_property(TARGET ah5.shared PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.shared PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.shared APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
target_link_libraries(ah5.shared ${HDF5_SHARED_C_LIBRARIES})
string(REPLACE ";" " " Ah5_SHARED_COMPILE_FLAGS "${HDF5_SHARED_C_COMPILE_FLAGS} ${OMP_C_COMPILE_FLAGS}")
append_property(TARGET ah5.shared PROPERTY COMPILE_FLAGS " ${Ah5_SHARED_COMPILE_FLAGS}")
string(REPLACE ";" " " Ah5_SHARED_LINK_FLAGS "${HDF5_SHARED_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS}")
append_property(TARGET ah5.shared PROPERTY LINK_FLAGS " ${Ah5_SHARED_LINK_FLAGS}")
install(TARGETS ah5.shared EXPORT Ah5_export
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
# 	INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)

#Fortran version
if("${ENABLE_FORTRAN}")
	add_subdirectory(vendor/bpp bpp EXCLUDE_FROM_ALL)
	bpp_preprocess(SRC_Ah5_F90 src/ah5.F90.bpp)
	add_custom_target(bpp_file DEPENDS "${SRC_Ah5_F90}") # to serialize src generation
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/finclude/"
		DESTINATION "lib/Ah5-${Ah5_VERSION}/finclude"
	)

	#Fortran static version
	add_library(ah5_f90.static STATIC
		${SRC_Ah5_F90}
	)
	add_dependencies(ah5_f90.static bpp_file) # to serialize src generation
	set_property(TARGET ah5_f90.static PROPERTY Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/finclude")
	set_property(TARGET ah5_f90.static PROPERTY OUTPUT_NAME ah5_f90)
	set_property(TARGET ah5_f90.static PROPERTY VERSION ${Ah5_VERSION})
	set_property(TARGET ah5_f90.static PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
	set_property(TARGET ah5_f90.static PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
	set_property(TARGET ah5_f90.static APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
	target_link_libraries(ah5_f90.static ah5.static ${HDF5_Fortran_LIBRARIES})
	string(REPLACE ";" " " Ah5_Fortran_STATIC_COMPILE_FLAGS "${HDF5_Fortran_COMPILE_FLAGS} ${OMP_Fortran_COMPILE_FLAGS}")
	append_property(TARGET ah5_f90.static PROPERTY COMPILE_FLAGS " ${Ah5_Fortran_STATIC_COMPILE_FLAGS}")
	string(REPLACE ";" " " Ah5_Fortran_STATIC_LINK_FLAGS "${HDF5_Fortran_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS}")
	append_property(TARGET ah5_f90.static PROPERTY LINK_FLAGS " ${Ah5_Fortran_STATIC_LINK_FLAGS}")
	install(TARGETS ah5_f90.static EXPORT Ah5_export
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
# 		INCLUDES DESTINATION "lib/Ah5-${Ah5_VERSION}/finclude"
	)
	
	#Fortran shared version
	add_library(ah5_f90.shared SHARED
		${SRC_Ah5_F90}
	)
	add_dependencies(ah5_f90.shared bpp_file) # to serialize src generation
	set_property(TARGET ah5_f90.shared PROPERTY Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/trash")
	set_property(TARGET ah5_f90.shared PROPERTY OUTPUT_NAME ah5_f90)
	set_property(TARGET ah5_f90.shared PROPERTY VERSION ${Ah5_VERSION})
	set_property(TARGET ah5_f90.shared PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
	set_property(TARGET ah5_f90.shared PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
	set_property(TARGET ah5_f90.shared APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
	target_link_libraries(ah5_f90.shared ah5.shared ${HDF5_SHARED_Fortran_LIBRARIES})
	add_dependencies(ah5_f90.shared ah5_f90.static) # for the .mod
	string(REPLACE ";" " " Ah5_Fortran_SHARED_COMPILE_FLAGS "${HDF5_SHARED_Fortran_COMPILE_FLAGS} ${OMP_Fortran_COMPILE_FLAGS}")
	append_property(TARGET ah5_f90.static PROPERTY COMPILE_FLAGS " ${Ah5_Fortran_SHARED_COMPILE_FLAGS}")
	string(REPLACE ";" " " Ah5_Fortran_SHARED_LINK_FLAGS "${HDF5_SHARED_Fortran_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS}")
	append_property(TARGET ah5_f90.static PROPERTY LINK_FLAGS " ${Ah5_Fortran_SHARED_LINK_FLAGS}")
	install(TARGETS ah5_f90.shared EXPORT Ah5_export
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
# 		INCLUDES DESTINATION "lib/Ah5-${Ah5_VERSION}/finclude"
	)
endif()


# Installable config

set(ConfigPackageLocation "lib/cmake/Ah5-${Ah5_VERSION}")
set(Ah5_INCLUDE_PATH ${HDF5_C_INCLUDE_PATH} "${CMAKE_INSTALL_PREFIX}/include" "${CMAKE_INSTALL_PREFIX}/lib/Ah5-${Ah5_VERSION}/finclude")
configure_file(cmake/Ah5Config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/Ah5Config.cmake" @ONLY)
configure_file(cmake/Ah5ConfigVersion.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/Ah5ConfigVersion.cmake" @ONLY)
install(EXPORT Ah5_export DESTINATION ${ConfigPackageLocation})
install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/Ah5Config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/Ah5ConfigVersion.cmake"
	DESTINATION ${ConfigPackageLocation}
)


# In-tree config

set(MAY_PARENT_SCOPE)
if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
	string(REPLACE MAY_ "" MAY_PARENT_SCOPE MAY_PARENT_SCOPE)
endif()

set(Ah5_INCLUDE_PATH
	${HDF5_C_INCLUDE_PATH}
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_BINARY_DIR}/finclude"
	${MAY_PARENT_SCOPE})
set(Ah5_STATIC_LINK_FLAGS            "${Ah5_STATIC_LINK_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_STATIC_COMPILE_FLAGS         "${Ah5_STATIC_COMPILE_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_SHARED_LINK_FLAGS            "${Ah5_SHARED_LINK_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_SHARED_COMPILE_FLAGS         "${Ah5_SHARED_COMPILE_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_Fortran_STATIC_LINK_FLAGS    "${Ah5_Fortran_STATIC_LINK_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_Fortran_STATIC_COMPILE_FLAGS "${Ah5_Fortran_STATIC_COMPILE_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_Fortran_SHARED_LINK_FLAGS    "${Ah5_Fortran_SHARED_LINK_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_Fortran_SHARED_COMPILE_FLAGS "${Ah5_Fortran_SHARED_COMPILE_FLAGS}" ${MAY_PARENT_SCOPE})
set(Ah5_LINK_FLAGS "${Ah5_LINK_FLAGS}" ${MAY_PARENT_SCOPE})


# Test examples

if("${ENABLE_TESTING}")
	enable_testing()
	add_subdirectory(examples)
	add_subdirectory(tests)
endif()
