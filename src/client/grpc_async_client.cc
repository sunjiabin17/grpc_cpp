/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "grpc_service.grpc.pb.h"
#include "grpc_service.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using grpc_service::HelloRequest;
using grpc_service::HelloResponse;
using grpc_service::TestService;

using grpc_service::HealthCheckRequest;
using grpc_service::HealthCheckResponse;

using grpc_service::MetaRequest;
using grpc_service::MetaResponse;

class TestClient {
public:
  explicit TestClient(std::shared_ptr<Channel> channel)
      : stub_(TestService::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the HelloResponse
  // back from the server.
  std::string SayHello(const std::string &name) {
    // Data we are sending to the server.
    HelloRequest HelloRequest;
    HelloRequest.set_name(name);

    // Container for the data we expect from the server.
    HelloResponse response;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue cq;

    // Storage for the status of the RPC upon completion.
    Status status;

    std::unique_ptr<ClientAsyncResponseReader<HelloResponse>> rpc(
        stub_->AsyncSayHello(&context, HelloRequest, &cq));

    // HelloRequest that, upon completion of the RPC, "response" be updated with
    // the server's HelloResponse; "status" with the indication of whether the
    // operation was successful. Tag the HelloRequest with the integer 1.
    rpc->Finish(&response, &status, (void *)1);
    void *got_tag;
    bool ok = false;
    // Block until the next result is available in the completion queue "cq".
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or the cq_ is shutting down.
    GPR_ASSERT(cq.Next(&got_tag, &ok));

    // Verify that the result from "cq" corresponds, by its tag, our previous
    // HelloRequest.
    GPR_ASSERT(got_tag == (void *)1);
    // ... and that the HelloRequest was completed successfully. Note that "ok"
    // corresponds solely to the HelloRequest for updates introduced by
    // Finish().
    GPR_ASSERT(ok);

    // Act upon the status of the actual RPC.
    if (status.ok()) {
      return response.message();
    } else {
      return "RPC failed";
    }
  }

  void TestQuery() {
    HelloRequest hello_request;
    hello_request.set_name("hello1");

    HealthCheckRequest health_check_request;
    health_check_request.set_service("health_check2");

    MetaRequest meta_request;
    meta_request.set_key("meta_key3");
    meta_request.set_value("meta_value3");

    ClientContext context1;
    ClientContext context2;
    ClientContext context3;

    CompletionQueue cq;
    Status status;

    HelloResponse hello_response;
    HealthCheckResponse health_check_response;
    MetaResponse meta_response;

    std::unique_ptr<ClientAsyncResponseReader<HelloResponse>> hello_rpc(
        stub_->AsyncSayHello(&context1, hello_request, &cq));

    std::unique_ptr<ClientAsyncResponseReader<HealthCheckResponse>>
        health_check_rpc(
            stub_->AsyncHealthCheck(&context2, health_check_request, &cq));

    std::unique_ptr<ClientAsyncResponseReader<MetaResponse>> meta_rpc(
        stub_->AsyncGetMetaData(&context3, meta_request, &cq));

    hello_rpc->Finish(&hello_response, &status, (void *)1);
    health_check_rpc->Finish(&health_check_response, &status, (void *)2);
    meta_rpc->Finish(&meta_response, &status, (void *)3);

    void *got_tag;
    bool ok = false;
    for (int i = 0; i < 3; ++i) {
      GPR_ASSERT(cq.Next(&got_tag, &ok));
      if (got_tag == (void *)1) {
        GPR_ASSERT(ok);
        std::cout << "hello_response: " << hello_response.message()
                  << std::endl;
      } else if (got_tag == (void *)2) {
        GPR_ASSERT(ok);
        std::cout << "health_check_response: " << health_check_response.status()
                  << std::endl;
      } else if (got_tag == (void *)3) {
        GPR_ASSERT(ok);
        for (int i = 0; i < meta_response.data_size(); i++) {
          std::cout << "meta_response: " << meta_response.data(i) << std::endl;
        }
      } else {
        GPR_ASSERT(ok);
      }
    }
  }

private:
  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<TestService::Stub> stub_;
};

int main(int argc, char **argv) {
  std::string target_str = "localhost:50051";
  TestClient TestService(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string name("world");
  std::string response = TestService.SayHello(name); // The actual RPC call!
  TestService.TestQuery();
  std::cout << "TestService received: " << response << std::endl;

  return 0;
}
