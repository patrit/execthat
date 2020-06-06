#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/StreamCopier.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Util/ServerApplication.h>
#include <iostream>
#include <set>
#include <string>
#include <unistd.h>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace Poco;
using namespace std;

class ExecRequestHandler : public HTTPRequestHandler {
public:
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) {
    TemporaryFile temp_req, temp_resp;

    if (req.getContentLength() > 0) {
      FileOutputStream fos(temp_req.path(), ios::binary);
      StreamCopier::copyStream(req.stream(), fos, req.getContentLength());
      fos.close();
    }

    string cmd = req.getURI() + " " + temp_req.path() + " " + temp_resp.path();
    int rc = system(cmd.c_str());

    FileInputStream fis(temp_resp.path());
    if ((rc != 0) || !fis.good()) {
      resp.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
      resp.send() << "";
    } else {
      resp.setStatus(HTTPResponse::HTTP_OK);
      resp.setContentType("application/json");
      StreamCopier::copyStream(fis, resp.send());
    }
  }
};

class MyRequestHandlerFactory : public HTTPRequestHandlerFactory {
  set<string> commands;

public:
  MyRequestHandlerFactory() {
    vector<File> files;
    auto dir = File("/app");
    dir.list(files);
    cout << "Found executables: ";
    for (File &file : files) {
      if (file.canExecute() && file.path() != "/app/server") {
        commands.insert(file.path());
        cout << file.path() << ", ";
      }
    }
    cout << endl;
  }
  virtual HTTPRequestHandler *
  createRequestHandler(const HTTPServerRequest &req) {
    if (commands.count(req.getURI()) > 0) {
      return new ExecRequestHandler;
    }
    return nullptr;
  }
};

class MyServerApp : public ServerApplication {
  int main(const vector<string> &) {
    HTTPServer server(new MyRequestHandlerFactory, ServerSocket(8080),
                      new HTTPServerParams);

    server.start();
    cout << "Server started on 8080" << endl;
    waitForTerminationRequest(); // wait for CTRL-C or kill
    server.stop();

    return Application::EXIT_OK;
  }
};

int main(int argc, char **argv) {
  MyServerApp app;
  return app.run(argc, argv);
}
