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

include_directories(${Ah5_INCLUDE_PATH})
find_library(M_LIB m)

add_executable(test_ah5_c_shared test_ah5.c)
target_link_libraries(test_ah5_c_shared ah5.shared ${M_LIB})
append_property(TARGET test_ah5_c_shared PROPERTY LINK_FLAGS " ${Ah5_LINK_FLAGS} ")
add_test(NAME test_ah5_c_shared COMMAND test_ah5_c_shared)

add_executable(test_ah5_c_static test_ah5.c)
target_link_libraries(test_ah5_c_static ah5.static ${M_LIB})
append_property(TARGET test_ah5_c_static PROPERTY LINK_FLAGS " ${Ah5_LINK_FLAGS} ")
add_test(NAME test_ah5_c_static COMMAND test_ah5_c_static)

add_executable(test_ah5_f_shared test_ah5.F90)
target_link_libraries(test_ah5_f_shared ah5.shared ah5_f90.shared ${M_LIB})
append_property(TARGET test_ah5_f_shared PROPERTY LINK_FLAGS " ${Ah5_LINK_FLAGS} ")
add_test(NAME test_ah5_f_shared COMMAND test_ah5_f_shared)

add_executable(test_ah5_f_static test_ah5.F90)
target_link_libraries(test_ah5_f_static ah5.static ah5_f90.static ${M_LIB})
append_property(TARGET test_ah5_f_static PROPERTY LINK_FLAGS " ${Ah5_LINK_FLAGS} ")
add_test(NAME test_ah5_f_static COMMAND test_ah5_f_static)
