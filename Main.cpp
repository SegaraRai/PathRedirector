#include "Hook.hpp"

#include <optional>
#include <string>

#include <Windows.h>

using namespace std::literals;



namespace {
  bool gHooked = false;
}



BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID Reserved) {
  switch (ulReason) {
    case DLL_PROCESS_ATTACH:
      try {
        Hook();
        gHooked = true;
        return TRUE;
      } catch (std::exception& exception) {
        const std::string message = "Failed to Hook: "s + exception.what();
        MessageBoxA(NULL, message.c_str(), "PathRedirector", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
      } catch (...) {
        MessageBoxW(NULL, L"Failed to Hook", L"PathRedirector", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
      }
      return FALSE;

    case DLL_PROCESS_DETACH:
      try {
        if (gHooked) {
          Unhook();
          gHooked = false;
        }
      } catch (...) {}
      break;
  }
  return TRUE;
}
