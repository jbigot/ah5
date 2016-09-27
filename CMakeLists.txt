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

cmake_minimum_required(VERSION 3.0)
project(Ah5 C Fortran)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(POLICY CMP0063)
cmake_policy(SET CMP0063 NEW)
endif()

include(AddCCompilerFlags)
include(CMakePackageConfigHelpers)


# Build options

option(ENABLE_FORTRAN
	"Enable compilation of the Fortran version of the library"
	ON)
option(ENABLE_OPENMP
	"Enables support for parallel copy with OpenMP"
	ON)

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
install(DIRECTORY "include/" DESTINATION include)
set(Ah5_SRC 
	src/api.c
	src/command_list.c
	src/logging.c
	src/memhandling.c
	src/runner_thread.c
)


#C static version
add_library(ah5.static STATIC ${Ah5_SRC})
add_compiler_flags(ah5.static PRIVATE -pedantic -Wall -Wextra -Werror -Wfatal-errors -pedantic-errors)
set_property(TARGET ah5.static PROPERTY OUTPUT_NAME ah5)
set_property(TARGET ah5.static PROPERTY VERSION ${Ah5_VERSION})
set_property(TARGET ah5.static PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.static PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.static APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
target_link_libraries(ah5.static PUBLIC ${HDF5_C_LIBRARIES} ${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS})
target_compile_options(ah5.static PUBLIC ${HDF5_C_COMPILE_FLAGS} ${OMP_C_COMPILE_FLAGS})
target_include_directories(ah5.static PUBLIC
		${HDF5_C_INCLUDE_PATH}
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:include>")
target_compile_features(ah5.static PUBLIC c_variadic_macros)
install(TARGETS ah5.static EXPORT Ah5_export ARCHIVE DESTINATION lib)

#C shared version
add_library(ah5.shared SHARED ${Ah5_SRC})
add_compiler_flags(ah5.shared PRIVATE -pedantic -Wall -Wextra -Werror -Wfatal-errors -pedantic-errors)
set_property(TARGET ah5.shared PROPERTY OUTPUT_NAME ah5)
set_property(TARGET ah5.shared PROPERTY VERSION ${Ah5_VERSION})
set_property(TARGET ah5.shared PROPERTY SOVERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.shared PROPERTY Ah5_MAJOR_VERSION ${Ah5_VERSION_MAJOR})
set_property(TARGET ah5.shared APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Ah5_MAJOR_VERSION)
target_link_libraries(ah5.shared PUBLIC ${HDF5_SHARED_C_LIBRARIES} ${HDF5_SHARED_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS})
target_compile_options(ah5.shared PUBLIC ${HDF5_SHARED_C_COMPILE_FLAGS} ${OMP_C_COMPILE_FLAGS})
target_include_directories(ah5.shared PUBLIC
		${HDF5_C_INCLUDE_PATH}
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:include>")
target_compile_features(ah5.shared PUBLIC c_variadic_macros)
install(TARGETS ah5.shared EXPORT Ah5_export LIBRARY DESTINATION lib )

#Fortran version
if("${ENABLE_FORTRAN}")
	add_subdirectory(vendor/bpp bpp EXCLUDE_FROM_ALL)
	
	bpp_preprocess(SRC_Ah5_F90 src/ah5.F90.bpp)
	set(GENINC "${CMAKE_CURRENT_BINARY_DIR}/geninc")
	file(MAKE_DIRECTORY "${GENINC}")
	add_custom_target(Ah5_f90_inc ALL
			COMMAND "${CMAKE_COMMAND}" -E copy "${SRC_Ah5_F90}" "${GENINC}/Ah5.f90"
			DEPENDS "${SRC_Ah5_F90}"
			COMMENT "Renaming Fortran90 include file to 'Ah5.f90'"
			VERBATIM)
	install(FILES "${GENINC}/Ah5.f90" DESTINATION "include")
	
	#Fortran static version
	add_library(ah5_f90.static INTERFACE)
	add_dependencies(ah5_f90.static Ah5_f90_inc)
	target_link_libraries(ah5_f90.static INTERFACE ah5.static ${HDF5_Fortran_LIBRARIES} ${HDF5_Fortran_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS})
	target_compile_options(ah5_f90.static INTERFACE ${HDF5_Fortran_COMPILE_FLAGS} ${OMP_Fortran_COMPILE_FLAGS})
	target_include_directories(ah5_f90.static INTERFACE
				${HDF5_Fortran_INCLUDE_PATH}
			"$<BUILD_INTERFACE:${GENINC}>" 
			"$<INSTALL_INTERFACE:include>")
	install(TARGETS ah5_f90.static EXPORT Ah5_export LIBRARY DESTINATION lib )
	
	#Fortran shared version
	add_library(ah5_f90.shared INTERFACE)
	add_dependencies(ah5_f90.shared Ah5_f90_inc)
	target_link_libraries(ah5_f90.shared INTERFACE ah5.shared ${HDF5_SHARED_Fortran_LIBRARIES} ${HDF5_SHARED_Fortran_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS})
	target_compile_options(ah5_f90.static INTERFACE ${HDF5_SHARED_Fortran_COMPILE_FLAGS} ${OMP_Fortran_COMPILE_FLAGS})
	target_include_directories(ah5_f90.shared INTERFACE 
				${HDF5_Fortran_INCLUDE_PATH}
			"$<BUILD_INTERFACE:${GENINC}>" 
			"$<INSTALL_INTERFACE:include>")
	install(TARGETS ah5_f90.shared EXPORT Ah5_export LIBRARY DESTINATION lib )
	
endif()


# Test & examples

enable_testing()
add_subdirectory(examples)
add_subdirectory(tests)


# Installable config

set(ConfigPackageLocation "lib/cmake/Ah5-${Ah5_VERSION}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/Ah5ConfigVersion.cmake"
	VERSION ${Ah5_VERSION}
	COMPATIBILITY AnyNewerVersion
)
install(EXPORT Ah5_export DESTINATION ${ConfigPackageLocation})
install(FILES
		cmake/Ah5Config.cmake
		"${CMAKE_CURRENT_BINARY_DIR}/Ah5ConfigVersion.cmake"
	DESTINATION ${ConfigPackageLocation}
)
