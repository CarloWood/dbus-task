#pragma once

#include "ErrorDomainManager.h"
#include <system_error>

namespace dbus::errors {

template<typename DOMAIN_TYPE, typename ENUM_TYPE>
class ErrorCategory : public std::error_category
{
  using domain_type = DOMAIN_TYPE;
  using enum_type = ENUM_TYPE;

 private:
  domain_type const m_domain;

 public:
  ErrorCategory() { ErrorDomainManager::instantiate().register_domain(get_domain(static_cast<ENUM_TYPE>(0)), &m_domain); }

  char const* name() const noexcept override final { /* Use ADL */ return get_domain(static_cast<ENUM_TYPE>(0)); }
  std::string message(int ev) const override final { /* Use ADL */ return to_string(static_cast<ENUM_TYPE>(ev)); }
};

} // namespace dbus::errors
