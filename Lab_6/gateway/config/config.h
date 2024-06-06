#pragma once

#include <string>

class  Config{
private:
    Config();
    std::string _cache_host;
    std::string _cache_port;

public:
    static Config& get();

    const std::string& get_cache_host() const;
    const std::string& get_cache_port() const;
};
