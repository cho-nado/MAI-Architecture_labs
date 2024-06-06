#include <exception>
#include <mutex>

#include <redis-cpp/stream.h>
#include <redis-cpp/execute.h>

#include "config/config.h"

#include "cache.h"

namespace database {

std::mutex Cache::_mtx;

Cache::Cache() {
    std::string host = Config::get().get_cache_host();
    std::string port = Config::get().get_cache_port();

    std::cout << "cache host:" << host <<" port:" << port << std::endl;
    
    _stream = rediscpp::make_stream(host, port);
}

Cache Cache::get() {
    static Cache instance;
    return instance;
}

void Cache::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::mutex> lck(_mtx);

    rediscpp::value response = rediscpp::execute(*_stream, "set", key,
                                                 value, "ex", "60");
}

std::string Cache::get(const std::string& key) {
    try {
        std::unique_lock<std::mutex> lck(_mtx);

        rediscpp::value response = rediscpp::execute(*_stream, "get", key);

        if (response.is_error_message() || response.empty()) {
            return "";
        }

        return response.as<std::string>();
    } catch(...) {}

    return "";
}

}
