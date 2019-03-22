#pragma once

#include <optional>
#include <string>


std::wstring UTF8toWString(const std::string& utf8String);
std::optional<std::wstring> GetEnvironmentVariableString(const std::wstring& environmentVariableName);
std::wstring ExpandEnvironmentVariable(const std::wstring& filepath);
std::wstring GetCurrentDirectoryString();
std::wstring ToAbsoluteFilepath(const std::wstring& filepath);
