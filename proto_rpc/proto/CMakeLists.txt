
find_package(Protobuf REQUIRED)

set(PROTO_FILES
    rpc.proto
)

#
# Add Library target with protobuf sources
#
add_library(rpc_proto ${PROTO_FILES})
target_link_libraries(rpc_proto
    PUBLIC
        protobuf::libprotobuf

)

target_include_directories(rpc_proto PUBLIC
${PROTOBUF_INCLUDE_DIRS} 
${CMAKE_CURRENT_BINARY_DIR})

#
# Compile protobuf and grpc files in mointor proto target to cpp
#
protobuf_generate(TARGET rpc_proto LANGUAGE cpp)

install(TARGETS  rpc_proto DESTINATION  ${PROJECT_BINARY_DIR}/lib)

