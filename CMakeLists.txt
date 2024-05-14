cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(
  aff3ct-frs
  LANGUAGES CXX
  DESCRIPTION "Folded Reed-Solomon codes with AFF3CT"
  VERSION 0.0.0.1)
# enable_testing()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(AFF3CT 3.0.2 REQUIRED)

add_library(aff3ct-frs SHARED Decoder_FRS.cpp)
target_link_libraries(aff3ct-frs PUBLIC aff3ct::aff3ct-static-lib)

add_executable(aff3ct-frs-run main.cpp Folding.cpp)
target_link_libraries(aff3ct-frs-run PUBLIC aff3ct-frs)
# add_test(NAME frs-run COMMAND aff3ct-frs-run)
