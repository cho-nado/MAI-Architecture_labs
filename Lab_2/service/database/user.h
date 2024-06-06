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

        long&        id();
        std::string& name();
        std::string& surname();
        std::string& email();
        std::string& login();
        std::string& password();

        static void init();
        static std::vector<User> search(std::string name, std::string surname);
        static bool check_login_uniqueness(std::string new_login);
        bool   check_credentials();
        void   create();
        void   update_login(std::string new_login);
        void   update_password(std::string new_password);
        void   update_name(std::string new_name);
        void   update_surname(std::string new_surname);
        void   update_email(std::string new_email);
        void   remove();

        Poco::JSON::Object::Ptr toJSON() const;
    };
}
