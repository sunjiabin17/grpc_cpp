// #include <iostream>
// #include <string>
// #include <sstream>
// #include <iomanip>
// #include <ctime>

// #include <grpc/grpc.h>
// #include <grpcpp/server.h>
// #include <grpcpp/server_builder.h>
// #include <grpcpp/server_context.h>
// #include <grpcpp/health_check_service_interface.h>
// #include <grpcpp/ext/proto_server_reflection_plugin.h>

// #include "grpc_service.pb.h"
// #include "grpc_service.grpc.pb.h"


// using grpc::Server;
// using grpc::ServerBuilder;
// using grpc::ServerContext;
// using grpc::Status;

// using grpc_service::Request;
// using grpc_service::Response;
// using grpc_service::InferenceService;

// class InferenceServiceImpl final : public InferenceService::Service {
//     Status GetImgClsResult(ServerContext* context, const Request* request,
//         Response* response) override;
// };

