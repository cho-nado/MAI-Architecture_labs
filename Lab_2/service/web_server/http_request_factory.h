#pragma once

#include <iostream>

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Crypto/DigestEngine.h"

#include "handlers/user_handler.h"


class HTTPRequestFactory: public Poco::Net::HTTPRequestHandlerFactory {
public:
    HTTPRequestFactory(const std::string& format): 
        _format(format), _digestEngine(new Poco::Crypto::DigestEngine("SHA256")) {}

    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) {
        std::cout << "request:" << request.getURI() << std::endl;
        auto uri = Poco::URI(request.getURI());
        
        std::vector<std::string> path_segments;
        uri.getPathSegments(path_segments);

        if (!path_segments.empty() && path_segments[0] == "user") { 
            return new UserHandler(_format, _digestEngine);
        }

        return 0;
    }

private:
    std::string _format;
    std::shared_ptr<Poco::Crypto::DigestEngine> _digestEngine;
};
