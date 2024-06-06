#include <string>
#include <map>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>

#include "config/config.h"

#include "database.h"

namespace database {
    
Database::Database() : _database(Config::get().get_db_name()) {
    Config cfg = Config::get(); 

    std::cout << "# Connecting to mongodb: " << cfg.get_db_host() << ":" << cfg.get_db_port() << std::endl;
    _connection.connect(cfg.get_db_host(), std::stoi(cfg.get_db_port()));
}

Database& Database::get() {
    static Database _instance;
    return _instance;
}

long Database::count_from_db(const std::string& collection, std::map<std::string, long>& params) {
    try {
        Poco::SharedPtr<Poco::MongoDB::QueryRequest> request = _database.createCountRequest(collection);

        auto& query = request->selector().addNewDocument("query");
        for (const auto& [key, value] : params) {
            query.addNewDocument(key).add("$eq", value);
        }
        
        Poco::MongoDB::ResponseMessage response;
        _connection.sendRequest(*request, response);

        if (response.hasDocuments()) {
            return response.documents()[0]->getInteger("n");
        }
    } catch (std::exception &e) {
        std::cout << "mongodb exception: " << e.what() << std::endl;

        std::string lastError = _database.getLastError(_connection);

        if (!lastError.empty()) {
            std::cout << "mongodb Last Error: " << lastError << std::endl;
        }
    }

    return -1;
}

void Database::insert_into_db(const std::string& collection, Poco::JSON::Object::Ptr json) {
    try {
        Poco::SharedPtr<Poco::MongoDB::InsertRequest> insertRequest = _database.createInsertRequest(collection);

        std::function<void(Poco::MongoDB::Document &, Poco::JSON::Object::Ptr &)> fill_document;

        fill_document = [&](Poco::MongoDB::Document &doc, Poco::JSON::Object::Ptr &obj) -> void {
            for (const auto& value : *obj) {
                if (value.second.isInteger()) {
                    int tmp;
                    value.second.convert(tmp);
                    doc.add(value.first, tmp);
                } else if (value.second.isString()) {
                    doc.add(value.first, value.second.extract<std::string>());
                } else {
                    try {
                        Poco::JSON::Object::Ptr child = value.second.extract<Poco::JSON::Object::Ptr>();

                        Poco::MongoDB::Document &child_doc = doc.addNewDocument(value.first);

                        fill_document(child_doc, child);
                    } catch (...) {
                        doc.add(value.first, value.second.toString());
                    }
                }
            }
        };

        Poco::MongoDB::Document &doc = insertRequest->addNewDocument();

        fill_document(doc, json);

        _connection.sendRequest(*insertRequest);
    } catch (std::exception &e) {
        std::cout << "mongodb exception: " << e.what() << std::endl;

        std::string lastError = _database.getLastError(_connection);

        if (!lastError.empty()) {
            std::cout << "mongodb Last Error: " << lastError << std::endl;
        }
    }
}



}
