#pragma once

#include "utils/Singleton.h"
#include <map>
#include <system_error>

namespace dbus {

class ErrorDomainBase
{
 protected:
  static std::error_code s_unknown_domain;
  static std::error_code s_unknown_error;

 public:
  virtual ~ErrorDomainBase() = default;

 public:
  virtual std::error_code get_error_code(std::string const& member_name) const
  {
    // Default.
    return s_unknown_domain;
  }
};

class ErrorDomainManager : public Singleton<ErrorDomainManager>
{
  friend_Instance;
 private:
  ErrorDomainManager() = default;
  ~ErrorDomainManager() = default;
  ErrorDomainManager(ErrorDomainManager const&) = delete;

  std::map<std::string, ErrorDomainBase const*> m_error_domains;
  ErrorDomainBase m_unknown_domain;

 public:
  ErrorDomainBase const& domain(std::string const& domain_name)
  {
    auto domain = m_error_domains.find("DBus:" + domain_name);
    if (domain == m_error_domains.end())
    {
      Dout(dc::warning, "Unknown error category \"" << domain_name << "\".");
      return m_unknown_domain;
    }
    return *domain->second;
  }

  void register_domain(std::string const& domain_name, ErrorDomainBase const* error_domain)
  {
    std::cout << "ErrorDomainManager::register_domain(\"" << domain_name << "\", " << (void*)error_domain << ")" << std::endl;
    auto res = m_error_domains.emplace(domain_name, error_domain);
    // Don't call register_domain twice for the same domain_name.
    ASSERT(res.second);
  }
};

} // namespace dbus
