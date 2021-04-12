protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./AppServer.proto

protoc -I . --cpp_out=. ./AppServer.proto
