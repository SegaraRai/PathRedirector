#include "PathModifier.hpp"
#include "Util.hpp"
#include "YamlWstring.hpp"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <Windows.h>

using namespace std::literals;



class ModifyRule {
  std::string mType;
  bool mLast;

protected:
  ModifyRule(const YAML::Node& node) :
    mType(node["type"].as<std::string>()),
    mLast(node["last"].as<bool>())
  {}

public:
  const std::string& GetType() const {
    return mType;
  }

  const bool IsLast() const {
    return mLast;
  }

  virtual std::optional<std::wstring> Modify(const std::wstring& src) const = 0;
};



class ModifyRuleFilepath : public ModifyRule {
public:
  static const std::string_view gType;

private:
  std::wstring sourcePrefix;
  std::wstring destinationPrefix;

  static std::wstring GetPrefix(const std::wstring& filepath) {
    const auto fqPath = ToAbsoluteFilepath(ExpandEnvironmentVariable(filepath));
    // TODO: support UNC paths
    return L"\\??\\"s + fqPath;
  }

public:
  ModifyRuleFilepath(const YAML::Node& node) :
    ModifyRule(node)
  {
    sourcePrefix = GetPrefix(node["source"].as<std::wstring>());
    destinationPrefix = GetPrefix(node["destination"].as<std::wstring>());
  }

  std::optional<std::wstring> Modify(const std::wstring& src) const {
    if (src.size() < sourcePrefix.size()) {
      return std::nullopt;
    }
    if (src.size() > sourcePrefix.size() && src[sourcePrefix.size()] != L'\\') {
      return std::nullopt;
    }
    if (_wcsnicmp(src.c_str(), sourcePrefix.c_str(), sourcePrefix.size()) != 0) {
      return std::nullopt;
    }
    return destinationPrefix + src.substr(sourcePrefix.size());
  }
};

const std::string_view ModifyRuleFilepath::gType = "filepath"sv;



template<typename T, typename... Rest>
std::unique_ptr<ModifyRule> CreateModifyRuleImpl(const YAML::Node& node, const std::string& type) {
  if (type == T::gType) {
    return std::make_unique<T>(node);
  }
  if constexpr (sizeof...(Rest) == 0) {
    throw std::runtime_error("unknown type specified");
  } else {
    return CreateModifyRuleImpl<Rest...>(node, type);
  }
}


std::unique_ptr<ModifyRule> CreateModifyRule(const YAML::Node& node) {
  const auto type = node["type"].as<std::string>();
  return CreateModifyRuleImpl<ModifyRuleFilepath>(node, type);
}



PathModifier::PathModifier(const std::wstring& filepath) {
  {
    std::ifstream ifs(filepath);
    mNode = YAML::Load(ifs);
  }

  for (const auto& ynRule : mNode["rules"]) {
    mModifyRules.emplace_back(CreateModifyRule(ynRule));
  }
}


PathModifier::~PathModifier() = default;


std::optional<std::wstring> PathModifier::Modify(const std::wstring& src) const {
  bool modified = false;
  std::wstring dest = src;
  for (const auto& modifyRule : mModifyRules) {
    const auto ret = modifyRule->Modify(dest);
    if (ret) {
      modified = true;
      dest = ret.value();
      if (modifyRule->IsLast()) {
        break;
      }
    }
  }
  return modified ? std::make_optional(dest) : std::nullopt;
}
