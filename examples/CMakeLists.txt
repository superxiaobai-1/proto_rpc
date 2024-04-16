
find_package(Protobuf REQUIRED)

set(PROTO_FILES
    monitor.proto
)

#
# Add Library target with protobuf sources
#
add_library(monitor_proto ${PROTO_FILES})
target_link_libraries(monitor_proto
    PUBLIC
        protobuf::libprotobuf

)

target_include_directories(monitor_proto PUBLIC
${PROTOBUF_INCLUDE_DIRS} 
${CMAKE_CURRENT_BINARY_DIR})

#
# Compile protobuf and grpc files in mointor proto target to cpp
#
protobuf_generate(TARGET monitor_proto LANGUAGE cpp)

install(TARGETS monitor_proto
  DESTINATION ${PROJECT_BINARY_DIR}/lib)

add_executable(protobuf_rpc_client client.cc)
target_link_libraries(protobuf_rpc_client monitor_proto network rpc_framework)
target_include_directories(protobuf_rpc_client PUBLIC
  ${PROJECT_SOURCE_DIR}/proto_rpc
)

add_executable(protobuf_rpc_server server.cc)
target_link_libraries(protobuf_rpc_server monitor_proto network rpc_framework )

target_include_directories(protobuf_rpc_server PUBLIC
  ${PROJECT_SOURCE_DIR}/proto_rpc
  ${PROJECT_SOURCE_DIR}/network/include
)

add_executable(test thread_local.cc)
target_link_libraries(test  pthread )

install(TARGETS protobuf_rpc_server protobuf_rpc_client test
  DESTINATION ${PROJECT_BINARY_DIR}/bin)

