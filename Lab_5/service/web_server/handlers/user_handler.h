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
#include "database/cache.h"
#include "web_server/checks.h"
#include "utils/utils.h"

class UserHandler : public Poco::Net::HTTPRequestHandler {
private:
public:
    UserHandler(const std::string& format, std::shared_ptr<Poco::Crypto::DigestEngine> digestEngine) :
    _format(format), _digestEngine(digestEngine) {}

    Poco::JSON::Object::Ptr remove_password(Poco::JSON::Object::Ptr src) {
        if (src->has("password")) {
            src->set("password", "********");
        }

        return src;
    }

    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) {
        Poco::Net::HTMLForm form(request, request.stream());

        try {
            auto uri = Poco::URI(request.getURI());

            if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET) {
                if (uri.getPath() == "/user/auth") {
                    std::string scheme;
                    std::string info;

                    request.getCredentials(scheme, info);

                    std::cout << "scheme: " << scheme << " identity: " << info << std::endl;

                    if (scheme == "Basic") {
                        std::string cache_result = utils::cache::get_cached("AUTH", request.getHost(), request.getURI(), info);

                        if(!cache_result.empty()){
                            std::cout << "# from cache: " << cache_result << std::endl;

                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            std::ostream &ostr = response.send();
                            ostr << cache_result;
                            ostr.flush();
                            return;
                        }

                        auto [login, password] = utils::auth::get_identity(info);

                        _digestEngine->update(password);
                        password = _digestEngine->digestToHex(_digestEngine->digest());

                        if (auto id = database::User::auth(login, password)) {
                            std::string token = utils::auth::generate_token(*id, login);
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            std::ostream& ostr = response.send();
                            std::string body = "{ \"id\" : \"" + std::to_string(*id) + "\", \"Token\" : \"" + token + "\"}";
                            ostr << body << std::endl;
                            ostr.flush();

                            utils::cache::put_cache(request.getMethod(), request.getHost(), request.getURI(), info, body);
                            
                            return;
                        }
                    }

                    response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_UNAUTHORIZED);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");
                    Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                    root->set("type", "/errors/unauthorized");
                    root->set("title", "Internal exception");
                    root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_UNAUTHORIZED));
                    root->set("detail", "not authorized");
                    root->set("instance", "/auth");
                    std::ostream &ostr = response.send();
                    Poco::JSON::Stringifier::stringify(root, ostr);
                    return;
                } else if (uri.getPath() == "/user/search" && form.has("name") && form.has("surname")) {
                    std::string cache_result = utils::cache::get_cached(request.getMethod(), request.getHost(), request.getURI(), "");

                    if(!cache_result.empty()){
                        std::cout << "# from cache: " << cache_result << std::endl;

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");
                        std::ostream &ostr = response.send();
                        ostr << cache_result;

                        return;
                    }

                    std::string name = form.get("name");
                    std::string surname = form.get("surname");

                    auto results = database::User::search_by_mask(name, surname);

                    Poco::JSON::Array arr;

                    for (const auto& result : results) {
                        arr.add(remove_password(result.toJSON()));
                    }

                    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                    response.setChunkedTransferEncoding(true);
                    response.setContentType("application/json");

                    std::ostream& ostr = response.send();

                    std::stringstream ss;
                    Poco::JSON::Stringifier::stringify(arr, ss);

                    ostr << ss.str();

                    utils::cache::put_cache(request.getMethod(), request.getHost(), request.getURI(), "", ss.str());

                    return;
                } else if (uri.getPath() == "/user/search" && form.has("id")) {
                    std::string cache_result = utils::cache::get_cached(request.getMethod(), request.getHost(), request.getURI(), "");

                    if(!cache_result.empty()){
                        // std::cout << "# from cache: " << cache_result << std::endl;

                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");
                        std::ostream &ostr = response.send();
                        ostr << cache_result;
                        
                        return;
                    }

                    long id = std::stol(form.get("id"));

                    std::optional<database::User> result = database::User::search_by_id(id);
                    if (result) {
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        std::ostream &ostr = response.send();

                        std::stringstream ss;
                        Poco::JSON::Stringifier::stringify(remove_password(result->toJSON()), ss);

                        ostr << ss.str();

                        utils::cache::put_cache(request.getMethod(), request.getHost(), request.getURI(), "", ss.str());

                        return;
                    } else {
                        response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");

                        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

                        root->set("type", "/errors/not_found");
                        root->set("title", "Internal exception");
                        root->set("status", std::to_string(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND));
                        root->set("detail", "user ot found");
                        root->set("instance", "/user");

                        std::ostream &ostr = response.send();
                        Poco::JSON::Stringifier::stringify(root, ostr);

                        return;
                    }
                }
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

                        std::stringstream ss;
                        Poco::JSON::Stringifier::stringify(remove_password(user.toJSON()), ss);

                        utils::cache::put_cache("GET", request.getHost(), "/user/search?id=" + std::to_string(user.get_id()), "", ss.str());

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
            } else if (request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT || 
                       request.getMethod() == Poco::Net::HTTPRequest::HTTP_DELETE) {
                std::string scheme;
                std::string info;
                
                long id {-1};

                std::string login;

                request.getCredentials(scheme, info);

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
        root->set("instance", "/user");

        std::ostream& ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }

private:
    std::string _format;
    std::shared_ptr<Poco::Crypto::DigestEngine> _digestEngine;
};
