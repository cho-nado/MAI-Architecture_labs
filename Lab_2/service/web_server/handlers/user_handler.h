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

class UserHandler : public Poco::Net::HTTPRequestHandler {
private:
    bool check_name(const std::string& name, std::string& reason) {
        if (name.length() < 3) {
            reason = "Name must be at leas 3 signs";
            return false;
        }

        if (name.find(' ') != std::string::npos) {
            reason = "Name can't contain spaces";
            return false;
        }

        if (name.find('\t') != std::string::npos) {
            reason = "Name can't contain spaces";
            return false;
        }

        return true;
    };

    bool check_email(const std::string& email, std::string& reason) {
        if (email.find('@') == std::string::npos) {
            reason = "Email must contain @";
            return false;
        }

        if (email.find(' ') != std::string::npos) {
            reason = "EMail can't contain spaces";
            return false;
        }

        if (email.find('\t') != std::string::npos) {
            reason = "EMail can't contain spaces";
            return false;
        }

        return true;
    };

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

                    user.login() = form.get("login");
                    user.email() = form.get("email");
                    user.name() = form.get("name");
                    user.surname() = form.get("surname");

                    _digestEngine->update(form.get("password"));
                    user.password() = _digestEngine->digestToHex(_digestEngine->digest());

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

                    if (!database::User::check_login_uniqueness(user.login())) {
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
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT) {
                if (form.has("login") && form.has("password")) {
                    database::User user;

                    user.login() = form.get("login");

                    _digestEngine->update(form.get("password"));
                    user.password() = _digestEngine->digestToHex(_digestEngine->digest());

                    if (!user.check_credentials()) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                        std::ostream& ostr = response.send();
                        ostr << "Invalid login or password";

                        response.send();

                        return;
                    }

                    if (form.has("new_login")) {
                        auto new_login = form.get("new_login");
                        if (!database::User::check_login_uniqueness(new_login)) {
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);

                            std::ostream& ostr = response.send();
                            ostr << "Login must be unique<br>";

                            response.send();

                            return;
                        }

                        user.update_login(new_login);

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream& ostr = response.send();
                        ostr << user.get_id();

                        return;
                    } else if (form.has("new_password")) {
                        user.update_password(form.get("new_password"));

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream& ostr = response.send();
                        ostr << user.get_id();

                        return;
                    } else if (form.has("new_name")) {
                        auto new_name = form.get("new_name");

                        std::string reason;
                        if (!check_name(new_name, reason)) {
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                            std::ostream& ostr = response.send();
                            ostr << reason + "<br>";

                            response.send();

                            return;
                        }

                        user.update_name(new_name);

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream& ostr = response.send();
                        ostr << user.get_id();

                        return;
                    } else if (form.has("new_surname")) {
                        auto new_surname = form.get("new_surname");

                        std::string reason;
                        if (!check_name(new_surname, reason)) {
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                            std::ostream& ostr = response.send();
                            ostr << reason + "<br>";

                            response.send();

                            return;
                        }

                        user.update_surname(new_surname);

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream& ostr = response.send();
                        ostr << user.get_id();

                        return;
                    } else if (form.has("new_email")) {
                        auto new_email = form.get("new_email");
                        
                        std::string reason;
                        if (!check_email(new_email, reason)) {
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                            std::ostream& ostr = response.send();
                            ostr << reason + "<br>";

                            response.send();

                            return;
                        }

                        user.update_email(new_email);

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream& ostr = response.send();
                        ostr << user.get_id();

                        return;
                    }
                }
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_DELETE) {
                if (form.has("login") && form.has("password")) {
                    database::User user;

                    user.login() = form.get("login");

                    _digestEngine->update(form.get("password"));
                    user.password() = _digestEngine->digestToHex(_digestEngine->digest());

                    if (user.check_credentials()) {
                        user.remove();

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        response.send();

                        return;
                    } else {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);

                        std::ostream& ostr = response.send();
                        ostr << "Invalid login or password";

                        response.send();

                        return;
                    }
                }
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
