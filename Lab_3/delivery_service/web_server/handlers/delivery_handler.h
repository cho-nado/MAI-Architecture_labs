#pragma once

#include <iostream>
#include <iostream>
#include <fstream>

#include <Poco/URI.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/DateTimeParser.h>

#include "database/delivery.h"
#include "utils/utils.h"

class DeliveryHandler : public Poco::Net::HTTPRequestHandler {
private:
public:
    DeliveryHandler(const std::string& format) : _format(format) {}

    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) {
        Poco::Net::HTMLForm form(request, request.stream());

        try {
            auto uri = Poco::URI(request.getURI());

            std::string scheme;
            std::string info;
            
            long id {-1};

            std::string login;

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
                root->set("instance", "/delivery");
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(root, ostr);

                return;
            }

            std::cout << "scheme: " << scheme << " identity: " << info << std::endl;
            
            if(scheme == "Bearer") {
                if(!utils::auth::extract_payload(info, id, login)) {
                    response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");
                    Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                    root->set("type", "/errors/not_authorized");
                    root->set("title", "Internal exception");
                    root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN));
                    root->set("detail", "user not authorized");
                    root->set("instance", "/delivery");
                    std::ostream &ostr = response.send();
                    Poco::JSON::Stringifier::stringify(root, ostr);

                    return;
                }

                std::cout << "user id: " << id << " user login:" << login << std::endl;
            } else {
                response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                root->set("type", "/errors/not_authorized");
                root->set("title", "Internal exception");
                root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN));
                root->set("detail", "user not authorized");
                root->set("instance", "/delivery");
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(root, ostr);

                return;
            }

            if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET && uri.getPath() == "/delivery" && form.has("delivery_id") ) {
                auto result = database::Delivery::read_by_delivery_id(std::stol(form.get("delivery_id")));

                if (result) {
                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");
                    std::ostream &ostr = response.send();
                    Poco::JSON::Stringifier::stringify(result->toJSON(), ostr);

                    return;
                } else {
                    response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");
                    Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                    root->set("type", "/errors/not_found");
                    root->set("title", "Internal exception");
                    root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND));
                    root->set("detail", "not found by delivery id");
                    root->set("instance", "/delivery");
                    std::ostream &ostr = response.send();
                    Poco::JSON::Stringifier::stringify(root, ostr);

                    return;
                }
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET && uri.getPath() == "/delivery" && form.has("sender_id") ) {
                long sender_id = std::stol(form.get("sender_id"));

                auto results = database::Delivery::read_by_sender_id(sender_id);

                Poco::JSON::Array arr;
                for (const auto& res : results) {
                    arr.add(res.toJSON());
                }

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(arr, ostr);

                return;
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET && uri.getPath() == "/delivery" && form.has("receiver_id") ) {
                long receiver_id = std::stol(form.get("receiver_id"));

                auto results = database::Delivery::read_by_receiver_id(receiver_id);

                Poco::JSON::Array arr;
                for (const auto& res : results) {
                    arr.add(res.toJSON());
                }

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(arr, ostr);

                return;
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_POST && uri.getPath() == "/delivery" &&
                       form.has("delivery_id") && form.has("sender_id") && form.has("sender_id") && form.has("date") && 
                       form.has("weight") && form.has("height") && form.has("width") && form.has("length")) {
                database::Delivery delivery;
                delivery.set_delivery_id(std::stol(form.get("delivery_id")));
                delivery.set_sender_id(std::stol(form.get("sender_id")));
                delivery.set_receiver_id(std::stol(form.get("receiver_id")));
                delivery.set_date(form.get("date"));
                delivery.set_package(
                    database::Package {
                        std::stod(form.get("weight")),
                        std::stod(form.get("height")),
                        std::stod(form.get("width")),
                        std::stod(form.get("length"))
                    }
                );

                delivery.add();

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                std::ostream &ostr = response.send();
                ostr << delivery.get_delivery_id();

                return;
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT && uri.getPath() == "/delivery" &&
                       form.has("delivery_id") && form.has("sender_id") && form.has("sender_id") && form.has("date") && 
                       form.has("weight") && form.has("height") && form.has("width") && form.has("length")) {
                database::Delivery delivery;
                delivery.set_delivery_id(std::stol(form.get("delivery_id")));
                delivery.set_sender_id(std::stol(form.get("sender_id")));
                delivery.set_receiver_id(std::stol(form.get("receiver_id")));
                delivery.set_date(form.get("date"));
                delivery.set_package(
                    database::Package {
                        std::stod(form.get("weight")),
                        std::stod(form.get("height")),
                        std::stod(form.get("width")),
                        std::stod(form.get("length"))
                    }
                );

                delivery.update();

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                std::ostream &ostr = response.send();
                ostr << delivery.get_delivery_id();
                return;
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_DELETE && uri.getPath() == "/delivery" && form.has("delivery_id")) {
                database::Delivery delivery;
                delivery.set_delivery_id(std::stol(form.get("delivery_id")));

                delivery.remove();

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                std::ostream &ostr = response.send();
                ostr << delivery.get_delivery_id();
                return;
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
        root->set("instance", "/delivery");

        std::ostream& ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }

private:
    std::string _format;
};
