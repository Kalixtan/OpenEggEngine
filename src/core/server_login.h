#pragma once

#include <string>

struct ServerLoginDetails {
    std::string username;
    std::string email;
    std::string password;
    std::string passwordConfirm;
    bool createAccount = false;
    bool remember = true;
};
