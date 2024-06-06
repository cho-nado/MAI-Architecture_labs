#include "config.h"

Config::Config() {
    _db_host = std::getenv("DB_HOST");
    _db_port = std::getenv("DB_PORT");
    _db_name = std::getenv("DB_DATABASE");
    _cache_host = std::getenv("CACHE_HOST");
    _cache_port = std::getenv("CACHE_PORT");
}

Config& Config::get() {
    static Config _instance;
    return _instance;
}

const std::string& Config::get_db_port() const {
    return _db_port;
}

const std::string& Config::get_db_host() const {
    return _db_host;
}

const std::string& Config::get_db_name() const {
    return _db_name;
}

const std::string& Config::get_cache_host() const {
    return _cache_host;
}

const std::string& Config::get_cache_port() const {
    return _cache_port;
}
