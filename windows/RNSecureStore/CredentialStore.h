#pragma once
#include <optional>
#include <string>

namespace rnssecurestore {
std::string defaultApplicationId();
void setItem(const std::string &key, const std::string &value, const std::string &service,
             const std::string &applicationId);
std::optional<std::string> getItem(const std::string &key, const std::string &service,
                                   const std::string &applicationId);
void removeItem(const std::string &key, const std::string &service,
                const std::string &applicationId);
bool hasItem(const std::string &key, const std::string &service, const std::string &applicationId);
} // namespace rnssecurestore
