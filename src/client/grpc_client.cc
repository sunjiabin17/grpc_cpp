// #include "grpc_client.h"

// std::string gRPCClient::Infer(const std::string& data) {
//     // Data we are sending to the server.
//     Request request;
//     request.set_data(data);

//     // Container for the data we expect from the server.
//     Response response;

//     // Context for the client. It could be used to convey extra information to
//     // the server and/or tweak certain RPC behaviors.
//     ClientContext context;

//     // The actual RPC.
//     Status status = stub_->GetImgClsResult(&context, request, &response);

//     // Act upon its status.
//     if (status.ok()) {
//         return response.message();
//     }
//     else {
//         std::cout << status.error_code() << ": " << status.error_message()
//             << std::endl;
//         return "RPC failed";
//     }
// }

// int main(int argc, char** argv) {
//     std::string target_str = "localhost:50051";
//     gRPCClient client(
//         grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    
//     std::string data("world");
//     std::string response = client.Infer(data);
//     std::cout << "Greeter received: " << response << std::endl;

//     return 0;
// }