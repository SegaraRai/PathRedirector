#include "Util.hpp"
#include "Config.hpp"
#include "Win32Error.hpp"

#include <memory>
#include <optional>
#include <string>

#include <Windows.h>

using namespace std::literals;



std::wstring UTF8toWString(const std::string& utf8String) {
  if (utf8String.empty()) {
    return {};
  }

  int wideCharLength = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, utf8String.data(), static_cast<int>(utf8String.size()), NULL, 0);
  if (!wideCharLength) {
    throw Win32Error("MultiByteToWideChar"s);
  }
  auto wideCharBuffer = std::make_unique<wchar_t[]>(wideCharLength);
  int wideCharLength2 = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, utf8String.data(), static_cast<int>(utf8String.size()), wideCharBuffer.get(), wideCharLength);
  if (!wideCharLength2) {
    throw Win32Error("MultiByteToWideChar"s);
  }
  return std::wstring(wideCharBuffer.get(), wideCharLength2);
}


std::optional<std::wstring> GetEnvironmentVariableString(const std::wstring& environmentVariableName) {
  const DWORD size = GetEnvironmentVariableW(environmentVariableName.c_str(), NULL, 0);
  if (!size) {
    if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    throw Win32Error("GetEnvironmentVariableW"s);
  }
  auto buffer = std::make_unique<wchar_t[]>(size);
  DWORD size2 = GetEnvironmentVariableW(environmentVariableName.c_str(), buffer.get(), size);
  if (!size2) {
    if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
      return std::nullopt;
    }
    throw Win32Error("GetEnvironmentVariableW"s);
  }
  return std::make_optional<std::wstring>(buffer.get(), size2);
}


std::wstring ExpandEnvironmentVariable(const std::wstring& filepath) {
  const DWORD size = ExpandEnvironmentStringsW(filepath.c_str(), NULL, 0);
  if (!size) {
    throw Win32Error("ExpandEnvironmentStringsW"s);
  }
  auto buffer = std::make_unique<wchar_t[]>(size);
  DWORD size2 = ExpandEnvironmentStringsW(filepath.c_str(), buffer.get(), size);
  if (!size2) {
    throw Win32Error("ExpandEnvironmentStringsW"s);
  }
  // NOTE: size2 includes the terminating null character
  return std::wstring(buffer.get(), size2 - 1);
}


std::wstring GetCurrentDirectoryString() {
  const DWORD size = GetCurrentDirectoryW(0, NULL);
  if (!size) {
    throw Win32Error("GetCurrentDirectoryW"s);
  }
  auto buffer = std::make_unique<wchar_t[]>(size);
  DWORD size2 = GetCurrentDirectoryW(size, buffer.get());
  if (!size2) {
    throw Win32Error("GetCurrentDirectoryW"s);
  }
  return std::wstring(buffer.get(), size2);
}


std::wstring ToAbsoluteFilepath(const std::wstring& filepath) {
  const DWORD size = GetFullPathNameW(filepath.c_str(), 0, NULL, NULL);
  if (!size) {
    throw Win32Error("GetFullPathNameW"s);
  }
  auto buffer = std::make_unique<wchar_t[]>(size);
  DWORD size2 = GetFullPathNameW(filepath.c_str(), size, buffer.get(), NULL);
  if (!size2) {
    throw Win32Error("GetFullPathNameW"s);
  }
  // trim trailing backslash
  if (buffer[size2 - 1] == L'\\') {
    buffer[--size2] = L'\0';
  }
  return std::wstring(buffer.get(), size2);
}
