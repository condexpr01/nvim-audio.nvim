set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.16)

# debug
if(OFF)
	set(CMAKE_CXX_COMPILER clang++)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
	set(CMAKE_BUILD_TYPE Debug)
endif()

#proj, ver
set(PROJECT_NAME "nvim_audio_lib")
project(${PROJECT_NAME} LANGUAGES C CXX)

#static crt
if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()


#########################
#  try jump compile     #
#########################
if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
	set(ARCH_TAG "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
elseif(CMAKE_C_COMPILER_ARCHITECTURE_ID)
	set(ARCH_TAG "${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
elseif(CMAKE_OSX_ARCHITECTURES)
	set(ARCH_TAG "${CMAKE_OSX_ARCHITECTURES}")
endif()

# fallback
if(NOT ARCH_TAG)
	set(ARCH_TAG "${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "CXX_ARCHITECTURE: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
message(STATUS "C_ARCHITECTURE: ${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
message(STATUS "ARCH_TAG: ${ARCH_TAG}")

set(PREBUILD_NAME "${CMAKE_SYSTEM_NAME}-${ARCH_TAG}-${PROJECT_NAME}")

set(PREBUILD_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
set(PREBUILD_BIN_DIR "${CMAKE_SOURCE_DIR}/precompile")
set(NVIM_LUA_REQUIRE_DIR "${CMAKE_SOURCE_DIR}/lua")

#generate flatform bin name
set(PREBUILD_BIN "${PREBUILD_BIN_DIR}/${PREBUILD_NAME}${PREBUILD_SUFFIX}")

set(PREBUILD_BIN_URL
	"https://github.com/condexpr01/nvim-audio.nvim/releases/latest/download/${PREBUILD_NAME}${PREBUILD_SUFFIX}"
)

message(STATUS "Downloading ${PREBUILD_BIN_URL}")

file(DOWNLOAD
    "${PREBUILD_BIN_URL}"
    "${PREBUILD_BIN}"
    STATUS DOWNLOAD_STATUS
    SHOW_PROGRESS
    INACTIVITY_TIMEOUT 60
)

list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)

if(NOT STATUS_CODE EQUAL 0)
    file(REMOVE "${PREBUILD_BIN}")
    message(STATUS 
		"Download failed (code ${STATUS_CODE}): ${ERROR_MESSAGE}"
	)
endif()

#check bin name
if(EXISTS "${PREBUILD_BIN}")
	message(STATUS "############################################")
	message(STATUS "#                                          #")
	message(STATUS "# Luckey! There's a available prebuild bin #")
	message(STATUS "#                                          #")
	message(STATUS "############################################")

	install(FILES "${PREBUILD_BIN}" 
		DESTINATION "${NVIM_LUA_REQUIRE_DIR}"
		RENAME "${PROJECT_NAME}${PREBUILD_SUFFIX}"
	)
	return()
else()
	message(STATUS "############################################")
	message(STATUS "#  No prebuild bin and will try to compile #")
	message(STATUS "############################################")

endif()


################
#   normal     #
################
#executable target
add_library(${PROJECT_NAME} MODULE)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)

#sdl audio hpp, src
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_sources(${PROJECT_NAME} PRIVATE src/audio-impl.cpp)


################
#   flags      #
################
#msvc utf8
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"
	AND CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
	add_compile_options("/utf-8")
endif()

if(WIN32)
	if(MINGW)
		target_link_options(${PROJECT_NAME} PRIVATE -static -s)
	endif()

	#target_link_options(${PROJECT_NAME} PRIVATE -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic)

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT APPLE)
	target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()


# allow undefined symbles settings
if(WIN32)
else()#POSIX, etc.
	if(APPLE)
		target_link_options(${PROJECT_NAME} PRIVATE "-undefined" "dynamic_lookup")
	endif()
endif()


################
find_program(GIT_EXECUTABLE git git.cmd)

set(SDL_REPO "https://github.com/libsdl-org/SDL.git")
#set(SDL_REPO "git@github.com:libsdl-org/SDL.git")
#set(SDL_REPO "file:////home/condexpr01/Desktop/clonerepos/SDL/.git")


################
#   SDL3       #
################
find_package(SDL3)
if(SDL3_FOUND)
	if(TARGET SDL3::SDL3-static)
		target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static)
	else()
		target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
	endif()

elseif(GIT_EXECUTABLE)

	set(SDL_CLONE_DIR "${CMAKE_SOURCE_DIR}/clone/SDL")

	if(NOT EXISTS "${SDL_CLONE_DIR}/.git")
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" clone
				--depth=1
				--branch release-3.4.8
				"${SDL_REPO}"
				--
				"${SDL_CLONE_DIR}"
		)
	endif()

	set(SDL_UNIX_CONSOLE_BUILD  ON  CACHE BOOL "" FORCE)
	set(SDL_AUDIO               ON  CACHE BOOL "" FORCE)
	set(SDL_THREADS             ON  CACHE BOOL "" FORCE)
	set(SDL_TIMERS              ON  CACHE BOOL "" FORCE)
	set(SDL_STATIC              ON  CACHE BOOL "" FORCE)
	set(SDL_SHARED              OFF CACHE BOOL "" FORCE)
	set(SDL_TEST                OFF CACHE BOOL "" FORCE)
	set(SDL_TESTS               OFF CACHE BOOL "" FORCE)
	set(SDL_EXAMPLES            OFF CACHE BOOL "" FORCE)
	set(SDL_INSTALL_TESTS       OFF CACHE BOOL "" FORCE)
	set(SDL_CAMERA              OFF CACHE BOOL "" FORCE)
	set(SDL_JOYSTICK            OFF CACHE BOOL "" FORCE)
	set(SDL_HAPTIC              OFF CACHE BOOL "" FORCE)
	set(SDL_SENSOR              OFF CACHE BOOL "" FORCE)
	set(SDL_GAMEPAD             OFF CACHE BOOL "" FORCE)
	set(SDL_TRAY                OFF CACHE BOOL "" FORCE)
	set(SDL_HIDAPI              OFF CACHE BOOL "" FORCE)
	set(SDL_POWER               OFF CACHE BOOL "" FORCE)
	set(SDL_DIALOG              OFF CACHE BOOL "" FORCE)
	set(SDL_SYSTEM              OFF CACHE BOOL "" FORCE)
	set(SDL_LOCALE              OFF CACHE BOOL "" FORCE)
	set(SDL_MISC                OFF CACHE BOOL "" FORCE)
	set(SDL_FILESYSTEM          OFF CACHE BOOL "" FORCE)
	set(SDL_LOADSO              OFF CACHE BOOL "" FORCE)
	set(SDL_CPUINFO             OFF CACHE BOOL "" FORCE)
	set(SDL_RENDER              OFF CACHE BOOL "" FORCE)
	set(SDL_VIDEO               OFF CACHE BOOL "" FORCE)
	set(SDL_EVENTS              OFF CACHE BOOL "" FORCE)

	set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
	add_subdirectory("${SDL_CLONE_DIR}")

	#SDL3::SDL3 is always available
	#turn off shared and turn on static, so SDL3::SDL3 will be SDL3::SDL3-static
	#target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static)
	target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

else()
	message(FATAL_ERROR "NO SDL3 FOUND, AND NO GIT FOR CLONING")

endif()



################
#   install    #
################
install(TARGETS ${PROJECT_NAME} DESTINATION "${NVIM_LUA_REQUIRE_DIR}")

#prebuild bin
install(FILES $<TARGET_FILE:${PROJECT_NAME}>
	DESTINATION "${PREBUILD_BIN_DIR}"
	RENAME "${PREBUILD_NAME}${PREBUILD_SUFFIX}"
)
