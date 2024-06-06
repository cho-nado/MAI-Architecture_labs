#pragma once

#include <iostream>

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>

#include "handlers/delivery_handler.h"


class HTTPRequestFactory: public Poco::Net::HTTPRequestHandlerFactory {
public:
    HTTPRequestFactory(const std::string& format): _format(format) {}

    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) {
        std::cout << "request: " << request.getMethod() << " " << request.getURI() << std::endl;
        
        return new DeliveryHandler(_format);
    }

private:
    std::string _format;
};
