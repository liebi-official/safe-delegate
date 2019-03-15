cmake_minimum_required(VERSION 3.5)
project(liebi-bank VERSION 1.0.1)

find_package(eosio.cdt)

### Generate the wasm and abi
add_contract( liebi.bank liebi.bank ${CMAKE_CURRENT_SOURCE_DIR}/src/bank.cpp)

target_include_directories(liebi.bank.wasm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

### add the path to where the ricardian contracts/clauses are found
target_compile_options( liebi.bank.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR} )
