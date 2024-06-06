#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "Poco/Base64Decoder.h"
#include "Poco/JWT/Token.h"
#include "Poco/JWT/Signer.h"

namespace utils::auth {

// returns login and password
std::tuple<std::string, std::string> get_identity(const std::string& identity) {
    std::istringstream istr(identity);
    Poco::Base64Decoder b64in(istr);
    
    std::string decoded;
    b64in >> decoded;

    size_t pos = decoded.find(':');

    return {decoded.substr(0, pos), decoded.substr(pos + 1)};
}

std::string getJWTKey() {
    if (std::getenv("JWT_KEY") != nullptr) {
        std::cout << "key loaded" << std::endl;
        return std::getenv("JWT_KEY");
    }

    return "0123456789ABCDEF0123456789ABCDEF";
}

std::string generate_token(const long& id, const std::string& login) {
    Poco::JWT::Token token;

    token.setType("JWT");
    token.setSubject("login");
    token.payload().set("login", login);
    token.payload().set("id", id);
    token.setIssuedAt(Poco::Timestamp());

    Poco::JWT::Signer signer(getJWTKey());

    return signer.sign(token, Poco::JWT::Signer::ALGO_HS256);
}

bool extract_payload(const std::string &jwt_token, long& id, std::string& login) {
    if (jwt_token.length() == 0) {
        return false;
    }

    Poco::JWT::Signer signer(getJWTKey());

    try {
        Poco::JWT::Token token = signer.verify(jwt_token);

        if (token.payload().has("login") && token.payload().has("id")) {
            login = token.payload().getValue<std::string>("login");
            id = token.payload().getValue<long>("id");

            return true;
        }

        std::cout << "Not enough fields in token" << std::endl;
    } catch (...) {
        std::cout << "Token verification failed" << std::endl;
    }
    return false;
}

}


namespace utils::cache {

std::string get_cache_key(const std::string& method, const std::string& host, const std::string& URI, const std::string& auth) {
    return method + ":" + host + ":" + URI + ":" + auth;
}

std::string get_cached(const std::string& method, const std::string& host, const std::string& URI, const std::string& auth) {
    std::string key = get_cache_key(method, host, URI, auth);

    return database::Cache::get().get(key);
}

void put_cache(const std::string& method, const std::string& host, const std::string& URI, const std::string& auth, const std::string& value) {
    std::string key = get_cache_key(method, host, URI, auth);

    database::Cache::get().put(key, value);
}

}
