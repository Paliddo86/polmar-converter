#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <filesystem>
#include <stdexcept>
#include <string>

#include "polmar_core.hpp"

namespace {

constexpr int IDC_OLD_EDIT = 1001;
constexpr int IDC_OLD_BROWSE = 1002;
constexpr int IDC_NEW_EDIT = 1003;
constexpr int IDC_NEW_BROWSE = 1004;
constexpr int IDC_OUT_EDIT = 1005;
constexpr int IDC_OUT_BROWSE = 1006;
constexpr int IDC_MODE_COMBO = 1007;
constexpr int IDC_DECIMAL_COMBO = 1008;
constexpr int IDC_FORCE_CHECK = 1009;
constexpr int IDC_CONVERT = 1010;
constexpr int IDC_OPEN_FOLDER = 1011;
constexpr int IDC_CLOSE = 1012;
constexpr int IDC_LOG_EDIT = 1013;
constexpr int IDC_SQ_COMBO = 1014;
constexpr int IDC_MIGRATE_CHECK = 1015;

HINSTANCE g_hInst = nullptr;
HWND g_hwnd = nullptr;
HFONT g_font = nullptr;

std::wstring getText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) {
        return L"";
    }
    std::wstring text(static_cast<std::size_t>(len) + 1, L'\0');
    GetWindowTextW(hwnd, text.data(), len + 1);
    text.resize(static_cast<std::size_t>(len));
    return text;
}

void setText(HWND hwnd, const std::wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
}

std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return L"";
    }
    int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) {
        return L"";
    }
    std::wstring out(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), count);
    return out;
}

std::wstring crlf(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size() + 16);
    for (wchar_t c : value) {
        if (c == L'\n' && (out.empty() || out.back() != L'\r')) {
            out += L"\r\n";
        } else {
            out += c;
        }
    }
    return out;
}

HWND makeControl(const wchar_t* cls,
                 const wchar_t* text,
                 DWORD style,
                 DWORD exStyle,
                 int x,
                 int y,
                 int w,
                 int h,
                 int id) {
    HWND control = CreateWindowExW(exStyle, cls, text, style, x, y, w, h, g_hwnd,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   g_hInst, nullptr);
    if (control && g_font) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
    }
    return control;
}

std::filesystem::path exeDir() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(std::wstring(buffer, len)).parent_path();
}

std::wstring openFileDialog(HWND owner, const std::wstring& title) {
    wchar_t buffer[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Polmar DAT (*.dat)\0*.dat\0Tutti i file (*.*)\0*.*\0\0";
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&ofn) ? std::wstring(buffer) : L"";
}

std::wstring saveFileDialog(HWND owner, const std::wstring& title, const std::wstring& defaultName) {
    wchar_t buffer[MAX_PATH] = L"";
    if (!defaultName.empty()) {
        wcsncpy(buffer, defaultName.c_str(), MAX_PATH - 1);
    }
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Polmar DAT (*.dat)\0*.dat\0Tutti i file (*.*)\0*.*\0\0";
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.lpstrDefExt = L"dat";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    return GetSaveFileNameW(&ofn) ? std::wstring(buffer) : L"";
}

void setIfExists(HWND edit, const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        setText(edit, path.wstring());
    }
}

void createControls(HWND hwnd) {
    const int labelX = 20;
    const int editX = 145;
    const int editW = 520;
    const int browseX = 675;
    const int browseW = 110;
    const int rowH = 24;

    makeControl(L"STATIC", L"File vecchio (.dat):", WS_CHILD | WS_VISIBLE,
                0, labelX, 20, 120, 22, -1);
    makeControl(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                WS_EX_CLIENTEDGE, editX, 18, editW, rowH, IDC_OLD_EDIT);
    makeControl(L"BUTTON", L"Sfoglia...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, browseX, 18, browseW, rowH, IDC_OLD_BROWSE);

    makeControl(L"STATIC", L"File nuovo/template (.dat):", WS_CHILD | WS_VISIBLE,
                0, labelX, 55, 120, 22, -1);
    makeControl(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                WS_EX_CLIENTEDGE, editX, 53, editW, rowH, IDC_NEW_EDIT);
    makeControl(L"BUTTON", L"Sfoglia...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, browseX, 53, browseW, rowH, IDC_NEW_BROWSE);

    makeControl(L"STATIC", L"File output (.dat):", WS_CHILD | WS_VISIBLE,
                0, labelX, 90, 120, 22, -1);
    makeControl(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                WS_EX_CLIENTEDGE, editX, 88, editW, rowH, IDC_OUT_EDIT);
    makeControl(L"BUTTON", L"Sfoglia...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, browseX, 88, browseW, rowH, IDC_OUT_BROWSE);

    makeControl(L"STATIC", L"Modalità:", WS_CHILD | WS_VISIBLE,
                0, labelX, 130, 80, 22, -1);
    HWND mode = makeControl(L"COMBOBOX", L"",
                            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                            WS_EX_CLIENTEDGE, 105, 127, 260, 200, IDC_MODE_COMBO);
    SendMessageW(mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"merge-full (canali completi)"));
    SendMessageW(mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"replace-all (svuota old vuoti)"));
    SendMessageW(mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"freq-only (solo frequenze)"));
    SendMessageW(mode, CB_SETCURSEL, 0, 0);

    makeControl(L"STATIC", L"Decimali:", WS_CHILD | WS_VISIBLE,
                0, 380, 130, 65, 22, -1);
    HWND decimal = makeControl(L"COMBOBOX", L"",
                               WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                               WS_EX_CLIENTEDGE, 445, 127, 110, 200, IDC_DECIMAL_COMBO);
    SendMessageW(decimal, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Auto"));
    SendMessageW(decimal, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Punto (.)"));
    SendMessageW(decimal, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Virgola (,)"));
    SendMessageW(decimal, CB_SETCURSEL, 0, 0);

    makeControl(L"STATIC", L"SQ Level:", WS_CHILD | WS_VISIBLE,
                0, 570, 130, 65, 22, -1);
    HWND sq = makeControl(L"COMBOBOX", L"",
                          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                          WS_EX_CLIENTEDGE, 635, 127, 145, 200, IDC_SQ_COMBO);
    SendMessageW(sq, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mantieni nuovo file"));
    SendMessageW(sq, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Off (0)"));
    SendMessageW(sq, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1 (consigliato)"));
    for (int level = 2; level <= 9; ++level) {
        std::wstring text = std::to_wstring(level);
        SendMessageW(sq, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
    SendMessageW(sq, CB_SETCURSEL, 2, 0);

    HWND migrate = makeControl(L"BUTTON", L"Migra impostazioni old",
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               0, 20, 160, 200, 22, IDC_MIGRATE_CHECK);
    SendMessageW(migrate, BM_SETCHECK, BST_CHECKED, 0);
    makeControl(L"BUTTON", L"Sovrascrivi output", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                0, 240, 160, 180, 22, IDC_FORCE_CHECK);

    makeControl(L"BUTTON", L"Converti", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 20, 195, 150, 32, IDC_CONVERT);
    makeControl(L"BUTTON", L"Apri cartella", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 180, 195, 150, 32, IDC_OPEN_FOLDER);
    makeControl(L"BUTTON", L"Chiudi", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 340, 195, 100, 32, IDC_CLOSE);

    makeControl(L"STATIC", L"Log:", WS_CHILD | WS_VISIBLE,
                0, 20, 235, 60, 20, -1);
    HWND log = makeControl(L"EDIT", L"",
                           WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                               ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                           WS_EX_CLIENTEDGE, 20, 255, 765, 320, IDC_LOG_EDIT);
    setText(log, L"Seleziona i file e premi Converti.\r\n"
                 L"Migra impostazioni old: attivo (Squelch 1, Beep Off, Battery Save On, TOT 15s, ecc.).\r\n"
                 L"SQ Level 1 (consigliato) evita il fruscio sui canali senza tono.\r\n");
}

void setDefaultPaths(HWND hwnd) {
    const auto dir = exeDir();
    const auto cwd = std::filesystem::current_path();

    HWND oldEdit = GetDlgItem(hwnd, IDC_OLD_EDIT);
    HWND newEdit = GetDlgItem(hwnd, IDC_NEW_EDIT);
    HWND outEdit = GetDlgItem(hwnd, IDC_OUT_EDIT);

    setIfExists(oldEdit, dir / L"vecchio.dat");
    if (getText(oldEdit).empty()) {
        setIfExists(oldEdit, cwd / L"vecchio.dat");
    }

    setIfExists(newEdit, dir / L"nuovo.dat");
    if (getText(newEdit).empty()) {
        setIfExists(newEdit, cwd / L"nuovo.dat");
    }

    std::filesystem::path output = dir / L"new.dat";
    if (getText(oldEdit).empty()) {
        output = cwd / L"new.dat";
    }
    setText(outEdit, output.wstring());
}

void onConvert(HWND hwnd) {
    const std::wstring oldPath = getText(GetDlgItem(hwnd, IDC_OLD_EDIT));
    const std::wstring newPath = getText(GetDlgItem(hwnd, IDC_NEW_EDIT));
    const std::wstring outPath = getText(GetDlgItem(hwnd, IDC_OUT_EDIT));
    if (oldPath.empty() || newPath.empty() || outPath.empty()) {
        MessageBoxW(hwnd, L"Seleziona il file vecchio, il file nuovo/template e il file output.",
                    L"File mancanti", MB_OK | MB_ICONWARNING);
        return;
    }

    const int modeIndex = static_cast<int>(
        SendMessageW(GetDlgItem(hwnd, IDC_MODE_COMBO), CB_GETCURSEL, 0, 0));
    Mode mode = Mode::MergeFull;
    if (modeIndex == 1) {
        mode = Mode::ReplaceAll;
    } else if (modeIndex == 2) {
        mode = Mode::FreqOnly;
    }

    const int decimalIndex = static_cast<int>(
        SendMessageW(GetDlgItem(hwnd, IDC_DECIMAL_COMBO), CB_GETCURSEL, 0, 0));
    DecimalMode decimal = DecimalMode::Auto;
    if (decimalIndex == 1) {
        decimal = DecimalMode::Dot;
    } else if (decimalIndex == 2) {
        decimal = DecimalMode::Comma;
    }

    const int sqIndex = static_cast<int>(
        SendMessageW(GetDlgItem(hwnd, IDC_SQ_COMBO), CB_GETCURSEL, 0, 0));
    int sqLevel = -1;
    if (sqIndex == 1) {
        sqLevel = 0;
    } else if (sqIndex >= 2) {
        sqLevel = sqIndex - 1;
    }

    const bool migrate =
        SendMessageW(GetDlgItem(hwnd, IDC_MIGRATE_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED;
    const bool force =
        SendMessageW(GetDlgItem(hwnd, IDC_FORCE_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED;

    ConvertOptions options;
    options.oldPath = std::filesystem::path(oldPath);
    options.newPath = std::filesystem::path(newPath);
    options.outputPath = std::filesystem::path(outPath);
    options.mode = mode;
    options.decimal = decimal;
    options.unknownTone = UnknownTone::Error;
    options.sqLevel = sqLevel;
    options.migrateOldGlobals = migrate;
    options.force = force;
    options.strict = false;
    options.dryRun = false;

    try {
        ConvertReport report = convertFiles(options);
        HWND log = GetDlgItem(hwnd, IDC_LOG_EDIT);
        setText(log, crlf(utf8ToWide(report.log)));
        const int len = GetWindowTextLengthW(log);
        SendMessageW(log, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
        SendMessageW(log, EM_SCROLLCARET, 0, 0);
        MessageBoxW(hwnd, L"Conversione completata.", L"Polmar Easy 2 Converter",
                    MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        const std::wstring message = L"Errore:\r\n" + utf8ToWide(e.what());
        setText(GetDlgItem(hwnd, IDC_LOG_EDIT), message);
        MessageBoxW(hwnd, message.c_str(), L"Errore", MB_OK | MB_ICONERROR);
    }
}

void onOpenFolder(HWND hwnd) {
    const std::wstring outPath = getText(GetDlgItem(hwnd, IDC_OUT_EDIT));
    if (outPath.empty()) {
        return;
    }
    std::filesystem::path path(outPath);
    std::filesystem::path dir = path.parent_path();
    if (dir.empty()) {
        dir = std::filesystem::current_path();
    }
    ShellExecuteW(hwnd, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            g_font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            createControls(hwnd);
            setDefaultPaths(hwnd);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_OLD_BROWSE: {
                    std::wstring path = openFileDialog(hwnd, L"Seleziona il vecchio .dat");
                    if (!path.empty()) {
                        setText(GetDlgItem(hwnd, IDC_OLD_EDIT), path);
                    }
                    return 0;
                }
                case IDC_NEW_BROWSE: {
                    std::wstring path = openFileDialog(hwnd, L"Seleziona il nuovo .dat / template");
                    if (!path.empty()) {
                        setText(GetDlgItem(hwnd, IDC_NEW_EDIT), path);
                    }
                    return 0;
                }
                case IDC_OUT_BROWSE: {
                    std::wstring path = saveFileDialog(hwnd, L"Salva il nuovo .dat", L"new.dat");
                    if (!path.empty()) {
                        setText(GetDlgItem(hwnd, IDC_OUT_EDIT), path);
                    }
                    return 0;
                }
                case IDC_CONVERT:
                    onConvert(hwnd);
                    return 0;
                case IDC_OPEN_FOLDER:
                    onOpenFolder(hwnd);
                    return 0;
                case IDC_CLOSE:
                    DestroyWindow(hwnd);
                    return 0;
                default:
                    return 0;
            }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    g_hInst = hInstance;
    SetProcessDPIAware();

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PolmarConvertWindow";
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) {
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Polmar Easy 2 Converter",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        820,
        660,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}



