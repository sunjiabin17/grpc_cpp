// #include <iostream>
// #include <string>

// #include <grpcpp/grpcpp.h>

// // #include "grpc_service.pb.h"
// #include "grpc_service.grpc.pb.h"

// using grpc::Channel;
// using grpc::ClientContext;
// using grpc::Status;
// using grpc_service::Request;
// using grpc_service::Response;
// using grpc_service::InferenceService;

// class gRPCClient {
// public:
//     gRPCClient(std::shared_ptr<Channel> channel)
//         : stub_(InferenceService::NewStub(channel)) {}

//     // Assembles the client's payload, sends it and presents the response back
//     // from the server.
//     std::string Infer(const std::string& data);

// private:
//     std::unique_ptr<InferenceService::Stub> stub_;
// };
