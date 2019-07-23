################################################################################
# Copyright (C) 2015-2019 Commissariat a l'energie atomique et aux energies alternatives (CEA)
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

include(CMakeFindDependencyMacro)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}")
find_dependency(Threads)
find_dependency(yaml)

# by default, if no component is specified, look for all
if("xx" STREQUAL "x${paraconf_FIND_COMPONENTS}x")
	set(paraconf_FIND_COMPONENTS C f90)
endif()

# we always look for C
list(REMOVE_ITEM paraconf_FIND_COMPONENTS C) 
include("${CMAKE_CURRENT_LIST_DIR}/paraconf.cmake")
if(NOT TARGET paraconf::paraconf)
	set(paraconf_FOUND "FALSE")
	if(NOT "${paraconf_FIND_QUIETLY}")
		message("paraconf: component \"C\" not found")
	endif()
endif()

# currently, only f90 is supported
foreach(_paraconf_ONE_COMPONENT ${paraconf_FIND_COMPONENTS})
	include("${CMAKE_CURRENT_LIST_DIR}/paraconf_${_paraconf_ONE_COMPONENT}.cmake" OPTIONAL)
	if(NOT TARGET "paraconf::paraconf_${_paraconf_ONE_COMPONENT}")
		if("${paraconf_FIND_REQUIRED_${_paraconf_ONE_COMPONENT}}")
			set(paraconf_FOUND "FALSE")
		endif()
		if(NOT "${paraconf_FIND_QUIETLY}")
			message("paraconf: component \"${_paraconf_ONE_COMPONENT}\" not found")
		endif()
	endif()
endforeach()
