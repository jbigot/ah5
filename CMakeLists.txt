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

cmake_minimum_required(VERSION 3.9)
project(Ah5 C Fortran)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


# Build options

option(BUILD_Fortran
	"Enable compilation of the Fortran version of the library"
	ON)
option(BUILD_OpenMP
	"Enables support for parallel copy with OpenMP"
	ON)


# Options handling

if("${BUILD_Fortran}")
	enable_language(Fortran)
	set(Fortran_COMPONENT "Fortran")
endif()


# Dependencies

include(CMakePackageConfigHelpers)
include(CTest)
include(GenerateExportHeader)
include(GNUInstallDirs)
find_package(HDF5 REQUIRED COMPONENTS C ${Fortran_COMPONENT})
if("${BUILD_OpenMP}")
	find_package(OpenMP REQUIRED)
	set(OpenMP_C_LIB "OpenMP::OpenMP_C")
endif()
if("${HDF5_IS_PARALLEL}")
	find_package(MPI REQUIRED COMPONENTS C ${Fortran_COMPONENT})
	list(APPEND HDF5_C_LIBRARIES "MPI::MPI_C")
	if("${BUILD_Fortran}")
		list(APPEND HDF5_Fortran_LIBRARIES "MPI::MPI_Fortran")
	endif()
endif()

# Version

file(READ VERSION Ah5_VERSION)
string(STRIP "${Ah5_VERSION}" Ah5_VERSION)
string(REPLACE "." ";" Ah5_VERSION_list ${Ah5_VERSION})
list(GET Ah5_VERSION_list 0 Ah5_VERSION_MAJOR)
list(GET Ah5_VERSION_list 1 Ah5_VERSION_MINOR)
list(GET Ah5_VERSION_list 2 Ah5_VERSION_PATCH)


# Libraries

## C version

add_library(Ah5_C
	src/ah5.c
)
generate_export_header(Ah5_C BASE_NAME ah5)
target_link_libraries(Ah5_C PUBLIC ${OpenMP_C_LIB} ${HDF5_C_LIBRARIES})
target_include_directories(Ah5_C PUBLIC
	"$<BUILD_INTERFACE:${Ah5_SOURCE_DIR}/include/>"
	${HDF5_C_INCLUDE_DIRS}
)
target_compile_definitions(Ah5_C PUBLIC ${HDF5_C_DEFINITIONS})
set_target_properties(Ah5_C PROPERTIES
	C_STANDARD 99
	C_STANDARD_REQUIRED TRUE
	C_VISIBILITY_PRESET hidden
	VERSION "${Ah5_VERSION}"
	SOVERSION "${Ah5_VERSION_MAJOR}"
	Ah5_MAJOR_VERSION "${Ah5_VERSION_MAJOR}")
set_property(TARGET Ah5_C APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
add_library(Ah5::Ah5_C ALIAS Ah5_C)
if("${CMAKE_VERSION}" VERSION_LESS "3.12")
	set(LIBRARY_COMPONENT "COMPONENT" "Runtime")
else()
	set(LIBRARY_COMPONENT "COMPONENT" "Runtime" "NAMELINK_COMPONENT" "Development")
endif()
install(TARGETS Ah5_C EXPORT AH5_EXPORT
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Development
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" ${LIBRARY_COMPONENT}
	INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(FILES include/ah5.h "${Ah5_BINARY_DIR}/ah5_export.h"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	COMPONENT Development
)


## Fortran version

if("${BUILD_Fortran}")
	add_subdirectory(vendor/bpp bpp EXCLUDE_FROM_ALL)
	bpp_preprocess(SRC_Ah5_Fortran src/ah5.F90.bpp)
	add_library(Ah5_Fortran ${SRC_Ah5_Fortran})
	target_link_libraries(Ah5_Fortran INTERFACE Ah5_C ${HDF5_Fortran_LIBRARIES})
	target_compile_definitions(Ah5_Fortran PUBLIC ${HDF5_Fortran_DEFINITIONS})
	target_include_directories(Ah5_Fortran PUBLIC
		"$<BUILD_INTERFACE:${Ah5_BINARY_DIR}/fortran_inc/>"
		${HDF5_Fortran_INCLUDE_DIRS})
	target_compile_definitions(Ah5_Fortran PUBLIC ${HDF5_Fortran_DEFINITIONS})
	set_property(TARGET Ah5_Fortran PROPERTY VERSION ${Ah5_VERSION})
	set_property(TARGET Ah5_Fortran PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
	set_property(TARGET Ah5_Fortran PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
	set_property(TARGET Ah5_Fortran PROPERTY Fortran_MODULE_DIRECTORY "${Ah5_BINARY_DIR}/fortran_inc/")
	add_library(Ah5::Ah5_Fortran ALIAS Ah5_Fortran)
	if(GNU STREQUAL "${CMAKE_Fortran_COMPILER_ID}")
		string(REGEX REPLACE "\\..*" "" COMPILER_FVER "${CMAKE_Fortran_COMPILER_VERSION}")
		set(INSTALL_FINCLUDEDIR "${CMAKE_INSTALL_LIBDIR}/gcc/${CMAKE_Fortran_LIBRARY_ARCHITECTURE}/${COMPILER_FVER}/finclude" CACHE PATH "Fortran module files (LIBDIR/gcc/${CMAKE_Fortran_LIBRARY_ARCHITECTURE}/${COMPILER_FVER}/finclude)")
	else()
		set(INSTALL_FINCLUDEDIR
			"${CMAKE_INSTALL_LIBDIR}/${CMAKE_Fortran_COMPILER_ID}/${CMAKE_Fortran_LIBRARY_ARCHITECTURE}/${CMAKE_Fortran_COMPILER_VERSION}/finclude"
			CACHE PATH
			"Fortran module files (LIBDIR/${CMAKE_Fortran_COMPILER_ID}/${CMAKE_Fortran_LIBRARY_ARCHITECTURE}/${CMAKE_Fortran_COMPILER_VERSION}/finclude)")
	endif()
	if("${CMAKE_VERSION}" VERSION_LESS "3.12")
		set(LIBRARY_COMPONENT "COMPONENT" "Runtime")
	else()
		set(LIBRARY_COMPONENT "COMPONENT" "Runtime" "NAMELINK_COMPONENT" "Development")
	endif()
	install(TARGETS Ah5_Fortran EXPORT AH5_EXPORT
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Development
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" ${LIBRARY_COMPONENT}
		INCLUDES DESTINATION "${INSTALL_FINCLUDEDIR}"
	)
	install(DIRECTORY "${Ah5_BINARY_DIR}/fortran_inc/"
		DESTINATION "${INSTALL_FINCLUDEDIR}"
		COMPONENT Development
	)
endif()


# CMake find

write_basic_package_version_file("${Ah5_BINARY_DIR}/Ah5ConfigVersion.cmake"
	VERSION ${Ah5_VERSION}
	COMPATIBILITY AnyNewerVersion
)
install(EXPORT AH5_EXPORT
	NAMESPACE "Ah5::"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/ah5/cmake"
	FILE Ah5-targets.cmake
	COMPONENT Development
)
install(FILES
	cmake/Ah5Config.cmake
	"${Ah5_BINARY_DIR}/Ah5ConfigVersion.cmake"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/ah5/cmake"
	COMPONENT Development
)


# Example

if("${BUILD_TESTING}")
	add_subdirectory(examples)
endif()
