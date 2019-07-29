/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019
 *
 *  @file  ConfigScope.h
 *  @brief Manages a full scope with many conig entries (or sub-scopes).
 *  @note Status: ALPHA
 */

#ifndef MABE_CONFIG_SCOPE_H
#define MABE_CONFIG_SCOPE_H

#include "base/map.h"

#include "ConfigEntry.h"

namespace mabe {

  // Set of multiple config entries.
  class ConfigScope : public ConfigEntry {
  protected:
    emp::map< std::string, emp::Ptr<ConfigEntry> > entries;

    template <typename T, typename... ARGS>
    T & Add(const std::string & name, ARGS &&... args) {
      auto new_ptr = emp::NewPtr<T>(name, std::forward<ARGS>(args)...);
      entries[name] = new_ptr;
      return *new_ptr;
    }
  public:
    ConfigScope(const std::string & _name,
                 const std::string & _desc,
                 emp::Ptr<ConfigScope> _scope)
      : ConfigEntry(_name, _desc, _scope) { }
    ConfigScope(const ConfigScope & in) : ConfigEntry(in) {
      for (const auto & x : in.entries) {
        entries[x.first] = x.second->Clone();
      }
    }
    ConfigScope(ConfigScope &&) = default;

    ~ConfigScope() {
      // Clear up all entries.
      for (auto & x : entries) { x.second.Delete(); }
    }

    emp::Ptr<ConfigScope> AsScopePtr() override { return this; }

    void UpdateDefault() override {
      // Recursively update all defaults within the structure.
      for (auto & x : entries) x.second->UpdateDefault();
      default_val = ""; /* @CAO: Need to spell out? */
    }

    // Get an entry out of this scope; 
    emp::Ptr<ConfigEntry> GetEntry(std::string in_name) {
      // Lookup this next entry is in the var list.
      auto it = entries.find(in_name);

      // If this name is unknown, fail!
      if (it == entries.end()) return nullptr;

      // Otherwise return the entry.
      return it->second;
    }

    // Lookup a variable, scanning outer scopes if needed
    emp::Ptr<ConfigEntry> LookupEntry(std::string in_name, bool scan_scopes=true) override {
      // See if this next entry is in the var list.
      auto it = entries.find(in_name);

      // If this name is unknown, check with the parent scope!
      if (it == entries.end()) {
        if (scope.IsNull() || !scan_scopes) return nullptr;  // No parent?  Just fail...
        return scope->LookupEntry(in_name);
      }

      // Otherwise we found it!
      return it->second;
    }

    // Lookup a variable, scanning outer scopes if needed (in constant context!)
    emp::Ptr<const ConfigEntry> LookupEntry(std::string in_name, bool scan_scopes=true) const override {
      // See if this entry is in the var list.
      auto it = entries.find(in_name);

      // If this name is unknown, check with the parent scope!
      if (it == entries.end()) {
        if (scope.IsNull() || !scan_scopes) return nullptr;  // No parent?  Just fail...
        return scope->LookupEntry(in_name);
      }

      // Otherwise we found it!
      return it->second;
    }

    template <typename VAR_T, typename DEFAULT_T>
    ConfigEntry_Linked<VAR_T> & LinkVar(VAR_T & var,
                                    const std::string & name,
                                    const std::string & desc,
                                    DEFAULT_T default_val) {
      return Add<ConfigEntry_Linked<VAR_T>>(name, var, desc, this);
    }

    ConfigScope & AddScope(const std::string & name, const std::string & desc) {
      return Add<ConfigScope>(name, desc, this);
    }

    ConfigEntry & Write(std::ostream & os=std::cout, const std::string & prefix="") override {
      if (desc.size()) os << prefix << "// " << desc << "\n";

      os << prefix << name << " = {\n";
      for (auto & x : entries) {
        x.second->Write(os, prefix+"  ");
      }
      os << prefix << "}\n";
      return *this;
    }

    emp::Ptr<ConfigEntry> Clone() const override { return emp::NewPtr<ConfigScope>(*this); }
  };

}
#endif