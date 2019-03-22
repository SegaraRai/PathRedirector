#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS

#include "Hook.hpp"
#include "Config.hpp"
#include "PathModifier.hpp"
#include "Util.hpp"

#include <MinHook.h>

#include <optional>
#include <stdexcept>
#include <string>

//#include <Ntifs.h>
#include <winternl.h>
#include <ntstatus.h>

using namespace std::literals;

using TNtCreateFile = NTSTATUS(WINAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
using TNtOpenFile = NTSTATUS(WINAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
using TNtSetInformationFile = NTSTATUS(WINAPI*)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

constexpr int FileRenameInformation = 10;
constexpr int FileRenameInformationEx = 65;

struct FILE_RENAME_INFORMATION {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS1)
  union {
    BOOLEAN ReplaceIfExists;  // FileRenameInformation
    ULONG Flags;              // FileRenameInformationEx
  } DUMMYUNIONNAME;
#else
  BOOLEAN ReplaceIfExists;
#endif
  HANDLE RootDirectory;
  ULONG FileNameLength;
  WCHAR FileName[1];
};


namespace {
  TNtCreateFile gOrgNtCreateFile;
  TNtOpenFile gOrgNtOpenFile;
  TNtSetInformationFile gOrgNtSetInformationFile;
  //TNtCreateFile gOrgZwCreateFile;
  //TNtOpenFile gOrgZwOpenFile;
  //TNtSetInformationFile gOrgZwSetInformationFile;

  std::optional<PathModifier> gPathModifierN;


  std::optional<std::wstring> ModifyPath(const std::wstring& src) {
#ifdef _DEBUG
    OutputDebugStringW(L"PathRedirector: [ModifyPath]\n     ");
    OutputDebugStringW(src.c_str());
    OutputDebugStringW(L"\n");
#endif

    const auto ret = gPathModifierN.value().Modify(src);

#ifdef _DEBUG
    if (ret) {
      OutputDebugStringW(L"  -> ");
      OutputDebugStringW(ret.value().c_str());
      OutputDebugStringW(L"\n");
    }
#endif

    return ret;
  }


  NTSTATUS ModifyObjectAttributes(OBJECT_ATTRIBUTES& objectAttributes, UNICODE_STRING& unicodeString, std::wstring& unicodeStringBuffer) noexcept {
    if (!objectAttributes.ObjectName->Buffer) {
#ifdef _DEBUG
      OutputDebugStringW(L"PathRedirector: [ModifyObjectAttributes] (null)");
#endif
      return STATUS_ALREADY_COMPLETE;
    }

    try {
      const std::wstring src(objectAttributes.ObjectName->Buffer, objectAttributes.ObjectName->Length / sizeof(wchar_t));

      const auto ret = ModifyPath(src);
      if (!ret) {
        return STATUS_ALREADY_COMPLETE;
      }

      unicodeStringBuffer = ret.value();
      unicodeString.Buffer = const_cast<PWSTR>(unicodeStringBuffer.c_str());
      unicodeString.Length = unicodeStringBuffer.size() * sizeof(wchar_t);    // Length does not include null terminator
      unicodeString.MaximumLength = unicodeStringBuffer.size() * sizeof(wchar_t);
      objectAttributes.ObjectName = &unicodeString;

      return STATUS_SUCCESS;
    } catch (...) {}

    return STATUS_UNSUCCESSFUL;
  }


  template<TNtCreateFile* OrgNtZwCreateFile>
  NTSTATUS WINAPI HookedNtZwCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength) {
#ifdef _DEBUG
    OutputDebugStringW(L"PathRedirector: [NtCreateFile] ");
    if (ObjectAttributes && ObjectAttributes->ObjectName->Buffer) {
      OutputDebugStringW(ObjectAttributes->ObjectName->Buffer);
    } else {
      OutputDebugStringW(L"(null)");
    }
    OutputDebugStringW(L"\n");
#endif

    try {
      do {
        if (!ObjectAttributes) {
          break;
        }
        std::wstring unicodeStringBuffer;
        UNICODE_STRING unicodeString;
        OBJECT_ATTRIBUTES NewObjectAttributes(*ObjectAttributes);
        if (const auto status = ModifyObjectAttributes(NewObjectAttributes, unicodeString, unicodeStringBuffer); status != STATUS_SUCCESS) {
          if (status == STATUS_ALREADY_COMPLETE) {
            break;
          }
          return status;
        }
        return (*OrgNtZwCreateFile)(FileHandle, DesiredAccess, &NewObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
      } while (false);
      return (*OrgNtZwCreateFile)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    } catch (...) {
      return STATUS_UNSUCCESSFUL;
    }
  }


  template<TNtOpenFile* OrgNtZwOpenFile>
  NTSTATUS WINAPI HookedNtZwOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions) {
#ifdef _DEBUG
    OutputDebugStringW(L"PathRedirector: [NtOpenFile]   ");
    if (ObjectAttributes && ObjectAttributes->ObjectName->Buffer) {
      OutputDebugStringW(ObjectAttributes->ObjectName->Buffer);
    } else {
      OutputDebugStringW(L"(null)");
    }
    OutputDebugStringW(L"\n");
#endif

    try {
      do {
        if (!ObjectAttributes) {
          break;
        }
        std::wstring unicodeStringBuffer;
        UNICODE_STRING unicodeString;
        OBJECT_ATTRIBUTES NewObjectAttributes(*ObjectAttributes);
        if (const auto status = ModifyObjectAttributes(NewObjectAttributes, unicodeString, unicodeStringBuffer); status != STATUS_SUCCESS) {
          if (status == STATUS_ALREADY_COMPLETE) {
            break;
          }
          return status;
        }
        return (*OrgNtZwOpenFile)(FileHandle, DesiredAccess, &NewObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
      } while (false);
      return (*OrgNtZwOpenFile)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    } catch (...) {
      return STATUS_UNSUCCESSFUL;
    }
  }


  template<TNtSetInformationFile* OrgNtZwSetInformationFile>
  NTSTATUS WINAPI HookedNtZwSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass) {
    try {
      do {
        if (FileInformationClass != FileRenameInformation && FileInformationClass != FileRenameInformationEx) {
          break;
        }
        if (!FileInformation) {
          break;
        }
        if (Length < sizeof(FILE_RENAME_INFORMATION)) {
          break;
        }

        const auto& fileRenameInformation = *reinterpret_cast<const FILE_RENAME_INFORMATION*>(FileInformation);

        if (fileRenameInformation.RootDirectory != NULL) {
          break;
        }

        if (fileRenameInformation.FileNameLength % sizeof(wchar_t) != 0) {
          break;
        }

        if (Length < sizeof(FILE_RENAME_INFORMATION) - sizeof(FILE_RENAME_INFORMATION::FileName) + fileRenameInformation.FileNameLength) {
          break;
        }

#ifdef _DEBUG
        OutputDebugStringW(L"PathRedirector: [NtSetInformationFile]\n");
#endif

        const std::wstring src(&fileRenameInformation.FileName[0], fileRenameInformation.FileNameLength / sizeof(wchar_t));
        const auto ret = ModifyPath(src);
        if (!ret) {
          break;
        }

        const auto newLength = sizeof(FILE_RENAME_INFORMATION) - sizeof(FILE_RENAME_INFORMATION::FileName) + (ret.value().size() + 1) * sizeof(wchar_t);
        auto newFileRenameInformationBuffer = std::make_unique<char[]>(newLength);
        auto ptrNewFileRenameInformationBuffer = reinterpret_cast<FILE_RENAME_INFORMATION*>(newFileRenameInformationBuffer.get());

        *ptrNewFileRenameInformationBuffer = fileRenameInformation;
        ptrNewFileRenameInformationBuffer->FileNameLength = ret.value().size() * sizeof(wchar_t);
        std::memcpy(&ptrNewFileRenameInformationBuffer->FileName[0], ret.value().c_str(), (ret.value().size() + 1) * sizeof(wchar_t));

        return (*OrgNtZwSetInformationFile)(FileHandle, IoStatusBlock, ptrNewFileRenameInformationBuffer, newLength, FileInformationClass);
      } while (false);
      return (*OrgNtZwSetInformationFile)(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    } catch (...) {
      return STATUS_UNSUCCESSFUL;
    }
  }


  void CheckMinHookResult(const std::string& funcName, MH_STATUS mhStatus) {
    if (mhStatus != MH_OK) {
      const std::string message = funcName + " failed with code "s + std::to_string(mhStatus);
      throw std::runtime_error(message);
    }
  }


  void Init() {
    const auto configFilepathN = GetEnvironmentVariableString(ConfigFileEnvironmentVariable);
    if (!configFilepathN) {
      throw std::runtime_error("config environment variable not set"s);
    }
    const auto configFilepath = ToAbsoluteFilepath(ExpandEnvironmentVariable(configFilepathN.value()));

#ifdef _DEBUG
    OutputDebugStringW(L"PathRedirector: config file = ");
    OutputDebugStringW(configFilepath.c_str());
    OutputDebugStringW(L"\n");
#endif

    const auto orgCurrentDirectory = GetCurrentDirectoryString();
    SetCurrentDirectoryW((configFilepath + L"\\.."s).c_str());

    gPathModifierN.emplace(configFilepath);

    SetCurrentDirectoryW(orgCurrentDirectory.c_str());

    CheckMinHookResult("MH_Initialize"s, MH_Initialize());
    CheckMinHookResult("MH_CreateHookApi of NtCreateFile"s, MH_CreateHookApi(L"NtDll.dll", "NtCreateFile", HookedNtZwCreateFile<&gOrgNtCreateFile>, reinterpret_cast<void**>(&gOrgNtCreateFile)));
    CheckMinHookResult("MH_CreateHookApi of NtOpenFile"s, MH_CreateHookApi(L"NtDll.dll", "NtOpenFile", HookedNtZwOpenFile<&gOrgNtOpenFile>, reinterpret_cast<void**>(&gOrgNtOpenFile)));
    CheckMinHookResult("MH_CreateHookApi of NtSetInformationFile"s, MH_CreateHookApi(L"NtDll.dll", "NtSetInformationFile", HookedNtZwSetInformationFile<&gOrgNtSetInformationFile>, reinterpret_cast<void**>(&gOrgNtSetInformationFile)));

    // not hooking Zw* because they points the same code position of Nt*
    //CheckMinHookResult("MH_CreateHookApi of ZwCreateFile"s, MH_CreateHookApi(L"NtDll.dll", "ZwCreateFile", HookedNtZwCreateFile<&gOrgZwCreateFile>, reinterpret_cast<void**>(&gOrgZwCreateFile)));
    //CheckMinHookResult("MH_CreateHookApi of ZwOpenFile"s, MH_CreateHookApi(L"NtDll.dll", "ZwOpenFile", HookedNtZwOpenFile<&gOrgZwOpenFile>, reinterpret_cast<void**>(&gOrgZwOpenFile)));
    //CheckMinHookResult("MH_CreateHookApi of ZwSetInformationFile"s, MH_CreateHookApi(L"NtDll.dll", "ZwSetInformationFile", HookedNtZwSetInformationFile<&gOrgZwSetInformationFile>, reinterpret_cast<void**>(&gOrgZwSetInformationFile)));
  }
}



void Hook() {
  Init();

  CheckMinHookResult("MH_EnableHook"s, MH_EnableHook(MH_ALL_HOOKS));

#ifdef _DEBUG
  OutputDebugStringW(L"PathRedirector: Hook loaded\n");
#endif
}


void Unhook() {
  CheckMinHookResult("MH_DisableHook"s, MH_DisableHook(MH_ALL_HOOKS));

  CheckMinHookResult("MH_Uninitialize"s, MH_Uninitialize());

#ifdef _DEBUG
  OutputDebugStringW(L"PathRedirector: Hook unloaded\n");
#endif
}
