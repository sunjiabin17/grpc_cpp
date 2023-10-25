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

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

using grpc_service::HelloRequest;
using grpc_service::HelloResponse;
using grpc_service::TestService;

using grpc_service::HealthCheckRequest;
using grpc_service::HealthCheckResponse;

using grpc_service::MetaRequest;
using grpc_service::MetaResponse;

class ICallData {
public:
  virtual ~ICallData() = default;
  virtual void Proceed() = 0;
};

template <typename ResponderType, typename RequestType, typename ResponseType>
class CallData : public ICallData {
public:
  using ResigterFunc = std::function<void(ServerContext *, RequestType *,
                                          ResponderType *, void *)>;
  using ProcessFunc =
      std::function<void(RequestType *, ResponseType *, Status *)>;

  CallData(TestService::AsyncService *service, ServerCompletionQueue *cq,
           const ResigterFunc register_func, const ProcessFunc process_func)
      : service_(service), cq_(cq), register_func_(register_func),
        process_func_(process_func), responder_(&ctx_), step_(CREATE) {
    register_func_(&ctx_, &request_, &responder_, this);
    step_ = PROCESS;
  }

  void Proceed() override {
    if (step_ == CREATE) {
      // do nothing
    } else if (step_ == PROCESS) {
      new CallData<ResponderType, RequestType, ResponseType>(
          service_, cq_, register_func_, process_func_);

      process_func_(&request_, &response_, &status_);
      // std::string prefix("Hello ");
      // response_.set_message(prefix + request_.name());
      step_ = FINISH;
      responder_.Finish(response_, status_, this);
    } else {
      GPR_ASSERT(step_ == FINISH);
      delete this;
    }
  }

private:
  ResponderType responder_;
  RequestType request_;
  ResponseType response_;

  ResigterFunc register_func_;
  ProcessFunc process_func_;
  TestService::AsyncService *service_;
  ServerCompletionQueue *cq_;
  ServerContext ctx_;
  enum CallStep { CREATE, PROCESS, FINISH };
  CallStep step_;

  Status status_;
};

class ServerImpl final {
public:
  ~ServerImpl() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run(uint16_t port) {
    std::string server_address = "0.0.0.0:" + std::to_string(port);

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs();
  }

private:
  void RegisterSayHello() {
    auto OnRegisterSayHello =
        [this](ServerContext *ctx, HelloRequest *request,
               ServerAsyncResponseWriter<HelloResponse> *responder, void *tag) {
          this->service_.RequestSayHello(ctx, request, responder,
                                         this->cq_.get(), this->cq_.get(), tag);
        };

    auto OnProcessSayHello = [](HelloRequest *request, HelloResponse *response,
                                Status *status) {
      std::string prefix("Hello ");
      response->set_message(prefix + request->name());
      *status = Status::OK;
    };

    new CallData<ServerAsyncResponseWriter<HelloResponse>, HelloRequest,
                 HelloResponse>(&service_, cq_.get(), OnRegisterSayHello,
                                OnProcessSayHello);
  }

  void RegisterHealthCheck() {
    auto OnRegisterHealthCheck =
        [this](ServerContext *ctx, HealthCheckRequest *request,
               ServerAsyncResponseWriter<HealthCheckResponse> *responder,
               void *tag) {
          this->service_.RequestHealthCheck(
              ctx, request, responder, this->cq_.get(), this->cq_.get(), tag);
        };

    auto OnProcessHealthCheck = [](HealthCheckRequest *request,
                                   HealthCheckResponse *response,
                                   Status *status) {
      std::string prefix(request->service());
      response->set_status(prefix + " is ok");
      *status = Status::OK;
    };

    new CallData<ServerAsyncResponseWriter<HealthCheckResponse>,
                 HealthCheckRequest, HealthCheckResponse>(
        &service_, cq_.get(), OnRegisterHealthCheck, OnProcessHealthCheck);
  }

  void RegisterMeta() {
    auto OnRegisterMeta =
        [this](ServerContext *ctx, MetaRequest *request,
               ServerAsyncResponseWriter<MetaResponse> *responder, void *tag) {
          this->service_.RequestGetMetaData(
              ctx, request, responder, this->cq_.get(), this->cq_.get(), tag);
        };

    auto OnProcessMeta = [](MetaRequest *request, MetaResponse *response,
                            Status *status) {
      std::string prefix(request->key() + ": " + request->value());
      response->add_data(prefix + '1');
      response->add_data(prefix + '2');
      response->add_data(prefix + '3');
      *status = Status::OK;
    };

    new CallData<ServerAsyncResponseWriter<MetaResponse>, MetaRequest,
                 MetaResponse>(&service_, cq_.get(), OnRegisterMeta,
                               OnProcessMeta);
  }

  // This can be run in multiple threads if needed.
  void HandleRpcs() {
    // Spawn a new CallData instance to serve new clients.
    RegisterSayHello();
    RegisterHealthCheck();
    RegisterMeta();

    void *tag; // uniquely identifies a HelloRequest.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      ICallData *call_data = static_cast<ICallData *>(tag);
      call_data->Proceed();
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  TestService::AsyncService service_;
  std::unique_ptr<Server> server_;
};

int main(int argc, char **argv) {
  ServerImpl server;
  server.Run(50051);

  return 0;
}
