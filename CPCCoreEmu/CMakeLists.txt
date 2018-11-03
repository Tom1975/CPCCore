cmake_minimum_required(VERSION 3.0)
project(CPCCoreEmu)

set(SRCS
	CAPSFile.cpp
	CRC.cpp
	DiskBuilder.cpp
	DiskContainer.cpp
	DiskGen.cpp
	FileAccess.cpp
	FormatTypeCTRAW.cpp
	FormatTypeEDSK.cpp
	FormatTypeDSK.cpp
	FormatTypeHFE.cpp
	FormatTypeIPF.cpp
	FormatTypeRAW.cpp
	FormatTypeSCP.cpp
	IDisk.cpp
	MediaManager.cpp
	)

set(HEADERS
	CAPSFile.h
	CRC.h
	DiskBuilder.h
	DiskContainer.h
	DiskGen.h
	DskTypeManager.h
	FDC765.h
	FileAccess.h
	FormatTypeCTRAW.h
	FormatTypeEDSK.h
	FormatTypeDSK.h
	FormatTypeHFE.h
	FormatTypeIPF.h
	FormatTypeRAW.h
	FormatTypeSCP.h
	IDisk.h
	ILoadingProgress.h
	ILog.h
	includes.h
	macro.h
	MediaManager.h
	stdafx.h
)

#Fichier ressource

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../zlib)
add_library(CPCCoreEmu ${SRCS} ${HEADERS})
target_include_directories( CPCCoreEmu PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../zlib)

message ("CPCCoreEmu" ${CPCCoreEmu} )

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )

if(CMAKE_COMPILER_IS_GNUCC)
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
endif()

