// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <functional>

#include "opentelemetry/sdk/instrumentationscope/instrumentation_scope.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace instrumentationscope
{

// A scope configurator is a function that returns the scope config for a given instrumentation
// scope.
template <class T>
using ScopeConfigurator = std::function<T(const InstrumentationScope &)>;

template <typename T>
class ScopeConfiguratorBuilder
{
public:
  /**
   * Constructor for a builder object that cam be used to create a scope configurator. A minimally
   * configured builder would build a ScopeConfigurator that applies the default_scope_config to
   * every instrumentation scope.
   * @param default_scope_config The default scope config that the built configurator should fall
   * back on.
   */
  explicit ScopeConfiguratorBuilder(T default_scope_config) noexcept
      : default_scope_config_(default_scope_config)
  {}

  /**
   * Allows the user to pass a generic function that evaluates an instrumentation scope through a
   * boolean check. If the check passes, the provided config is applied. Conditions are evaluated in
   * order.
   * @param scope_matcher a function that returns true if the scope being evaluated matches the
   * criteria defined by the function.
   * @param scope_config the scope configuration to return for the matched scope.
   * @return this
   */
  ScopeConfiguratorBuilder<T> AddCondition(
      std::function<bool(const InstrumentationScope &)> scope_matcher,
      T scope_config)
  {
    conditions_.push_back(ScopeConfiguratorBuilder::Condition(scope_matcher, scope_config));
    return this;
  }

  /**
   * A convenience condition that specifically matches the scope name of the scope being evaluated.
   * If the scope name matches to the provided string, then the provided scope configuration is
   * applied to the scope.
   * @param scope_name The scope name to which the config needs to be applied.
   * @param scope_config The scope config for the matching scopes.
   * @return this
   */
  ScopeConfiguratorBuilder<T> AddConditionNameEquals(nostd::string_view scope_name, T scope_config)
  {
    auto name_equals_matcher = [scope_name](const InstrumentationScope &scope_info) {
      return scope_info.GetName() == scope_name;
    };
    conditions_.push_back(ScopeConfiguratorBuilder::Condition(name_equals_matcher, scope_config));
    return this;
  }

  /**
   * Constructs the scope configurator object that can be used to retrieve scope config depending on
   * the instrumentation scope.
   * @return a configured scope configurator.
   */
  ScopeConfigurator<T> Build()
  {
    if (conditions_.size() == 0)
    {
      return [default_scope_config_ = this->default_scope_config_](const InstrumentationScope &) {
        return default_scope_config_;
      };
    }

    // Return a configurator that processes all the conditions
    return [conditions_ = this->conditions_, default_scope_config_ = this->default_scope_config_](
               const InstrumentationScope &scope_info) {
      for (Condition condition : conditions_)
      {
        if (condition.scope_matcher(scope_info))
        {
          return condition.scope_config;
        }
      }
      return default_scope_config_;
    };
  }

private:
  /**
   * An internal struct to encapsulate 'conditions' that can be applied to a
   * ScopeConfiguratorBuilder. The applied conditions influence the behavior of the generatred
   * ScopeConfigurator.
   */
  struct Condition
  {
    std::function<bool(const InstrumentationScope &)> scope_matcher;
    T scope_config;
  };

  T default_scope_config_;
  std::vector<Condition> conditions_;
};
}  // namespace instrumentationscope
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
