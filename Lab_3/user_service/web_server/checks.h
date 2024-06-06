#pragma once

#include <string>

bool check_name(const std::string& name, std::string& reason) {
    if (name.length() < 3) {
        reason = "Name must be at leas 3 signs";
        return false;
    }

    if (name.find(' ') != std::string::npos) {
        reason = "Name can't contain spaces";
        return false;
    }

    if (name.find('\t') != std::string::npos) {
        reason = "Name can't contain spaces";
        return false;
    }

    return true;
};

bool check_email(const std::string& email, std::string& reason) {
    if (email.find('@') == std::string::npos) {
        reason = "Email must contain @";
        return false;
    }

    if (email.find(' ') != std::string::npos) {
        reason = "EMail can't contain spaces";
        return false;
    }

    if (email.find('\t') != std::string::npos) {
        reason = "EMail can't contain spaces";
        return false;
    }

    return true;
};