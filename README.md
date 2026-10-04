# CorralWindows

An experiment towards arranging Windows desktop icons into corrals.

## Experiment 1: observe Explorer's desktop

The console application finds Explorer's active desktop view, enumerates its
items (including virtual items such as Recycle Bin), and prints each display
name and `(x, y)` position. It only reads the view; it does not move icons or
change desktop settings.

It uses Windows Shell COM interfaces: `IShellWindows` with `SWC_DESKTOP`,
`IShellBrowser`, `IFolderView` and `IShellFolder`. Positions are the item's
upper-left corner in the desktop view's coordinate system, not absolute screen
coordinates. Names follow Explorer's display settings, including hidden file
extensions. The enumeration is not an atomic snapshot; keep the desktop stable
while running it.

## Build and run

Requires Windows, Visual Studio 2022 with **Desktop development with C++**,
the MSVC v143 toolset and a Windows 10/11 SDK. No external packages are needed.
This initial project targets x64.

Open `CorralWindows.sln`, select **Debug / x64** or **Release / x64**, and build.
Use **Ctrl+F5** to run with the console kept open, or run the executable from
a terminal.

Alternatively, in a Visual Studio Developer PowerShell or Developer Command Prompt:

```powershell
msbuild CorralWindows.sln /m /p:Configuration=Debug /p:Platform=x64
.\bin\x64\Debug\CorralWindows.exe
```

Example (illustrative; names and coordinates depend on your desktop):

```text
CorralWindows - Experiment 1
Name    (x, y) [desktop view coordinates]
Recycle Bin    (0, 0)
Notes    (0, 105)
2 item(s) printed.
```

Run as the signed-in user in an interactive session with Explorer running;
administrator privileges are not required. If the desktop view is unavailable,
the application reports an error and returns exit code 1. Individual item
failures are reported while enumeration continues, also resulting in exit code 1.
A complete enumeration, including an empty view, returns 0. Output redirected
to a file is UTF-8.

## Manual verification

Build both Debug and Release, then run from a terminal. Check that the printed
names match the desktop, including Recycle Bin and any public-desktop shortcuts.
Check a name containing non-ASCII characters, both in the console and redirected
output (`.\bin\x64\Debug\CorralWindows.exe > desktop-items.txt`).

With auto-arrange disabled, move one icon manually and run again: its position
should change and other stationary items should retain theirs. Check an empty
desktop view if available. Showing/hiding desktop icons can behave differently
across Explorer versions; it is not a substitute for checking an empty view.
Repeat with multiple monitors or different DPI settings if those are relevant.
The application itself should leave all icon positions unchanged.

These are live Explorer checks, not automated regression tests. No icon-moving
code or test harness is included in this first experiment.

### Initial validation (4 October 2026)

Debug and Release x64 builds succeeded without compiler warnings using Visual
Studio 2022, MSVC v143 and Windows SDK 10.0.26100.0. The restricted Codex build
session required explicit SDK discovery overrides, disabling the user vcpkg
import for the build invocation, and removing duplicate environment variable
names from the child build process. The ordinary Developer Command Prompt
invocation above remains to be checked outside that session.

A Debug run returned exit code 1 with `Find desktop view failed (HRESULT
0x80070005)` (access denied). This verified the error report, but enumeration
of real desktop items, Unicode output, positions and the empty-view case still
need the interactive manual checks above.

API references: [FindWindowSW](https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-findwindowsw),
[IFolderView::Items](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifolderview-items),
and [GetItemPosition](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifolderview-getitemposition).

## Licence

MIT; see [LICENSE](LICENSE). The source depends only on the Windows SDK and
standard C++ library.
