#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>

#include <yaml-cpp/yaml.h>



class ModifyRule;


class PathModifier {
  YAML::Node mNode;
  std::deque<std::unique_ptr<ModifyRule>> mModifyRules;

public:
  PathModifier(const std::wstring& filepath);
  ~PathModifier();

  std::optional<std::wstring> Modify(const std::wstring&) const;
};
