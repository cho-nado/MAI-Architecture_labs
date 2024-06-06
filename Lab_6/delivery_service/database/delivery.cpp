#include <sstream>
#include <exception>
#include <memory>

#include <Poco/Data/SessionFactory.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/DateTime.h>
#include <Poco/MongoDB/ObjectId.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>

#include "config/config.h"

#include "delivery.h"
#include "database.h"

namespace database {

Delivery Delivery::fromJSON(const std::string& str) {
    int start = str.find("_id");
    int end = str.find(",", start);

    std::string s1 = str.substr(0, start - 1);
    std::string s2 = str.substr(end + 1);

    // std::cout << s1 << s2 << std::endl;
    // std::cout << "from json:" << str << std::endl;
    Delivery delivery;

    Poco::JSON::Parser parser;
    Poco::Dynamic::Var result = parser.parse(s1 + s2);
    Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

    delivery.set_delivery_id(object->getValue<long>("delivery_id"));
    delivery.set_sender_id(object->getValue<long>("sender_id"));
    delivery.set_receiver_id(object->getValue<long>("receiver_id"));
    delivery.set_date(object->getValue<std::string>("date"));
    Poco::JSON::Object::Ptr object_array = object->getObject("package");
    delivery._package.weight = object_array->getValue<double>("weight");
    delivery._package.height = object_array->getValue<double>("height");
    delivery._package.width = object_array->getValue<double>("width");
    delivery._package.length = object_array->getValue<double>("length");

    return delivery;
}

long Delivery::get_delivery_id() const {
    return _delivery_id;
}

long Delivery::get_sender_id() const {
    return _sender_id;
}

long Delivery::get_receiver_id() const {
    return _receiver_id;
}

std::string Delivery::get_date() const {
    return _date;
}

Package Delivery::get_package() const {
    return _package;
}

const std::string Delivery::get_package_as_string() const {
    return _package.to_string();
}

const std::string Package::to_string() const {
    std::stringstream ss;
    ss << weight << ", " << height << ", " << width << ", " << length;

    return ss.str();
}

void Delivery::set_delivery_id(long delivery_id) {
    _delivery_id = delivery_id;
}

void Delivery::set_sender_id(long sender_id) {
    _sender_id = sender_id;
}

void Delivery::set_receiver_id(long receiver_id) {
    _receiver_id = receiver_id;
}

void Delivery::set_date(const std::string& date) {
    _date = date;
}

void Delivery::set_package(Package package) {
    _package = std::move(package);
}

std::optional<Delivery> Delivery::read_by_delivery_id(long delivery_id) {
    std::optional<Delivery> result;
    std::map<std::string, long> params;
    params["delivery_id"] = delivery_id;

    std::vector<std::string> results = database::Database::get().get_from_db("delivery", params);

    if(!results.empty()) {
        result = fromJSON(results[0]);
    }
    
    return result;
}

std::vector<Delivery> Delivery::read_by_sender_id(long sender_id) {
    std::vector<Delivery> result;
    std::map<std::string, long> params;
    params["sender_id"] = sender_id;

    std::vector<std::string> results = database::Database::get().get_from_db("delivery", params);

    for(std::string& res : results) {
        result.push_back(fromJSON(res));
    }
    
    return result;
}

std::vector<Delivery> Delivery::read_by_receiver_id(long receiver_id) {
    std::vector<Delivery> result;
    std::map<std::string, long> params;
    params["receiver_id"] = receiver_id;

    std::vector<std::string> results = database::Database::get().get_from_db("delivery", params);

    for(std::string& res : results) {
        result.push_back(fromJSON(res));
    }
    
    return result;
}

void Delivery::add() {
    database::Database::get().insert_into_db("delivery", toJSON());
}

void Delivery::update() {
    std::map<std::string, long> params;
    params["delivery_id"] = _delivery_id;
    database::Database::get().update_db("delivery", params, toJSON());
}

void Delivery::remove() {
    std::map<std::string, long> params;
    params["delivery_id"] = _delivery_id;
    database::Database::get().remove_from_db("delivery", params);
}

Poco::JSON::Object::Ptr Delivery::toJSON() const {
    Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

    root->set("delivery_id", _delivery_id);
    root->set("sender_id", _sender_id);
    root->set("receiver_id", _receiver_id);
    root->set("date", _date);

    Poco::JSON::Object::Ptr package = new Poco::JSON::Object();

    package->set("weight", _package.weight);
    package->set("height", _package.height);
    package->set("width", _package.width);
    package->set("length", _package.length);

    root->set("package", package);


    return root;
}

}
