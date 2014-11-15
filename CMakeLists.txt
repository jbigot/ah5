cmake_minimum_required(VERSION 2.8)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# protect the project call to workaround a cmake bug
if ( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}" )
	project(Ah5 C Fortran)
endif()

include(AppendProperty)
include(FortranCInterface)
find_package(HDF5 REQUIRED)
find_package(OMP)
add_subdirectory(vendor/bpp bpp EXCLUDE_FROM_ALL)

option(ENABLE_FORTRAN "Enables the generation of the Fortran wrapper for async HDF5" ON)

include_directories("src/" "include/" "${CMAKE_CURRENT_BINARY_DIR}" ${HDF5_Fortran_INCLUDE_PATH} ${HDF5_C_INCLUDE_PATH})

set(SRC_AH5 src/ah5.c)
append_property(SOURCE ${SRC_AH5}
	PROPERTY COMPILE_FLAGS
	" ${HDF5_C_COMPILE_FLAGS} ${OMP_C_COMPILE_FLAGS}")

add_library(ah5.static STATIC ${SRC_AH5})
target_link_libraries(ah5.static ${HDF5_C_LIBRARIES})

add_library(ah5.shared SHARED ${SRC_AH5})
target_link_libraries(ah5.shared ${HDF5_SHARED_C_LIBRARIES})

append_property(TARGET ah5.static ah5.shared PROPERTY LINK_FLAGS " ${HDF5_C_LINK_FLAGS} ${OMP_C_LINK_FLAGS}")
set_property(TARGET ah5.static ah5.shared PROPERTY OUTPUT_NAME ah5)
install(TARGETS ah5.static ah5.shared DESTINATION lib EXPORT Ah5Lib)

install(FILES include/ah5.h DESTINATION include)

if ( ENABLE_FORTRAN )
	bpp_preprocess(SRC_AH5_F90 src/ah5.F90.bpp)
	append_property(SOURCE ${SRC_AH5_F90}
		PROPERTY COMPILE_FLAGS
		" ${HDF5_Fortan_COMPILE_FLAGS} ${OMP_Fortran_COMPILE_FLAGS}")
	add_custom_target(bpp_file DEPENDS "${SRC_AH5_F90}") # to serialize src generation

	add_library(ah5_f90.static STATIC ${SRC_AH5_F90})
	add_dependencies(ah5_f90.static bpp_file) # to serialize src generation
	append_property(TARGET ah5_f90.static PROPERTY COMPILE_FLAGS " ${HDF5_Fortran_COMPILE_FLAGS} ")
	target_link_libraries(ah5_f90.static ah5.static ${HDF5_Fortran_LIBRARIES} ${HDF5_C_LIBRARIES})

	add_library(ah5_f90.shared SHARED ${SRC_AH5_F90})
	add_dependencies(ah5_f90.shared bpp_file) # to serialize src generation
	append_property(TARGET ah5_f90.shared PROPERTY COMPILE_FLAGS " ${HDF5_SHARED_Fortran_COMPILE_FLAGS} ")
	target_link_libraries(ah5_f90.shared ah5.shared ${HDF5_SHARED_Fortran_LIBRARIES} ${HDF5_C_LIBRARIES})

	append_property(TARGET ah5_f90.static ah5_f90.shared PROPERTY LINK_FLAGS " ${HDF5_Fortran_LINK_FLAGS} ${HDF5_C_LINK_FLAGS} ${OMP_Fortran_LINK_FLAGS}")
	set_property(TARGET ah5_f90.static ah5_f90.shared PROPERTY OUTPUT_NAME ah5_f90)
	install(TARGETS ah5_f90.static ah5_f90.shared DESTINATION lib EXPORT Ah5Lib)

	set_property(TARGET ah5_f90.static PROPERTY Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modinclude")
	install(DIRECTORY "${CMAKE_Fortran_MODULE_DIRECTORY}/" DESTINATION include FILES_MATCHING PATTERN "*.mod")
endif()


set(WORK_DIR "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}")


# in tree config

set(AH5_INCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE STRING "AH5 Include path")
export(EXPORT Ah5Lib FILE "${PROJECT_BINARY_DIR}/Ah5Lib.cmake")
configure_file(cmake/Ah5Config.cmake.in "${PROJECT_BINARY_DIR}/Ah5Config.cmake" @ONLY)
export(PACKAGE Ah5)


# installable config

set(AH5_INCLUDE_PATH "${CMAKE_INSTALL_PREFIX}/include")
install(EXPORT Ah5Lib DESTINATION "lib/CMake/Ah5" )
configure_file(cmake/Ah5Config.cmake.in "${WORK_DIR}/Ah5Config.cmake" @ONLY)
install(FILES "${WORK_DIR}/Ah5Config.cmake" DESTINATION "lib/CMake/Ah5")

