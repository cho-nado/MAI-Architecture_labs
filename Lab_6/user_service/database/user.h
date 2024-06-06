#pragma once

#include <string>
#include <vector>
#include <optional>

#include "Poco/JSON/Object.h"

namespace database {

class User {
private:
    long _id;
    std::string _login;
    std::string _password;
    std::string _email;
    std::string _name;
    std::string _surname;

public:

    static User fromJSON(const std::string & str);

    long               get_id() const;
    const std::string &get_name() const;
    const std::string &get_surname() const;
    const std::string &get_email() const;
    const std::string &get_login() const;
    const std::string &get_password() const;

    void set_id(const long& value);
    void set_name(const std::string& value);
    void set_surname(const std::string& value);
    void set_email(const std::string& value);
    void set_login(const std::string& value);
    void set_password(const std::string& value);

    static void init();
    static std::vector<User> search_by_mask(std::string name, std::string surname);
    static std::optional<User> search_by_id(long id);
    static bool check_login_uniqueness(std::string new_login);
    static std::optional<long> auth(std::string login, std::string password);
    void   create();
    void   update_login();
    void   update_password();
    void   update_name();
    void   update_surname();
    void   update_email();
    void   remove();

    Poco::JSON::Object::Ptr toJSON() const;
};

}
