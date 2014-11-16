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

# compatibility macro for cmake pre 2.8.6

function(APPEND_PROPERTY TYPE)
	set(APPEND_PROPERTY_TARGETS)
	set(APPEND_PROPERTY_PROPNAME)
	set(APPEND_PROPERTY_VALUES)
	set(APPEND_PROPERTY_CURRENT APPEND_PROPERTY_TARGETS)
	foreach(ARG ${ARGN})
		if("${APPEND_PROPERTY_CURRENT}" STREQUAL "APPEND_PROPERTY_PROPNAME")
			set(APPEND_PROPERTY_PROPNAME "${ARG}")
			set(APPEND_PROPERTY_CURRENT APPEND_PROPERTY_VALUES)
		elseif("${ARG}" STREQUAL "PROPERTY")
			set(APPEND_PROPERTY_CURRENT APPEND_PROPERTY_PROPNAME)
		else()
			list(APPEND "${APPEND_PROPERTY_CURRENT}" "${ARG}")
		endif()
	endforeach()
	foreach(TARGET ${APPEND_PROPERTY_TARGETS})
		get_property(APPEND_PROPERTY_PREVIOUS_VALUE "${TYPE}" "${TARGET}" PROPERTY "${APPEND_PROPERTY_PROPNAME}")
		set_property("${TYPE}" "${TARGET}" PROPERTY "${APPEND_PROPERTY_PROPNAME}" "${APPEND_PROPERTY_PREVIOUS_VALUE}${APPEND_PROPERTY_VALUES}")
	endforeach()
endfunction()
