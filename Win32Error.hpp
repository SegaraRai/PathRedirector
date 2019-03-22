#pragma once

#include <stdexcept>
#include <string>

#include <Windows.h>


class Win32Error : public std::runtime_error {
  DWORD errorCode;

public:
  Win32Error(DWORD error, const std::string& funcName);
  Win32Error(const std::string& funcName);
  Win32Error(DWORD error);
  Win32Error();

  DWORD GetErrorCode() const;
};
