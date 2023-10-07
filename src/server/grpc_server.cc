#include "grpc_server.h"

Status InferenceServiceImpl::GetImgClsResult(ServerContext* context, const Request* request,
    Response* response) {
    std::string prefix("Hello ");
    // get current time, format it as HH:MM:SS
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%H:%M:%S");
    std::string time_str = ss.str();
    response->set_message(prefix + request->data() + " at " + time_str);
    return Status::OK;
}

void RunServer(uint16_t port) {
    std::string server_address("0.0.0.0:" + std::to_string(port));
    InferenceServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    uint16_t port = 50051;
    RunServer(port);
    return 0;
}