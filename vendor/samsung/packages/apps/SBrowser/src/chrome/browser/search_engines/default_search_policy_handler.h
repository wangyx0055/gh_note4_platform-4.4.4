// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_ENGINES_DEFAULT_SEARCH_POLICY_HANDLER_H_
#define CHROME_BROWSER_SEARCH_ENGINES_DEFAULT_SEARCH_POLICY_HANDLER_H_

#include <vector>

#include "components/policy/core/browser/configuration_policy_handler.h"

namespace policy {

// ConfigurationPolicyHandler for the DefaultSearchEncodings policy.
class DefaultSearchEncodingsPolicyHandler
    : public TypeCheckingPolicyHandler {
 public:
  DefaultSearchEncodingsPolicyHandler();
  virtual ~DefaultSearchEncodingsPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultSearchEncodingsPolicyHandler);
};

// ConfigurationPolicyHandler for the default search policies.
class DefaultSearchPolicyHandler : public ConfigurationPolicyHandler {
 public:
  DefaultSearchPolicyHandler();
  virtual ~DefaultSearchPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  // Calls |CheckPolicySettings()| on each of the handlers in |handlers_|
  // and returns whether all of the calls succeeded.
  bool CheckIndividualPolicies(const PolicyMap& policies,
                               PolicyErrorMap* errors);

  // Returns whether there is a value for |policy_name| in |policies|.
  bool HasDefaultSearchPolicy(const PolicyMap& policies,
                              const char* policy_name);

  // Returns whether any default search policies are specified in |policies|.
  bool AnyDefaultSearchPoliciesSpecified(const PolicyMap& policies);

  // Returns whether the default search provider is disabled.
  bool DefaultSearchProviderIsDisabled(const PolicyMap& policies);

  // Returns whether the default search URL is set and valid.  On success, both
  // outparams (which must be non-NULL) are filled with the search URL.
  bool DefaultSearchURLIsValid(const PolicyMap& policies,
                               const base::Value** url_value,
                               std::string* url_string);

  // Make sure that the |path| is present in |prefs_|.  If not, set it to
  // a blank string.
  void EnsureStringPrefExists(PrefValueMap* prefs, const std::string& path);

  // Make sure that the |path| is present in |prefs_| and is a ListValue.  If
  // not, set it to an empty list.
  void EnsureListPrefExists(PrefValueMap* prefs, const std::string& path);

  // The ConfigurationPolicyHandler handlers for each default search policy.
  std::vector<TypeCheckingPolicyHandler*> handlers_;

  DISALLOW_COPY_AND_ASSIGN(DefaultSearchPolicyHandler);
};

}  // namespace policy

#endif  // CHROME_BROWSER_SEARCH_ENGINES_DEFAULT_SEARCH_POLICY_HANDLER_H_