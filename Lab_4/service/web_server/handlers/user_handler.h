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

#include "database/user.h"
#include "web_server/checks.h"
#include "utils/utils.h"

class UserHandler : public Poco::Net::HTTPRequestHandler {
private:
    

public:
    UserHandler(const std::string& format, std::shared_ptr<Poco::Crypto::DigestEngine> digestEngine) :
    _format(format), _digestEngine(digestEngine) {}

    Poco::JSON::Object::Ptr remove_password(Poco::JSON::Object::Ptr src) {
        if (src->has("password"))
            src->set("password", "********");
        return src;
    }

    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) {
        Poco::Net::HTMLForm form(request, request.stream());

        try {
            auto uri = Poco::URI(request.getURI());

            if (uri.getPath() == "/user/search") {
                std::string name = form.get("name");
                std::string surname = form.get("surname");

                auto results = database::User::search(name, surname);

                Poco::JSON::Array arr;

                for (auto s : results) {
                    arr.add(remove_password(s.toJSON()));
                }

                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");

                std::ostream& ostr = response.send();
                Poco::JSON::Stringifier::stringify(arr, ostr);

                return;
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_POST) {
                if (form.has("name") && form.has("surname") && form.has("email") && form.has("login") && form.has("password")) {
                    database::User user;

                    user.set_login(form.get("login"));
                    user.set_email(form.get("email"));
                    user.set_name(form.get("name"));
                    user.set_surname(form.get("surname"));

                    _digestEngine->update(form.get("password"));
                    user.set_password(_digestEngine->digestToHex(_digestEngine->digest()));

                    bool check_result = true;
                    std::string message;
                    std::string reason;

                    if (!check_name(user.get_name(), reason)) {
                        check_result = false;
                        message += reason;
                        message += "<br>";
                    }

                    if (!check_name(user.get_surname(), reason)) {
                        check_result = false;
                        message += reason;
                        message += "<br>";
                    }

                    if (!check_email(user.get_email(), reason)) {
                        check_result = false;
                        message += reason;
                        message += "<br>";
                    }

                    if (!database::User::check_login_uniqueness(user.get_login())) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);

                        std::ostream& ostr = response.send();
                        ostr << "Login must be unique<br>";

                        response.send();

                        return;
                    }

                    if (check_result) {
                            user.create();

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream& ostr = response.send();
                        ostr << user.get_id();

                        return;
                    } else {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_ACCEPTABLE);

                        std::ostream& ostr = response.send();
                        ostr << message;

                        response.send();

                        return;
                    }
                }
            } else if (uri.getPath() == "/user/auth") {
                std::string scheme;
                std::string info;

                request.getCredentials(scheme, info);

                std::cout << "scheme: " << scheme << " identity: " << info << std::endl;

                if (scheme == "Basic") {
                    auto [login, password] = get_identity(info);

                    _digestEngine->update(password);
                    password = _digestEngine->digestToHex(_digestEngine->digest());

                    if (auto id = database::User::auth(login, password)) {
                        std::string token = generate_token(*id, login);
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");
                        std::ostream &ostr = response.send();
                        ostr << "{ \"id\" : \"" << *id << "\", \"Token\" : \""<< token <<"\"}" << std::endl;
                        return;
                    }
                }

                response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_UNAUTHORIZED);
                response.setChunkedTransferEncoding(true);
                response.setContentType("application/json");
                Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                root->set("type", "/errors/unauthorized");
                root->set("title", "Internal exception");
                root->set("status", "401");
                root->set("detail", "not authorized");
                root->set("instance", "/auth");
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(root, ostr);
                return;
            }
        } catch (...) {}

        response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");

        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

        root->set("type", "/errors/not_found");
        root->set("title", "Internal exception");
        root->set("status", Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
        root->set("detail", "request not found");
        root->set("instance", "/user");

        std::ostream& ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }

private:
    std::string _format;
    std::shared_ptr<Poco::Crypto::DigestEngine> _digestEngine;
};
