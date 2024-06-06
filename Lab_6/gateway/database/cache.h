#pragma once

#include <string>
#include <iostream>
#include <memory>


namespace database {

class Cache {
private:
    static std::mutex _mtx;
    std::shared_ptr<std::iostream> _stream;

    Cache();
public:
    static Cache get();
    void put(const std::string& key, const std::string& value);
    std::string get(const std::string & key);
};

}
