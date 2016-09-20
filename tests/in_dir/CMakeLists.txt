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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")

add_subdirectory(../.. Ah5 EXCLUDE_FROM_ALL)
find_library(M_LIB m)

add_executable(ah5_c_shared ../../examples/ah5_example.c)
target_link_libraries(ah5_c_shared ah5.shared ${M_LIB})

add_executable(ah5_c_static ../../examples/ah5_example.c)
target_link_libraries(ah5_c_static ah5.static ${M_LIB})

add_executable(ah5_f_shared ../../examples/ah5_example.F90)
target_link_libraries(ah5_f_shared ah5.shared ah5_f90.shared ${M_LIB})

add_executable(ah5_f_static ../../examples/ah5_example.F90)
target_link_libraries(ah5_f_static ah5.static ah5_f90.static ${M_LIB})
