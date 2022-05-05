#include <string>
#include <functional>

struct User {
    std::u32string name;
    std::string phoneNumber;

    User(const std::u32string& n, const std::string& p) : name(n), phoneNumber(p) {};
    User(User&&) = default;

    auto operator<=>(const User&) const = default;
};