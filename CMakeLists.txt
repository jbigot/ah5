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
# protect the project call to workaround a cmake bug
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	project(Ah5 C Fortran)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(AppendProperty)
include(FortranCInterface)
find_package(HDF5 REQUIRED)
find_package(OMP)

option(ENABLE_FORTRAN "Enables the generation of the Fortran wrapper for async HDF5" ON)
option(ENABLE_OPENMP  "Enables support for parallel copy with OpenMP" ON)

file(READ VERSION Ah5_VERSION)

include_directories("src/" "include/" "${CMAKE_CURRENT_BINARY_DIR}" ${HDF5_Fortran_INCLUDE_PATH} ${HDF5_C_INCLUDE_PATH})

set(SRC_Ah5 src/ah5.c)
append_property(SOURCE ${SRC_Ah5}
	PROPERTY COMPILE_FLAGS
	" ${HDF5_C_COMPILE_FLAGS} ${OMP_C_COMPILE_FLAGS}")

add_library(ah5.static STATIC ${SRC_Ah5})
target_link_libraries(ah5.static ${HDF5_C_LIBRARIES})

add_library(ah5.shared SHARED ${SRC_Ah5})
target_link_libraries(ah5.shared ${HDF5_SHARED_C_LIBRARIES})

append_property(TARGET ah5.static ah5.shared PROPERTY LINK_FLAGS " ${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS} ")
set(Ah5_LINK_FLAGS " ${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS} ")
set_property(TARGET ah5.static ah5.shared PROPERTY OUTPUT_NAME ah5)
install(TARGETS ah5.static ah5.shared DESTINATION lib EXPORT Ah5Lib)
set(AH5_EXPORT_TARGET ah5.static ah5.shared)

install(FILES include/ah5.h DESTINATION include)

if ( ENABLE_FORTRAN )
	add_subdirectory(vendor/bpp bpp EXCLUDE_FROM_ALL)

	bpp_preprocess(SRC_Ah5_F90
		src/ah5.F90.bpp
	)
	add_custom_target(bpp_file DEPENDS "${SRC_Ah5_F90}") # to serialize src generation

	append_property(SOURCE ${SRC_Ah5_F90}
		PROPERTY COMPILE_FLAGS
		" ${OMP_Fortran_COMPILE_FLAGS}")

	add_library(ah5_f90.static STATIC ${SRC_Ah5_F90})
	add_dependencies(ah5_f90.static bpp_file) # to serialize src generation
	append_property(TARGET ah5_f90.static PROPERTY COMPILE_FLAGS " ${HDF5_Fortran_COMPILE_FLAGS} ")
	target_link_libraries(ah5_f90.static ah5.static ${HDF5_Fortran_LIBRARIES} ${HDF5_C_LIBRARIES})
	set_property(TARGET ah5_f90.static PROPERTY Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modinclude")

	add_library(ah5_f90.shared SHARED ${SRC_Ah5_F90})
	add_dependencies(ah5_f90.shared bpp_file) # to serialize src generation
	append_property(TARGET ah5_f90.shared PROPERTY COMPILE_FLAGS " ${HDF5_SHARED_Fortran_COMPILE_FLAGS} ")
	target_link_libraries(ah5_f90.shared ah5.shared ${HDF5_SHARED_Fortran_LIBRARIES} ${HDF5_SHARED_C_LIBRARIES})

	append_property(TARGET ah5_f90.static ah5_f90.shared PROPERTY LINK_FLAGS " ${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS} ${HDF5_Fortran_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS} ")
	set(Ah5_LINK_FLAGS " ${Ah5_LINK_FLAGS} ${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS} ${HDF5_Fortran_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS} ")
	set_property(TARGET ah5_f90.static ah5_f90.shared PROPERTY OUTPUT_NAME ah5_f90)
	install(TARGETS ah5_f90.static ah5_f90.shared DESTINATION lib EXPORT Ah5Lib)
	set(AH5_EXPORT_TARGET ${AH5_EXPORT_TARGET} ah5_f90.static ah5_f90.shared)
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modinclude" DESTINATION include FILES_MATCHING PATTERN "*.mod")
endif()


set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}")
configure_file(cmake/Ah5ConfigVersion.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/Ah5ConfigVersion.cmake" @ONLY)


# installable config

set(Ah5_INCLUDE_PATH "${CMAKE_INSTALL_PREFIX}/include")
install(EXPORT Ah5Lib DESTINATION "lib/CMake/Ah5" )
configure_file(cmake/Ah5Config.cmake.in "${WORK_DIR}/Ah5Config.cmake" @ONLY)
install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/Ah5ConfigVersion.cmake"
		"${WORK_DIR}/Ah5Config.cmake"
	DESTINATION "lib/CMake/Ah5"
)


# in tree config

set(Ah5_INCLUDE_PATH ${HDF5_C_INCLUDE_PATH} "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/modinclude" CACHE STRING "Ah5 Include path" FORCE)
set(Ah5_LINK_FLAGS   "${Ah5_LINK_FLAGS}" CACHE STRING "Ah5 link flags" FORCE)
export(TARGETS ${AH5_EXPORT_TARGET} FILE "${CMAKE_CURRENT_BINARY_DIR}/Ah5Lib.cmake")
configure_file(cmake/Ah5Config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/Ah5Config.cmake" @ONLY)
include("${CMAKE_CURRENT_BINARY_DIR}/Ah5Config.cmake")
