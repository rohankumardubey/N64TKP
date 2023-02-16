#ifndef ERROR_FACTORY_HXX
#define ERROR_FACTORY_HXX
#include <string>
#include <stdexcept>

struct ErrorFactory {
    static std::runtime_error generate_exception(std::string data) {
        return std::runtime_error(data);
    }
};
#endif