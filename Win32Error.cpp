#include "Win32Error.hpp"

#include <string>
#include <Windows.h>

using namespace std::literals;



Win32Error::Win32Error(DWORD error, const std::string& funcName) :
  std::runtime_error(funcName + " failed with code "s + std::to_string(error)),
  errorCode(error)
{}


Win32Error::Win32Error(const std::string& funcName) :
  Win32Error(GetLastError(), funcName)
{}


Win32Error::Win32Error(DWORD error) :
  Win32Error(error, "Win32 API call"s)
{}


Win32Error::Win32Error() :
  Win32Error(GetLastError())
{}


DWORD Win32Error::GetErrorCode() const {
  return errorCode;
}
