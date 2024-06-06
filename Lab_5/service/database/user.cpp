#include <sstream>
#include <exception>

#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Dynamic/Var.h"

#include "config/config.h"

#include "user.h"
#include "database.h"

namespace database {

void User::init() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement init_db(session);
        init_db << "CREATE TABLE IF NOT EXISTS users (id SERIAL,"
                << "login VARCHAR(256) NOT NULL,"
                << "password VARCHAR(256) NOT NULL,"
                << "email VARCHAR(256) NULL,"
                << "name VARCHAR(256) NOT NULL,"
                << "surname VARCHAR(256) NOT NULL,"
                << "CONSTRAINT users_login_key UNIQUE (login),"
                << "CONSTRAINT users_pkey PRIMARY KEY (id));",
            Poco::Data::Keywords::now;
    } catch (Poco::Data::PostgreSQL::PostgreSQLException& e) {
        std::cout << "connection:" << e.displayText() << std::endl;

        throw;
    } catch (Poco::Data::ConnectionFailedException& e) {
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

    user.set_id(object->getValue<long>("id"));
    user.set_login(object->getValue<std::string>("login"));
    user.set_password(object->getValue<std::string>("password"));
    user.set_email(object->getValue<std::string>("email"));
    user.set_name(object->getValue<std::string>("name"));
    user.set_surname(object->getValue<std::string>("surname"));

    return user;
}

std::optional<long> User::auth(std::string login, std::string password) {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement select(session);

        long id;
        select << "SELECT id FROM users where login=$1 and password=$2",
            Poco::Data::Keywords::into(id),
            Poco::Data::Keywords::use(login),
            Poco::Data::Keywords::use(password),
            Poco::Data::Keywords::range(0, 1);

        select.execute();
        Poco::Data::RecordSet rs(select);

        if (rs.moveFirst()) {
            return id;
        }
    }

    catch (Poco::Data::PostgreSQL::ConnectionException &e) {
        std::cout << "connection:" << e.what() << std::endl;
    }

    catch (Poco::Data::PostgreSQL::StatementException &e) {
        std::cout << "statement:" << e.what() << std::endl;
    }
    
    return {};
}


std::vector<User> User::search_by_mask(std::string name, std::string surname) {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement select(session);
        std::vector<User> result;
        User tmp;
        name += "%";
        surname += "%";
        select << "SELECT id, login, password, email, name, surname FROM users where name LIKE $1 and surname LIKE $2",
            Poco::Data::Keywords::into(tmp._id),
            Poco::Data::Keywords::into(tmp._login),
            Poco::Data::Keywords::into(tmp._password),
            Poco::Data::Keywords::into(tmp._email),
            Poco::Data::Keywords::into(tmp._name),
            Poco::Data::Keywords::into(tmp._surname),
            Poco::Data::Keywords::use(name),
            Poco::Data::Keywords::use(surname),
            Poco::Data::Keywords::range(0, 1);

        while (!select.done()) {
            if (select.execute()) {
                result.push_back(tmp);
            }
        }

        return result;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {
        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

std::optional<User> User::search_by_id(long id) {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement select(session);
        User tmp;

        select << "SELECT id, login, password, email, name, surname FROM users where id=$1",
            Poco::Data::Keywords::into(tmp._id),
            Poco::Data::Keywords::into(tmp._login),
            Poco::Data::Keywords::into(tmp._password),
            Poco::Data::Keywords::into(tmp._email),
            Poco::Data::Keywords::into(tmp._name),
            Poco::Data::Keywords::into(tmp._surname),
            Poco::Data::Keywords::use(id),
            Poco::Data::Keywords::range(0, 1);

        select.execute();
        
        Poco::Data::RecordSet rs(select);
        if (rs.moveFirst()) {
            return tmp;
        } else {
            return {};
        }
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
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
            Poco::Data::Keywords::use(_login),
            Poco::Data::Keywords::use(_password),
            Poco::Data::Keywords::use(_email),
            Poco::Data::Keywords::use(_name),
            Poco::Data::Keywords::use(_surname);

        insert.execute();

        Poco::Data::Statement select(session);
        select << "SELECT LASTVAL()",
            Poco::Data::Keywords::into(_id),
            Poco::Data::Keywords::range(0, 1);

        if (!select.done()) {
            select.execute();
        }
        
        std::cout << "inserted: " << _id << std::endl;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

bool User::check_login_uniqueness(std::string new_login) {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement select(session);
        User tmp;
        tmp._id = -1;

        select << "SELECT id FROM users where login=$1",
            Poco::Data::Keywords::into(tmp._id),
            Poco::Data::Keywords::use(new_login),
            Poco::Data::Keywords::range(0, 1);
        
        select.execute();

        if (select.done()) {
            return tmp._id == -1;
        }
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }

    return false;
}

void User::update_login() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement update(session);

        update << "UPDATE users SET login=$1 WHERE id=$2",
            Poco::Data::Keywords::use(_login),
            Poco::Data::Keywords::use(_id);

        update.execute();
        
        std::cout << "updated: " << _id << std::endl;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

void User::update_password() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement update(session);

        update << "UPDATE users SET password=$1 WHERE id=$2",
            Poco::Data::Keywords::use(_password),
            Poco::Data::Keywords::use(_id);

        update.execute();
        
        std::cout << "updated: " << _id << std::endl;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

void User::update_name() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement update(session);

        update << "UPDATE users SET name=$1 WHERE id=$2",
            Poco::Data::Keywords::use(_name),
            Poco::Data::Keywords::use(_id);

        update.execute();
        
        std::cout << "updated: " << _id << std::endl;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

void User::update_surname() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement update(session);

        update << "UPDATE users SET surname=$1 WHERE id=$2",
            Poco::Data::Keywords::use(_surname),
            Poco::Data::Keywords::use(_id);

        update.execute();
        
        std::cout << "updated: " << _id << std::endl;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

void User::update_email() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement update(session);

        update << "UPDATE users SET email=$1 WHERE id=$2",
            Poco::Data::Keywords::use(_email),
            Poco::Data::Keywords::use(_login);

        update.execute();
        
        std::cout << "updated: " << _id << std::endl;
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
}

void User::remove() {
    try {
        Poco::Data::Session session = database::Database::get().create_session();
        Poco::Data::Statement remove(session);

        remove << "DELETE FROM users WHERE id=$1",
            Poco::Data::Keywords::use(_id);

        remove.execute();
    } catch (Poco::Data::PostgreSQL::ConnectionException& e) {
        std::cout << "connection error: " << e.what() << std::endl;
        throw;
    } catch (Poco::Data::PostgreSQL::StatementException& e) {

        std::cout << "statement error: " << e.what() << std::endl;
        throw;
    }
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

const std::string& User::get_login() const {
    return _login;
}

const std::string& User::get_password() const {
    return _password;
}

void User::set_id(const long& value) {
    _id = value;
}

void User::set_name(const std::string& value) {
    _name = value;
}

void User::set_surname(const std::string& value) {
    _surname = value;
}

void User::set_email(const std::string& value) {
    _email = value;
}

void User::set_login(const std::string& value) {
    _login = value;
}

void User::set_password(const std::string& value) {
    _password = value;
}

}
