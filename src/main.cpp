// SPDX-License-Identifier: MIT
#include <windows.h>
#include <ole2.h>
#include <exdisp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl/client.h>

#include <fcntl.h>
#include <conio.h>
#include <io.h>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace
{
    struct TaskMemoryDeleter
    {
        void operator()(void* memory) const noexcept
        {
            CoTaskMemFree(memory);
        }
    };

    class ComApartment
    {
    public:
        ComApartment() : result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}

        ~ComApartment()
        {
            if (SUCCEEDED(result))
            {
                CoUninitialize();
            }
        }

        ComApartment(const ComApartment&) = delete;
        ComApartment& operator=(const ComApartment&) = delete;

        const HRESULT result;
    };

    int ReportError(const wchar_t* operation, HRESULT result)
    {
        std::wcerr << operation << L" failed (HRESULT 0x"
                   << std::hex << std::uppercase << std::setw(8)
                   << std::setfill(L'0') << static_cast<unsigned long>(result)
                   << L").\n";
        return 1;
    }

    void WaitForExitKey()
    {
        DWORD inputMode  = 0;
        DWORD outputMode = 0;
        if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &inputMode) ||
            !GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &outputMode))
        {
            return;
        }

        std::wcout << L"\nPress any key to exit..." << std::flush;
        _getwch();
        std::wcout << L"\n";
    }

    HRESULT FindDesktopView(ComPtr<IFolderView>& folderView)
    {
        ComPtr<IShellWindows> shellWindows;
        HRESULT result = CoCreateInstance(CLSID_ShellWindows, nullptr,
            CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(shellWindows.GetAddressOf()));
        if (FAILED(result))
        {
            return result;
        }

        VARIANT location = {};
        VARIANT root     = {};
        long window      = 0;
        ComPtr<IDispatch> desktop;
        result = shellWindows->FindWindowSW(&location, &root, SWC_DESKTOP,
            &window, SWFO_NEEDDISPATCH, desktop.GetAddressOf());
        if (result != S_OK || !desktop)
        {
            return FAILED(result) ? result : HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }

        ComPtr<IServiceProvider> services;
        result = desktop.As(&services);
        if (FAILED(result))
        {
            return result;
        }

        ComPtr<IShellBrowser> browser;
        result = services->QueryService(SID_STopLevelBrowser,
            IID_PPV_ARGS(browser.GetAddressOf()));
        if (FAILED(result))
        {
            return result;
        }

        ComPtr<IShellView> view;
        result = browser->QueryActiveShellView(view.GetAddressOf());
        if (FAILED(result))
        {
            return result;
        }

        return view.As(&folderView);
    }

    int PrintDesktopItems()
    {
        ComPtr<IFolderView> view;
        HRESULT result = FindDesktopView(view);
        if (FAILED(result))
        {
            std::wcerr << L"Explorer's desktop view is unavailable. Run in an interactive "
                          L"Windows session with Explorer running.\n";
            return ReportError(L"Find desktop view", result);
        }

        ComPtr<IShellFolder> folder;
        result = view->GetFolder(IID_PPV_ARGS(folder.GetAddressOf()));
        if (FAILED(result))
        {
            return ReportError(L"Get desktop folder", result);
        }

        ComPtr<IEnumIDList> items;
        result = view->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(items.GetAddressOf()));
        if (FAILED(result))
        {
            return ReportError(L"Enumerate desktop items", result);
        }

        unsigned int count = 0;
        bool itemFailed    = false;
        std::wcout << L"CorralWindows - Experiment 1\n"
                      L"Name\t(x, y) [desktop view coordinates]\n";

        for (;;)
        {
            PITEMID_CHILD rawItem = nullptr;
            result = items->Next(1, &rawItem, nullptr);
            std::unique_ptr<ITEMID_CHILD, TaskMemoryDeleter> item(rawItem);
            if (result == S_FALSE)
            {
                break;
            }
            if (FAILED(result))
            {
                return ReportError(L"Read next desktop item", result);
            }
            if (!item)
            {
                return ReportError(L"Read next desktop item", E_UNEXPECTED);
            }

            STRRET displayName = {};
            result = folder->GetDisplayNameOf(item.get(), SHGDN_NORMAL, &displayName);
            if (FAILED(result))
            {
                ReportError(L"Read item name", result);
                itemFailed = true;
                continue;
            }

            PWSTR rawName = nullptr;
            result = StrRetToStrW(&displayName, item.get(), &rawName);
            std::unique_ptr<wchar_t, TaskMemoryDeleter> name(rawName);
            if (FAILED(result))
            {
                ReportError(L"Convert item name", result);
                itemFailed = true;
                continue;
            }

            POINT position = {};
            result = view->GetItemPosition(item.get(), &position);
            if (FAILED(result))
            {
                std::wcerr << L"Item: " << name.get() << L"\n";
                ReportError(L"Read item position", result);
                itemFailed = true;
                continue;
            }

            std::wcout << name.get() << L"\t(" << position.x << L", "
                       << position.y << L")\n";
            ++count;
        }

        std::wcout << count << L" item(s) printed.\n";
        return itemFailed ? 1 : 0;
    }
}

int wmain()
{
    // Unicode console output; redirected output is UTF-8.
    const int outputMode = _isatty(_fileno(stdout)) ? _O_U16TEXT : _O_U8TEXT;
    const int errorMode  = _isatty(_fileno(stderr)) ? _O_U16TEXT : _O_U8TEXT;
    if (_setmode(_fileno(stdout), outputMode) == -1 ||
        _setmode(_fileno(stderr), errorMode) == -1)
    {
        return 1;
    }

    ComApartment apartment;
    const int exitCode = FAILED(apartment.result)
        ? ReportError(L"Initialize COM", apartment.result)
        : PrintDesktopItems();
    WaitForExitKey();
    return exitCode;
}
