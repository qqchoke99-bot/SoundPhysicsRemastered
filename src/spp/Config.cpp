#include "spp/Config.hpp"
namespace spp::cfg {
Settings& settings() {
    static Settings s;
    return s;
}
}
