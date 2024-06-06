#pragma once

#include <string>

class  Config{
private:
    Config();
    std::string _db_host;
    std::string _db_port;
    std::string _db_login;
    std::string _db_password;
    std::string _db_database;
    std::string _cache_host;
    std::string _cache_port;

public:
    static Config& get();

    const std::string& get_db_port() const;
    const std::string& get_db_host() const;
    const std::string& get_db_login() const;
    const std::string& get_db_password() const;
    const std::string& get_db_database() const;
    const std::string& get_cache_host() const;
    const std::string& get_cache_port() const;
};
