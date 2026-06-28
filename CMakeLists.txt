cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -lpthread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -lpthread")
endif()

project(SCL)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(NNG_TESTS OFF CACHE BOOL "Disable nng tests" FORCE)
add_subdirectory(third_party/nng-1.11)

find_package(Threads)
# UserCode dll
add_library(SCL SHARED ${CMAKE_SOURCE_DIR}/src/dll.cxx)
set_target_properties(SCL PROPERTIES SUFFIX ".so")
target_include_directories(SCL PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(SCL PRIVATE ${CMAKE_SOURCE_DIR}/third_party/not-enough-standards/include)
target_include_directories(SCL PRIVATE ${CMAKE_SOURCE_DIR}/third_party/Eigen)
if(WIN32)
    target_link_libraries(SCL PRIVATE ${CMAKE_SOURCE_DIR}/lib/UserFunctions.dll)
endif()
target_include_directories(SCL SYSTEM PRIVATE /usr/include)
target_link_libraries(SCL PRIVATE nng)
target_link_libraries(SCL PRIVATE ${CMAKE_DL_LIBS})
target_compile_definitions(SCL PRIVATE NNG_STATIC_LIB)
target_compile_definitions(SCL PRIVATE NNG_ELIDE_DEPRECATED)

