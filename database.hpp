#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>
#include <list>
#include <regex>

#include "user.hpp"
#include "uninorms.h"

class Database {
public:
    Database() = default;
    void populateFromFile(std::ifstream& file);
    bool getCommand();

private:
    std::list<User> Users;

    void clean(std::string& str);
    std::u32string normalizeToUTF32(const std::string& str) const;
    std::string u32ToString(const std::u32string &str) const;
    bool validateName(std::u32string& name) const;
    bool validatePhoneNumber(std::string& phoneNumber) const;

    void add();
    void del();
    void list();
};