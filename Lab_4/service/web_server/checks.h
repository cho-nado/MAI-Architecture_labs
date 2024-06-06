#pragma once

#include <string>

bool check_name(const std::string& name, std::string& reason) {
    if (name.length() < 1) {
        reason = "Name must be at leas 1 signs";
        return false;
    }

    if (name.find('\t') != std::string::npos) {
        reason = "Name can't contain spaces";
        return false;
    }

    return true;
};

bool check_email(const std::string& email, std::string& reason) {

    if (email.find('\t') != std::string::npos) {
        reason = "EMail can't contain spaces";
        return false;
    }

    return true;
};