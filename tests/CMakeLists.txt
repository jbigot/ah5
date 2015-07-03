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
project(Ah5_tests C Fortran)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/")

add_subdirectory(.. ah5)

include(AppendProperty)

# find_package(Ah5 REQUIRED)
find_package(OMP REQUIRED)

include_directories(${Ah5_INCLUDE_PATH})

add_executable(test_ah5_c test_ah5.c)
append_property(TARGET test_ah5_c PROPERTY LINK_FLAGS " ${OMP_C_LINK_FLAGS}")
find_library(M_LIB m)
target_link_libraries(test_ah5_c ah5.shared ${M_LIB})
# or target_link_libraries(test_ah5_c ah5.static)

add_executable(test_ah5_f test_ah5.F90)
append_property(TARGET test_ah5_f PROPERTY LINK_FLAGS " ${OMP_Fortran_LINK_FLAGS}")
target_link_libraries(test_ah5_f ah5.static ah5_f90.static  ${HDF5_STATIC_Fortran_LIBRARIES} ${HDF5_STATIC_C_LIBRARIES})
# or target_link_libraries(test_ah5_f ah5.shared)
