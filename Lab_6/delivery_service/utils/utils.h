#pragma once

#include <istream>
#include <ostream>
#include <string>

#include <Poco/Base64Decoder.h>
#include <Poco/JWT/Token.h>
#include <Poco/JWT/Signer.h>

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
