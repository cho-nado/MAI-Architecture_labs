#pragma once

#include <string>
#include <vector>
#include <optional>

#include <Poco/JSON/Object.h>

namespace database {

struct Package {
    double weight;
    double height;
    double width;
    double length;
    const std::string to_string() const;
};

class Delivery {
private:
    long _delivery_id;
    long _sender_id;
    long _receiver_id;
    std::string _date;

    Package _package;

public:

    static Delivery  fromJSON(const std::string & str);

    long        get_delivery_id() const;
    long        get_sender_id() const;
    long        get_receiver_id() const;
    std::string get_date() const;
    Package     get_package() const;

    const std::string  get_package_as_string() const;

    void set_delivery_id(long delivery_id);
    void set_sender_id(long sender_id);
    void set_receiver_id(long receiver_id);
    void set_date(const std::string& date);
    void set_package(Package package);

    static std::optional<Delivery> read_by_delivery_id(long delivery_id);
    static std::vector<Delivery> read_by_sender_id(long sender_id);
    static std::vector<Delivery> read_by_receiver_id(long receiver_id);
    void   add();
    void   update();
    void   remove();
    Poco::JSON::Object::Ptr toJSON() const;

};

}
