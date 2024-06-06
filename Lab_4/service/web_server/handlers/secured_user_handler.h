#pragma once

#include <iostream>
#include <iostream>
#include <fstream>

#include "Poco/URI.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTMLForm.h"

#include "utils/utils.h"
#include "web_server/checks.h"


class SecuredHandler : public Poco::Net::HTTPRequestHandler
{

public:
    SecuredHandler(const std::string& format, std::shared_ptr<Poco::Crypto::DigestEngine> digestEngine) :
    _format(format), _digestEngine(digestEngine) {}

    void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {
        Poco::Net::HTMLForm form(request, request.stream());

        try {
            std::string scheme;
            std::string info;
            
            long id {-1};

            std::string login;

            request.getCredentials(scheme, info);

            std::cout << "scheme: " << scheme << " identity: " << info << std::endl;
            
            if(scheme == "Bearer") {
                if(!extract_payload(info, id, login)) {
                    response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_FORBIDDEN);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");
                    Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                    root->set("type", "/errors/not_authorized");
                    root->set("title", "Internal exception");
                    root->set("status", "403");
                    root->set("detail", "user not authorized");
                    root->set("instance", "/user");
                    std::ostream &ostr = response.send();
                    Poco::JSON::Stringifier::stringify(root, ostr);
                    return;                   
                }
            }

            std::cout << "id:" << id << " login:" << login << std::endl;

            if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT) {
                database::User user;

                user.set_id(id);

                if (form.has("login")) {
                    user.set_login(form.get("login"));
                    
                    if (!database::User::check_login_uniqueness(user.get_login())) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);

                        std::ostream& ostr = response.send();
                        ostr << "Login must be unique<br>";

                        response.send();

                        return;
                    }

                    user.update_login();

                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");

                    std::ostream& ostr = response.send();
                    ostr << user.get_id();

                    return;
                } else if (form.has("password")) {
                    user.set_password(form.get("password"));

                    user.update_password();

                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");

                    std::ostream& ostr = response.send();
                    ostr << user.get_id();

                    return;
                } else if (form.has("name")) {
                    user.set_name(form.get("name"));

                    std::string reason;
                    if (!check_name(user.get_name(), reason)) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                        std::ostream& ostr = response.send();
                        ostr << reason + "<br>";

                        response.send();

                        return;
                    }

                    user.update_name();

                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");

                    std::ostream& ostr = response.send();
                    ostr << user.get_id();

                    return;
                } else if (form.has("surname")) {
                    user.set_surname(form.get("surname"));

                    std::string reason;
                    if (!check_name(user.get_surname(), reason)) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                        std::ostream& ostr = response.send();
                        ostr << reason + "<br>";

                        response.send();

                        return;
                    }

                    user.update_surname();

                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");

                    std::ostream& ostr = response.send();
                    ostr << user.get_id();

                    return;
                } else if (form.has("email")) {
                    user.set_email(form.get("email"));

                    std::string reason;
                    if (!check_email(user.get_email(), reason)) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                        std::ostream& ostr = response.send();
                        ostr << reason + "<br>";

                        response.send();

                        return;
                    }

                    user.update_email();

                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");

                    std::ostream& ostr = response.send();
                    ostr << user.get_id();

                    return;
                }
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_DELETE) {
                database::User user;

                user.set_id(id);

                user.remove();

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");

                response.send();

                return;
            }
            
        }
        catch (std::exception &ex)
        {
            std::cout << "exception:" << ex.what() << std::endl;
        }

        response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
        root->set("type", "/errors/not_found");
        root->set("title", "Internal exception");
        root->set("status", Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
        root->set("detail", "request ot found");
        root->set("instance", "/user");
        std::ostream &ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }

private:
    std::string _format;
    std::shared_ptr<Poco::Crypto::DigestEngine> _digestEngine;
};
