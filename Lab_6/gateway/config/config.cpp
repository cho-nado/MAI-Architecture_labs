#include "config.h"

Config::Config() {
    _cache_host = std::getenv("CACHE_HOST");
    _cache_port = std::getenv("CACHE_PORT");
}

Config& Config::get() {
    static Config _instance;
    return _instance;
}

const std::string& Config::get_cache_host() const {
    return _cache_host;
}

const std::string& Config::get_cache_port() const {
    return _cache_port;
}
