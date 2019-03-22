#pragma once

#include "Util.hpp"

#include <string>

#include <Windows.h>

#include <yaml-cpp/yaml.h>



namespace YAML {
  template<>
  struct convert<std::wstring> {
    static bool decode(const Node& node, std::wstring& rhs) {
      if (!node.IsScalar()) {
        return false;
      }
      rhs = UTF8toWString(node.as<std::string>());
      return true;
    }
  };
}
