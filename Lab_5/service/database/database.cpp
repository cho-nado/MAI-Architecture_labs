#include "config/config.h"

#include "database.h"

namespace database {
    
Database::Database(){
    Config cfg = Config::get(); 

    _connection_string = "host=" +      cfg.get_db_host() +
                        " user=" +      cfg.get_db_login() + 
                        " dbname=" +    cfg.get_db_database() + 
                        " port=" +      cfg.get_db_port() +
                        " password=" +  cfg.get_db_password();

    std::cout << "Connection string:" << _connection_string << std::endl;
    
    Poco::Data::PostgreSQL::Connector::registerConnector();
    _pool = std::make_unique<Poco::Data::SessionPool>(Poco::Data::PostgreSQL::Connector::KEY, _connection_string);
}

Database& Database::get(){
    static Database _instance;
    return _instance;
}

Poco::Data::Session Database::create_session(){
    return Poco::Data::Session(_pool->get());
}

}
