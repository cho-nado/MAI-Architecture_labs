#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <Poco/URI.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/Object.h>

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/ServerApplication.h>


#include "CircuitBreaker.h"
#include "database/cache.h"
#include "utils/utils.h"

class GatewayHandler : public Poco::Net::HTTPRequestHandler {
protected:
    std::string get_request(const std::string &method, const std::string &base_path, const std::string &query, 
                            const std::string &basic_auth, const std::string &token, const std::string &body) {
        try {
            // URL для отправки запроса
            Poco::URI uri(base_path + query);
            std::string path(uri.getPathAndQuery());
            if (path.empty())
                path = "/";

            std::cout << "# api gateway: request " << base_path + query << std::endl;
            // Создание сессии HTTP
            Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

            // Создание запроса
            Poco::Net::HTTPRequest request(method, path, Poco::Net::HTTPMessage::HTTP_1_1);
            request.setContentType("application/json");
            request.setContentLength(body.length());
            session.sendRequest(request) << body;

            if (!basic_auth.empty())
            {
                request.set("Authorization", "Basic " + basic_auth);
            }
            else if (!token.empty())
            {
                request.set("Authorization", "Bearer " + token);
            }

            // Отправка запроса
            session.sendRequest(request);

            // Получение ответа
            Poco::Net::HTTPResponse response;
            std::istream &rs = session.receiveResponse(response);

            // Вывод ответа на экран
            std::stringstream ss;
            Poco::StreamCopier::copyStream(rs, ss);

            return ss.str();
        }
        catch (Poco::Exception &ex)
        {
            std::cerr << ex.displayText() << std::endl;
            return std::string();
        }
        return std::string();
    }

public:
    void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {
        static CircuitBreaker circuit_breaker;

        try {
            std::cout << std::endl << "# api gateway start handle request" << std::endl;

            std::string base_url_user = "http://localhost:8080";
            std::string base_url_data = "http://localhost:8081";

            if(std::getenv("USER_ADDRESS")) base_url_user = std::getenv("USER_ADDRESS");
            if(std::getenv("DATA_ADDRESS")) base_url_data = std::getenv("DATA_ADDRESS");

            std::string scheme;
            std::string info;
            
            try {
                request.getCredentials(scheme, info);
            } catch(...) {
                response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                root->set("type", "/errors/not_authorized");
                root->set("title", "Internal exception");
                root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN));
                root->set("detail", "user not authorized");
                root->set("instance", "/gateway");
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(root, ostr);

                return;
            }

            std::cout << "# api gateway - scheme: " << scheme << " identity: " << info << std::endl;

            std::string login, password;
            if (scheme == "Basic") {
                std::string token = get_request(Poco::Net::HTTPRequest::HTTP_GET, base_url_user, "/user/auth", info, "", "");

                std::cout << "# api gateway - auth :" << token << std::endl;
                if (!token.empty()) {
                    Poco::JSON::Parser parser;
                    Poco::Dynamic::Var result = parser.parse(token);
                    Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

                    token = object->getValue<std::string>("Token");

                    std::string service_name{"delivery"};
                    if(circuit_breaker.check(service_name)) {
                        std::cout << "# api gateway - request from service" << std::endl;
                        std::string real_result = get_request(request.getMethod(), base_url_data, request.getURI(), "", token, "");
                        std::cout << "# api gateway - result: " << std::endl << real_result << std::endl;

                        if(!real_result.empty()){
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            std::ostream &ostr = response.send();
                            ostr << real_result;
                            ostr.flush();
                            circuit_breaker.success(service_name);
                            utils::cache::put_cache(request.getMethod(), base_url_data, request.getURI(), info, real_result);

                            return;
                        } else {
                            response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_UNAUTHORIZED);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                            root->set("type", "/errors/unauthorized");
                            root->set("title", "Internal exception");
                            root->set("status", "401");
                            root->set("detail", "not authorized");
                            root->set("instance", "/user/auth");
                            std::ostream &ostr = response.send();
                            Poco::JSON::Stringifier::stringify(root, ostr);
                            circuit_breaker.fail(service_name);

                            return;
                        }
                    } else {
                        std::cout << "# api gateway - request from cache" << std::endl;
                        if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET) {
                            std::string cache_result = utils::cache::get_cached(request.getMethod(), base_url_data, request.getURI(),info);
                            if(!cache_result.empty()){
                                std::cout << "# api gateway - from cache : " << cache_result << std::endl;
                                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                                response.setChunkedTransferEncoding(true);
                                response.setContentType("application/json");
                                std::ostream &ostr = response.send();
                                ostr << cache_result;
                                ostr.flush();
                                return;
                            }
                        }

                        std::cout << "# api gateway - fail to request from cache" << std::endl;

                        response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_SERVICE_UNAVAILABLE);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");
                        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                        root->set("type", "/errors/unavailable");
                        root->set("title", "Service unavailable");
                        root->set("status", "503");
                        root->set("detail", "circuit breaker open");
                        root->set("instance", request.getURI());
                        std::ostream &ostr = response.send();
                        Poco::JSON::Stringifier::stringify(root, ostr);

                        return;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        } catch (...) {}

        response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");

        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

        root->set("type", "/errors/not_found");
        root->set("title", "Internal exception");
        root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND));
        root->set("detail", "request not found");
        root->set("instance", "/gateway");

        std::ostream& ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }
};

class HTTPRequestFactory : public Poco::Net::HTTPRequestHandlerFactory {
    Poco::Net::HTTPRequestHandler *createRequestHandler([[maybe_unused]] const Poco::Net::HTTPServerRequest &request) {
        return new GatewayHandler();
    }
};

class HTTPWebServer : public Poco::Util::ServerApplication {
protected:
    int main([[maybe_unused]] const std::vector<std::string> &args) {
        Poco::Net::ServerSocket svs(Poco::Net::SocketAddress("0.0.0.0", 8888));
        Poco::Net::HTTPServer srv(new HTTPRequestFactory(), svs, new Poco::Net::HTTPServerParams);

        std::cout << "Started gatweay on port: 8888" << std::endl;

        srv.start();
        waitForTerminationRequest();
        srv.stop();

        return Application::EXIT_OK;
    }
};

int main(int argc, char *argv[]) {
    HTTPWebServer app;
    return app.run(argc, argv);
}