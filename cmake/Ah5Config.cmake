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

include(CMakeFindDependencyMacro)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}")

find_dependency(HDF5)
if("${BUILD_OPENMP}")
	find_package(OMP REQUIRED)
endif()

# by default, if no component is specified, look for all
if("xx" STREQUAL "x${Ah5_FIND_COMPONENTS}x")
	set(Ah5_FIND_COMPONENTS C Fortran)
	set(Ah5_FIND_REQUIRED_C TRUE)
	set(Ah5_FIND_REQUIRED_Fortran FALSE)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/Ah5-targets.cmake")

if(NOT TARGET Ah5::Ah5_C)
	set(Ah5_FOUND "FALSE")
	if(NOT "${Ah5_FIND_QUIETLY}")
		message(WARNING "Ah5: component \"C\" not found")
	endif()
endif()

# currently, only Fortran optional component is supported
list(REMOVE_ITEM Ah5_FIND_COMPONENTS "C")
foreach(_Ah5_ONE_COMPONENT ${Ah5_FIND_COMPONENTS})
	if("x${_Ah5_ONE_COMPONENT}x" STREQUAL "xFortranx")
		if(NOT TARGET "Ah5::Ah5_Fortran")
			if("${Ah5_FIND_REQUIRED_Fortran}")
				if(NOT "${Ah5_FIND_QUIETLY}")
					message(SEND_ERROR "Ah5: required component \"Fortran\" not found")
				endif()
				set(Ah5_FOUND "FALSE")
			else()
				if(NOT "${Ah5_FIND_QUIETLY}")
					message(WARNING "Ah5: optional component \"Fortran\" not found")
				endif()
			endif()
		endif()
	else()
		if("${Ah5_FIND_REQUIRED_${_Ah5_ONE_COMPONENT}}")
			message(SEND_ERROR "Ah5: invalid required component \"${_Ah5_ONE_COMPONENT}\"")
			set(Ah5_FOUND "FALSE")
		else()
			if(NOT "${Ah5_FIND_QUIETLY}")
				message(WARNING "Ah5: invalid optional component \"${_Ah5_ONE_COMPONENT}\"")
			endif()
		endif()
	endif()
endforeach()
