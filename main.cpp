#include <iostream>
#include <string>
#include "database.hpp"

int main(int argc, char *argv[]) {
    Database users;

    if (argc > 2) {
        std::cout << "Invalid input" << std::endl;
    }
    if (argc == 2) {
        // if a filename is provided as input, parse names and phone numbers from it
        std::ifstream file(argv[1]);

        // check to see if the file opened properly
        if(!file.is_open()) {
            std::cout << "The file provided was unable to opened" << std::endl;
            return -1;
        }

        // if the file is valid initilize the database with the information
        users.populateFromFile(file);
        
        file.close();
    }

    // while the user has not quit, continue reading in commands
    while(users.getCommand()) {}
    
    return 0;
}