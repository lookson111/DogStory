cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
#include(CTest)
#include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)

conan_basic_setup(TARGETS)
add_definitions(-DBOOST_BEAST_USE_STD_STRING_VIEW)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(BOOST_LIB CONAN_PKG::boost)
set(ZLIB_LIB Threads::Threads)
set(CATCH2LIB CONAN_PKG::catch2)

add_library(collision_detection_lib STATIC
    src/model/collision_detector.h
    src/model/collision_detector.cpp
)
target_link_libraries(collision_detection_lib PUBLIC 
	${BOOST_LIB} 
	${ZLIB_LIB} 
)

add_library(util_lib STATIC
    src/util/tagged.h
    src/util/token.h
    src/util/tagged_uuid.h
    src/util/tagged_uuid.cpp
)
target_link_libraries(util_lib PUBLIC 
	${BOOST_LIB} 
	${ZLIB_LIB} 
)

# Добавляем библиотеку, указывая, что она статическая.
add_library(model_lib STATIC     
	src/model/model.h
    src/model/model.cpp
    src/model/dog.h
    src/model/dog.cpp
    src/model/extra_data.h
    src/model/extra_data.cpp
    src/model/loot_generator.h
    src/model/loot_generator.cpp
    src/model/geom.h
	src/sdk.h
)

# Добавляем сторонние библиотеки. Указываем видимость PUBLIC, т. к. 
# они должны быть ввидны и в библиотеке model_lib и в зависимостях.
target_include_directories(model_lib PUBLIC ${BOOST_LIB})
target_link_libraries(model_lib PUBLIC 
	${BOOST_LIB} 
	${ZLIB_LIB} 
	util_lib
	collision_detection_lib
)

add_library(posgres_sql_lib STATIC
	src/player.h
	src/player.cpp
    src/db/postgres.h
    src/db/postgres.cpp
	src/db/player_repository.h
	src/db/player_repository.cpp
	src/db/pool_connection.h
	src/db/pool_connection.cpp
)
target_link_libraries(posgres_sql_lib PUBLIC 
	${BOOST_LIB} 
	${ZLIB_LIB}
	CONAN_PKG::libpq 
	CONAN_PKG::libpqxx
	util_lib
)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler/request_handler.cpp
	src/request_handler/request_handler.h
	src/request_handler/api_request.cpp
	src/request_handler/api_request.h
	src/request_handler/file_request.cpp
	src/request_handler/file_request.h
	src/request_handler/base_request.cpp
	src/request_handler/base_request.h
	src/request_handler/response.cpp
	src/request_handler/response.h
	src/request_handler/uri_api.h
	src/request_handler/defs.h
	src/infrastructure/model_serialization.h
	src/infrastructure/app_serialization.h
	src/infrastructure/infrastructure.h
	src/infrastructure/infrastructure.cpp
	src/log.h
	src/log.cpp
	src/app.h
	src/app.cpp
	src/player.h
	src/player.cpp
	src/ticker.h
)

target_link_libraries(game_server PRIVATE 
	${BOOST_LIB} 
	${ZLIB_LIB} 
	model_lib
	posgres_sql_lib
) 

add_executable(game_server_tests
    tests/model_tests.cpp
    tests/loot_generator_tests.cpp
	tests/collision-detector-tests.cpp
	tests/state-serialization-tests.cpp
	tests/tagged_uuid_tests.cpp
	../src/json_loader.h
	../src/json_loader.cpp
	../src/app.cpp
	../src/app.h
	../src/player.cpp
	../src/player.h
)

target_link_libraries(game_server_tests PRIVATE 
	${CATCH2LIB} 
	${BOOST_LIB} 
	${ZLIB_LIB} 
	model_lib 
	posgres_sql_lib
) 

#catch_discover_tests(game_server_tests)
