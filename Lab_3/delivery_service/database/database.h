#pragma once

#include <string>
#include <memory>

#include <Poco/MongoDB/MongoDB.h>
#include <Poco/MongoDB/Connection.h>
#include <Poco/MongoDB/Database.h>
#include <Poco/MongoDB/Cursor.h>

namespace database{

class Database{
private:
    Poco::MongoDB::Connection _connection;
    Poco::MongoDB::Database   _database;
    
    Database();
public:
    static Database& get();
    void insert_into_db(const std::string& collection, Poco::JSON::Object::Ptr json);
    long count_from_db(const std::string& collection, std::map<std::string, long>& params);
    
    template <typename T> std::vector<std::string> get_from_db(const std::string& collection, std::map<std::string, T>& params) {
        std::vector<std::string> result;

        try {
            Poco::MongoDB::QueryRequest request(Config::get().get_db_name() + "." + collection);
            Poco::MongoDB::ResponseMessage response;

            for (const auto& [key, val] : params) {
                request.selector().add(key, val);
            }

            _connection.sendRequest(request, response);

            for (const auto& doc : response.documents()) {
                result.push_back(doc->toString());
            }
        } catch (std::exception &e) {
            std::cout << "mongodb exception: " << e.what() << std::endl;

            std::string lastError = _database.getLastError(_connection);

            if (!lastError.empty()) {
                std::cout << "mongodb Last Error: " << lastError << std::endl;
            }
        }
        return result;
    }

    template <typename T> void update_db(const std::string& collection, std::map<std::string, T>& params, Poco::JSON::Object::Ptr json) {
        try {
            Poco::SharedPtr<Poco::MongoDB::UpdateRequest> request = _database.createUpdateRequest(collection);

            for (const auto& [key, value] : params) {
                request->selector().add(key, value);
            }

            std::function<void(Poco::MongoDB::Document&, Poco::JSON::Object::Ptr&)> fill_document;

            fill_document = [&](Poco::MongoDB::Document& doc, Poco::JSON::Object::Ptr& obj) -> void
            {
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

            Poco::MongoDB::Document &doc = request->update();

            fill_document(doc, json);

            _connection.sendRequest(*request);
        } catch (std::exception &e) {
            std::cout << "mongodb exception: " << e.what() << std::endl;

            std::string lastError = _database.getLastError(_connection);

            if (!lastError.empty()) {
                std::cout << "mongodb Last Error: " << lastError << std::endl;
            }
        }
    }

    template <typename T> void remove_from_db(const std::string& collection, std::map<std::string, T>& params) {
        try {
            Poco::SharedPtr<Poco::MongoDB::DeleteRequest> request = _database.createDeleteRequest(collection);

            for (const auto& [key, value] : params) {
                request->selector().add(key, value);
            }

            _connection.sendRequest(*request);
        } catch (std::exception &e) {
            std::cout << "mongodb exception: " << e.what() << std::endl;

            std::string lastError = _database.getLastError(_connection);

            if (!lastError.empty()) {
                std::cout << "mongodb Last Error: " << lastError << std::endl;
            }
        }
    }
};

}
