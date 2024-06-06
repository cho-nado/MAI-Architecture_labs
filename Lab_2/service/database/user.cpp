#include <sstream>
#include <exception>

#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Dynamic/Var.h"

#include "config/config.h"

#include "user.h"
#include "database.h"

using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;

namespace database {
    void User::init() {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement create_stmt(session);
            create_stmt << "CREATE TABLE IF NOT EXISTS users (id SERIAL,"
                        << "login VARCHAR(256) NOT NULL,"
                        << "password VARCHAR(256) NOT NULL,"
                        << "email VARCHAR(256) NULL,"
                        << "name VARCHAR(256) NOT NULL,"
                        << "surname VARCHAR(256) NOT NULL,"
                        << "CONSTRAINT users_login_key UNIQUE (login),"
                        << "CONSTRAINT users_pkey PRIMARY KEY (id));",
                now;
        }

        catch (Poco::Data::PostgreSQL::PostgreSQLException& e) {
            std::cout << "connection:" << e.displayText() << std::endl;
            throw;
        }
        catch (Poco::Data::ConnectionFailedException& e) {
            std::cout << "connection:" << e.displayText() << std::endl;
            throw;
        }
    }

    Poco::JSON::Object::Ptr User::toJSON() const {
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

        root->set("id", _id);
        root->set("login", _login);
        root->set("password", _password);
        root->set("name", _name);
        root->set("surname", _surname);
        root->set("email", _email);

        return root;
    }

    User User::fromJSON(const std::string& str) {
        User user;
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(str);
        Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

        user.id() = object->getValue<long>("id");
        user.login() = object->getValue<std::string>("login");
        user.password() = object->getValue<std::string>("password");
        user.email() = object->getValue<std::string>("email");
        user.name() = object->getValue<std::string>("name");
        user.surname() = object->getValue<std::string>("surname");

        return user;
    }


    std::vector<User> User::search(std::string name, std::string surname) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);
            std::vector<User> result;
            User a;
            name += "%";
            surname += "%";
            select << "SELECT id, login, password, email, name, surname FROM users where name LIKE $1 and surname LIKE $2",
                into(a._id),
                into(a._login),
                into(a._password),
                into(a._email),
                into(a._name),
                into(a._surname),
                use(name),
                use(surname),
                range(0, 1);

            while (!select.done()) {
                if (select.execute()) {
                    result.push_back(a);
                }
            }

            return result;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {
            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    void User::create() {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement insert(session);

            insert << "INSERT INTO users (login, password, email, name, surname) VALUES($1, $2, $3, $4, $5)",
                use(_login),
                use(_password),
                use(_email),
                use(_name),
                use(_surname);

            insert.execute();

            Poco::Data::Statement select(session);
            select << "SELECT LASTVAL()",
                into(_id),
                range(0, 1);

            if (!select.done()) {
                select.execute();
            }
            
            std::cout << "inserted: " << _id << std::endl;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    bool User::check_login_uniqueness(std::string new_login) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);
            User tmp;
            tmp._id = -1;

            select << "SELECT id FROM users where login=$1",
                into(tmp._id),
                use(new_login),
                range(0, 1);
            
            select.execute();

            if (select.done()) {
                return tmp._id == -1;
            }
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }

        return false;
    }

    void User::update_login(std::string new_login) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update << "UPDATE users SET login=$1 WHERE login=$2",
                use(new_login),
                use(_login);

            update.execute();

            _login = new_login;

            Poco::Data::Statement select(session);
            select << "SELECT id FROM users where login=$1",
                into(_id),
                use(_login),
                range(0, 1);

            if (!select.done()) {
                select.execute();
            }
            
            std::cout << "updated: " << _id << std::endl;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    void User::update_password(std::string new_password) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update << "UPDATE users SET password=$1 WHERE login=$2",
                use(new_password),
                use(_login);

            update.execute();

            Poco::Data::Statement select(session);
            select << "SELECT id FROM users where login=$1",
                into(_id),
                use(_login),
                range(0, 1);

            if (!select.done()) {
                select.execute();
            }
            
            std::cout << "updated: " << _id << std::endl;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    void User::update_name(std::string new_name) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update << "UPDATE users SET name=$1 WHERE login=$2",
                use(new_name),
                use(_login);

            update.execute();

            Poco::Data::Statement select(session);
            select << "SELECT id FROM users where login=$1",
                into(_id),
                use(_login),
                range(0, 1);

            if (!select.done()) {
                select.execute();
            }
            
            std::cout << "updated: " << _id << std::endl;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    void User::update_surname(std::string new_surname) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update << "UPDATE users SET surname=$1 WHERE login=$2",
                use(new_surname),
                use(_login);

            update.execute();

            Poco::Data::Statement select(session);
            select << "SELECT id FROM users where login=$1",
                into(_id),
                use(_login),
                range(0, 1);

            if (!select.done()) {
                select.execute();
            }
            
            std::cout << "updated: " << _id << std::endl;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    void User::update_email(std::string new_email) {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update << "UPDATE users SET email=$1 WHERE login=$2",
                use(new_email),
                use(_login);

            update.execute();

            Poco::Data::Statement select(session);
            select << "SELECT id FROM users where login=$1",
                into(_id),
                use(_login),
                range(0, 1);

            if (!select.done()) {
                select.execute();
            }
            
            std::cout << "updated: " << _id << std::endl;
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    void User::remove() {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement remove(session);

            remove << "DELETE FROM users WHERE login=$1 and password=$2",
                use(_login),
                use(_password);

            remove.execute();
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }
    }

    bool User::check_credentials() {
        try {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);

            User tmp;
            tmp._id = -1;

            select << "SELECT id FROM users where login=$1 and password=$2",
                into(tmp._id),
                use(_login),
                use(_password),
                range(0, 1);
            
            select.execute();

            if (select.done()) {
                return tmp._id != -1;
            }
        }
        catch (Poco::Data::PostgreSQL::ConnectionException& e) {
            std::cout << "connection error: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::PostgreSQL::StatementException& e) {

            std::cout << "statement error: " << e.what() << std::endl;
            throw;
        }

        return false;
    }

    const std::string& User::get_login() const {
        return _login;
    }

    const std::string& User::get_password() const {
        return _password;
    }

    std::string& User::login() {
        return _login;
    }

    std::string& User::password() {
        return _password;
    }

    long User::get_id() const {
        return _id;
    }

    const std::string& User::get_name() const {
        return _name;
    }

    const std::string& User::get_surname() const {
        return _surname;
    }

    const std::string& User::get_email() const {
        return _email;
    }

    long& User::id() {
        return _id;
    }

    std::string& User::name() {
        return _name;
    }

    std::string& User::surname() {
        return _surname;
    }

    std::string& User::email() {
        return _email;
    }
}
