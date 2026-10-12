cmake_minimum_required(VERSION 3.16)
project(canon_xl_controller LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(XL_SANITIZERS "Check memory and undefined behaviour in host tests" OFF)

add_library(xl_core
    canon_xl_controller/LensProtocol.cpp
    canon_xl_controller/LensController.cpp
    canon_xl_controller/SerialProtocol.cpp
    canon_xl_controller/Telemetry.cpp)
target_include_directories(xl_core PUBLIC canon_xl_controller)
target_compile_options(xl_core PRIVATE -Wall -Wextra -Werror -pedantic)
if(XL_SANITIZERS)
    target_compile_options(xl_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(xl_core PUBLIC -fsanitize=address,undefined)
endif()

enable_testing()
add_executable(protocol_tests tests/test_protocol.cpp)
target_link_libraries(protocol_tests PRIVATE xl_core)
target_compile_options(protocol_tests PRIVATE -Wall -Wextra -Werror -pedantic)
add_test(NAME protocol COMMAND protocol_tests)

add_executable(transport_tests tests/test_transport.cpp canon_xl_controller/Rp2040Transport.cpp)
target_include_directories(transport_tests PRIVATE tests/stubs)
target_compile_definitions(transport_tests PRIVATE ARDUINO_NANO_RP2040_CONNECT)
target_compile_options(transport_tests PRIVATE -Wall -Wextra -Werror -pedantic)
target_link_libraries(transport_tests PRIVATE xl_core)
add_test(NAME transport_model COMMAND transport_tests)

add_executable(sketch_tests tests/test_sketch.cpp canon_xl_controller/Rp2040Transport.cpp)
target_include_directories(sketch_tests PRIVATE tests/stubs)
target_compile_definitions(sketch_tests PRIVATE ARDUINO_NANO_RP2040_CONNECT)
target_compile_options(sketch_tests PRIVATE -Wall -Wextra -Werror -pedantic)
target_link_libraries(sketch_tests PRIVATE xl_core)
add_test(NAME sketch_model COMMAND sketch_tests)

add_executable(avr_tests tests/test_avr.cpp canon_xl_controller/AvrTransport.cpp)
target_include_directories(avr_tests PRIVATE tests/stubs)
target_compile_definitions(avr_tests PRIVATE ARDUINO_AVR_NANO __AVR_ATmega328P__ F_CPU=16000000UL)
target_compile_options(avr_tests PRIVATE -Wall -Wextra -Werror -pedantic)
target_link_libraries(avr_tests PRIVATE xl_core)
add_test(NAME avr_model COMMAND avr_tests)
