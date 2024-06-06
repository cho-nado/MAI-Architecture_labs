#pragma once

#include <string>

class  Config{
private:
    Config();
    std::string _host;
    std::string _port;
    std::string _login;
    std::string _password;
    std::string _database;

public:
    static Config& get();

    const std::string& get_port() const;
    const std::string& get_host() const;
    const std::string& get_login() const;
    const std::string& get_password() const;
    const std::string& get_database() const;
};
