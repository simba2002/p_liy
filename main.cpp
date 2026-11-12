#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <wininet.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cwctype>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#define WM_UPDATE_RESPONSE (WM_USER + 101)
#define WM_APPLY_THEME     (WM_USER + 102)

#define IDC_POPUP_EDIT     2001
#define IDC_POPUP_COPY     2002
#define IDC_POPUP_CLOSE    2003
#define IDC_POPUP_THEME    2004

#define IDC_SET_SAVE       1001
#define IDC_SET_CANCEL     1002
#define IDC_SET_THEME      1003
#define IDC_SET_RESET_PRM  1004
#define IDC_SET_LANG_COMBO 1005

enum HotkeyAction {
    HK_CAPTURE = 1,
    HK_SHOW,
    HK_EXIT,
    HK_SETTINGS,
    HK_CONSOLE
};

enum AppLanguage {
    LANG_EN = 0,
    LANG_UK = 1,
    LANG_RU = 2
};

struct KeyConfig {
    UINT vkCapture = VK_F8;
    UINT vkShow = VK_F9;
    UINT vkExit = VK_F4;
    UINT vkSettings = VK_INSERT;
    UINT vkConsole = VK_HOME;
    AppLanguage lang = LANG_EN;
    bool darkTheme = true;
    std::string selectedModel = "gemini-3.5-flash-lite";
    std::string customPrompt = "";
} g_keys;

struct LocStrings {
    const wchar_t* wTitle;
    const wchar_t* wNoResponse;
    const wchar_t* wLoading;
    const wchar_t* wCopyBtn;
    const wchar_t* wCopiedBtn;
    const wchar_t* wCloseBtn;
    const wchar_t* wSettingsTitle;
    const wchar_t* wCapLabel;
    const wchar_t* wShowLabel;
    const wchar_t* wConsoleLabel;
    const wchar_t* wSettingsLabel;
    const wchar_t* wExitLabel;
    const wchar_t* wLangLabel;
    const wchar_t* wModelLabel;
    const wchar_t* wPromptLabel;
    const wchar_t* wResetPromptBtn;
    const wchar_t* wKeysLabel;
    const wchar_t* wSaveBtn;
    const wchar_t* wCancelBtn;
    const wchar_t* wThemeDarkBtn;
    const wchar_t* wThemeLightBtn;
    const wchar_t* wSuccessSave;
    const wchar_t* wErrKey;
    const char* aiPrompt;
};

// Using explicit Unicode escapes (\uXXXX) prevents MSVC codepage / UTF-8 BOM mojibake
const LocStrings LOC[] = {
    // 0: English
    {
        L"AI Assistant Response",
        L"No response yet. Press the capture key.",
        L"Analyzing screenshot, please wait...",
        L"Copy",
        L"Copied!",
        L"Close",
        L"Settings & Keys",
        L"Capture Screen:",
        L"Toggle Window:",
        L"Debug Console:",
        L"Settings Menu:",
        L"Exit Program:",
        L"Interface Language:",
        L"Gemini Model:",
        L"AI Prompt (sent with screenshot):",
        L"Default",
        L"Gemini API Keys (One per line):",
        L"Save",
        L"Cancel",
        L"Theme: Dark",
        L"Theme: Light",
        L"Settings successfully saved to config.ini!",
        L"Invalid key name (e.g. f1-f12, insert, delete, home, end, a-z)",
        "Analyze this screenshot carefully. If there is a question, test, or task, provide a concise, direct, and correct answer in English. If it contains code or an error, explain the fix concisely in English. IMPORTANT: Format your response as clean plain text only. Do NOT use Markdown (no **, *, #) and do NOT use LaTeX math syntax (no $, $$, \\frac, \\text, \\quad). Use standard plain-text math notation and Unicode symbols (e.g., x^2, /, *, <, >, <=, >=, !=)."
    },
    // 1: Українська
    {
        // "Відповідь ШІ"
        L"\u0412\u0456\u0434\u043F\u043E\u0432\u0456\u0434\u044C \u0428\u0406",
        // "Відповіді ще немає. Натисніть клавішу аналізу екрана."
        L"\u0412\u0456\u0434\u043F\u043E\u0432\u0456\u0434\u0456 \u0449\u0435 \u043D\u0435\u043C\u0430\u0454. \u041D\u0430\u0442\u0438\u0441\u043D\u0456\u0442\u044C \u043A\u043B\u0430\u0432\u0456\u0448\u0443 \u0430\u043D\u0430\u043B\u0456\u0437\u0443 \u0435\u043A\u0440\u0430\u043D\u0430.",
        // "Аналізую скріншот, зачекайте..."
        L"\u0410\u043D\u0430\u043B\u0456\u0437\u0443\u044E \u0441\u043A\u0440\u0456\u043D\u0448\u043E\u0442, \u0437\u0430\u0447\u0435\u043A\u0430\u0439\u0442\u0435...",
        // "Копіювати"
        L"\u041A\u043E\u043F\u0456\u044E\u0432\u0430\u0442\u0438",
        // "Скопійовано!"
        L"\u0421\u043A\u043E\u043F\u0456\u0439\u043E\u0432\u0430\u043D\u043E!",
        // "Закрити"
        L"\u0417\u0430\u043A\u0440\u0438\u0442\u0438",
        // "Налаштування та Ключі"
        L"\u041D\u0430\u043B\u0430\u0448\u0442\u0443\u0432\u0430\u043D\u043D\u044F \u0442\u0430 \u041A\u043B\u044E\u0447\u0456",
        // "Зробити скріншот:"
        L"\u0417\u0440\u043E\u0431\u0438\u0442\u0438 \u0441\u043A\u0440\u0456\u043D\u0448\u043E\u0442:",
        // "Показати вікно:"
        L"\u041F\u043E\u043A\u0430\u0437\u0430\u0442\u0438 \u0432\u0456\u043A\u043D\u043E:",
        // "Консоль / Дебаг:"
        L"\u041A\u043E\u043D\u0441\u043E\u043B\u044C / \u0414\u0435\u0431\u0430\u0433:",
        // "Меню налаштувань:"
        L"\u041C\u0435\u043D\u044E \u043D\u0430\u043B\u0430\u0448\u0442\u0443\u0432\u0430\u043D\u044C:",
        // "Вихід з програми:"
        L"\u0412\u0438\u0445\u0456\u0434 \u0437 \u043F\u0440\u043E\u0433\u0440\u0430\u043C\u0438:",
        // "Мова інтерфейсу:"
        L"\u041C\u043E\u0432\u0430 \u0456\u043D\u0442\u0435\u0440\u0444\u0435\u0439\u0441\u0443:",
        // "Модель Gemini:"
        L"\u041C\u043E\u0434\u0435\u043B\u044C Gemini:",
        // "Промпт для ШІ (відправляється зі скріншотом):"
        L"\u041F\u0440\u043E\u043C\u043F\u0442 \u0434\u043B\u044F \u0428\u0406 (\u0437\u0456 \u0441\u043A\u0440\u0456\u043D\u0448\u043E\u0442\u043E\u043C):",
        // "Стандартний"
        L"\u0421\u0442\u0430\u043D\u0434\u0430\u0440\u0442\u043D\u0438\u0439",
        // "API-ключі Gemini (по одному на рядок):"
        L"API-\u043A\u043B\u044E\u0447\u0456 Gemini (\u043F\u043E \u043E\u0434\u043D\u043E\u043C\u0443 \u043D\u0430 \u0440\u044F\u0434\u043E\u043A):",
        // "Зберегти"
        L"\u0417\u0431\u0435\u0440\u0435\u0433\u0442\u0438",
        // "Скасувати"
        L"\u0421\u043A\u0430\u0441\u0443\u0432\u0430\u0442\u0438",
        // "Тема: Темна"
        L"\u0422\u0435\u043C\u0430: \u0422\u0435\u043C\u043D\u0430",
        // "Тема: Світла"
        L"\u0422\u0435\u043C\u0430: \u0421\u0432\u0456\u0442\u043B\u0430",
        // "Налаштування успішно збережено у config.ini!"
        L"\u041D\u0430\u043B\u0430\u0448\u0442\u0443\u0432\u0430\u043D\u043D\u044F \u0443\u0441\u043F\u0456\u0448\u043D\u043E \u0437\u0431\u0435\u0440\u0435\u0436\u0435\u043D\u043E \u0443 config.ini!",
        // "Некоректна назва клавіші (f1-f12, insert, delete, home, end, a-z)"
        L"\u041D\u0435\u043A\u043E\u0440\u0435\u043A\u0442\u043D\u0430 \u043D\u0430\u0437\u0432\u0430 \u043A\u043B\u0430\u0432\u0456\u0448\u0456 (f1-f12, insert, delete, home, end, a-z)",
        "Analyze this screenshot carefully. If there is a question, test, or task, provide a concise, direct, and correct answer in Ukrainian. If it contains code or an error, explain the fix concisely in Ukrainian. IMPORTANT: Format your response as clean plain text only. Do NOT use Markdown (no **, *, #) and do NOT use LaTeX math syntax (no $, $$, \\frac, \\text, \\quad). Use standard plain-text math notation and Unicode symbols (e.g., x^2, /, *, <, >, <=, >=, !=)."
    },
    // 2: Русский
    {
        // "Ответ ИИ"
        L"\u041E\u0442\u0432\u0435\u0442 \u0418\u0418",
        // "Ответа пока нет. Нажмите клавишу захвата экрана."
        L"\u041E\u0442\u0432\u0435\u0442\u0430 \u043F\u043E\u043A\u0430 \u043D\u0435\u0442. \u041D\u0430\u0436\u043C\u0438\u0442\u0435 \u043A\u043B\u0430\u0432\u0438\u0448\u0443 \u0437\u0430\u0445\u0432\u0430\u0442\u0430 \u044D\u043A\u0440\u0430\u043D\u0430.",
        // "Анализирую скриншот, подождите..."
        L"\u0410\u043D\u0430\u043B\u0438\u0437\u0438\u0440\u0443\u044E \u0441\u043A\u0440\u0438\u043D\u0448\u043E\u0442, \u043F\u043E\u0434\u043E\u0436\u0434\u0438\u0442\u0435...",
        // "Копировать"
        L"\u041A\u043E\u043F\u0438\u0440\u043E\u0432\u0430\u0442\u044C",
        // "Скопировано!"
        L"\u0421\u043A\u043E\u043F\u0438\u0440\u043E\u0432\u0430\u043D\u043E!",
        // "Закрыть"
        L"\u0417\u0430\u043A\u0440\u044B\u0442\u044C",
        // "Настройки и Ключи"
        L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u0438 \u041A\u043B\u044E\u0447\u0438",
        // "Сделать скриншот:"
        L"\u0421\u0434\u0435\u043B\u0430\u0442\u044C \u0441\u043A\u0440\u0438\u043D\u0448\u043E\u0442:",
        // "Показать окно:"
        L"\u041F\u043E\u043A\u0430\u0437\u0430\u0442\u044C \u043E\u043A\u043D\u043E:",
        // "Консоль / Дебаг:"
        L"\u041A\u043E\u043D\u0441\u043E\u043B\u044C / \u0414\u0435\u0431\u0430\u0433:",
        // "Меню настроек:"
        L"\u041C\u0435\u043D\u044E \u043D\u0430\u0441\u0442\u0440\u043E\u0435\u043A:",
        // "Выход из кода:"
        L"\u0412\u044B\u0445\u043E\u0434 \u0438\u0437 \u043A\u043E\u0434\u0430:",
        // "Язык интерфейса:"
        L"\u042F\u0437\u044B\u043A \u0438\u043D\u0442\u0435\u0440\u0444\u0435\u0439\u0441\u0430:",
        // "Модель Gemini:"
        L"\u041C\u043E\u0434\u0435\u043B\u044C Gemini:",
        // "Промпт для ИИ (отправляется со скриншотом):"
        L"\u041F\u0440\u043E\u043C\u043F\u0442 \u0434\u043B\u044F \u0418\u0418 (\u0441\u043E \u0441\u043A\u0440\u0438\u043D\u0448\u043E\u0442\u043E\u043C):",
        // "По умолч."
        L"\u041F\u043E \u0443\u043C\u043E\u043B\u0447.",
        // "API-ключи Gemini (по одному на строку):"
        L"API-\u043A\u043B\u044E\u0447\u0438 Gemini (\u043F\u043E \u043E\u0434\u043D\u043E\u043C\u0443 \u043D\u0430 \u0441\u0442\u0440\u043E\u043A\u0443):",
        // "Сохранить"
        L"\u0421\u043E\u0445\u0440\u0430\u043D\u0438\u0442\u044C",
        // "Отмена"
        L"\u041E\u0442\u043C\u0435\u043D\u0430",
        // "Тема: Тёмная"
        L"\u0422\u0435\u043C\u0430: \u0422\u0451\u043C\u043D\u0430\u044F",
        // "Тема: Светлая"
        L"\u0422\u0435\u043C\u0430: \u0421\u0432\u0435\u0442\u043B\u0430\u044F",
        // "Настройки успешно сохранены в config.ini!"
        L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u0443\u0441\u043F\u0435\u0448\u043D\u043E \u0441\u043E\u0445\u0440\u0430\u043D\u0435\u043D\u044B \u0432 config.ini!",
        // "Неверная клавиша (f1-f12, insert, delete, home, end, a-z)"
        L"\u041D\u0435\u0432\u0435\u0440\u043D\u0430\u044F \u043A\u043B\u0430\u0432\u0438\u0448\u0430 (f1-f12, insert, delete, home, end, a-z)",
        "Analyze this screenshot carefully. If there is a question, test, or task, provide a concise, direct, and correct answer in Russian. If it contains code or an error, explain the fix concisely in Russian. IMPORTANT: Format your response as clean plain text only. Do NOT use Markdown (no **, *, #) and do NOT use LaTeX math syntax (no $, $$, \\frac, \\text, \\quad). Use standard plain-text math notation and Unicode symbols (e.g., x^2, /, *, <, >, <=, >=, !=)."
    }
};

std::vector<std::string> API_KEYS;
const std::vector<std::wstring> PRESET_MODELS = {
    L"gemini-3.5-flash-lite",
    L"gemini-3.5-flash",
    L"gemini-3.6-flash",
    L"gemini-3.8-flash"
};

std::string last_working_key = "";
std::string latest_answer = "";
std::atomic<bool> is_loading(false);

HWND g_hPopupWnd = NULL;
HWND g_hSettingsWnd = NULL;
HWND g_hConsoleHostWnd = NULL;
std::atomic<bool> g_consoleVisible(false);

// Shared theme brushes
HBRUSH g_hBrDarkBg = NULL;
HBRUSH g_hBrDarkCtrl = NULL;
HBRUSH g_hBrLightBg = NULL;
HBRUSH g_hBrLightCtrl = NULL;

void InitThemeBrushes() {
    if (!g_hBrDarkBg)    g_hBrDarkBg    = CreateSolidBrush(RGB(24, 24, 28));
    if (!g_hBrDarkCtrl)  g_hBrDarkCtrl  = CreateSolidBrush(RGB(36, 36, 42));
    if (!g_hBrLightBg)   g_hBrLightBg   = CreateSolidBrush(RGB(240, 240, 240));
    if (!g_hBrLightCtrl) g_hBrLightCtrl = CreateSolidBrush(RGB(255, 255, 255));
}

void ApplyWindowDarkTitleBar(HWND hWnd, bool dark) {
    BOOL useDark = dark ? TRUE : FALSE;
    // 20 = DWMWA_USE_IMMERSIVE_DARK_MODE (Win11 & Win10 20H1+), 19 = fallback for older Win10
    if (FAILED(DwmSetWindowAttribute(hWnd, 20, &useDark, sizeof(useDark)))) {
        DwmSetWindowAttribute(hWnd, 19, &useDark, sizeof(useDark));
    }
}

static BOOL CALLBACK ApplyChildThemeProc(HWND hChild, LPARAM lParam) {
    bool dark = (lParam != 0);
    wchar_t cls[64] = { 0 };
    GetClassNameW(hChild, cls, 63);
    if (_wcsicmp(cls, L"COMBOBOX") == 0) {
        SetWindowTheme(hChild, dark ? L"DarkMode_CFD" : L"CFD", NULL);
    }
    else {
        SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : L"Explorer", NULL);
    }
    InvalidateRect(hChild, NULL, TRUE);
    return TRUE;
}

void ApplyThemeToWindowAndChildren(HWND hWnd, bool dark) {
    if (!hWnd || !IsWindow(hWnd)) return;
    InitThemeBrushes();
    ApplyWindowDarkTitleBar(hWnd, dark);
    EnumChildWindows(hWnd, ApplyChildThemeProc, (LPARAM)(dark ? 1 : 0));
    InvalidateRect(hWnd, NULL, TRUE);
    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
}

void DrawThemedButton(const DRAWITEMSTRUCT* dis, bool dark) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    bool focused = (dis->itemState & ODS_FOCUS) != 0;

    COLORREF bgCol = dark
        ? (pressed ? RGB(62, 62, 72) : RGB(45, 45, 54))
        : (pressed ? RGB(215, 215, 220) : RGB(228, 228, 232));
    COLORREF borderCol = dark
        ? (focused ? RGB(100, 140, 220) : RGB(78, 78, 90))
        : (focused ? RGB(0, 120, 215) : RGB(170, 170, 175));
    COLORREF txtCol = dark ? RGB(240, 240, 245) : RGB(20, 20, 24);

    HBRUSH hBr = CreateSolidBrush(bgCol);
    FillRect(hdc, &rc, hBr);
    DeleteObject(hBr);

    HBRUSH hBorderBr = CreateSolidBrush(borderCol);
    FrameRect(hdc, &rc, hBorderBr);
    DeleteObject(hBorderBr);

    wchar_t text[128] = { 0 };
    GetWindowTextW(dis->hwndItem, text, 127);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, txtCol);
    if (pressed) OffsetRect(&rc, 1, 1);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static const char* CIPHER_ALPHABET = "q8Nw2Km9Px5Rz7Lv4Tb6Yc3Hd1Fg0JkE_nSpWuAyCeGiMoU-aBDrsItVjXlhOZfQ";

static uint64_t MixState64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

static uint8_t RotL8(uint8_t v, unsigned int s) {
    s &= 7;
    return (uint8_t)((v << s) | (v >> ((8 - s) & 7)));
}

static uint8_t RotR8(uint8_t v, unsigned int s) {
    s &= 7;
    return (uint8_t)((v >> s) | (v << ((8 - s) & 7)));
}

static uint32_t ComputeKeyMac(const std::string& plain, uint32_t salt) {
    uint64_t h = 0x736F6D6570736575ULL ^ ((uint64_t)salt * 0x9E3779B97F4A7C15ULL);
    for (size_t i = 0; i < plain.size(); ++i) {
        h ^= (uint64_t)(uint8_t)plain[i] + (i * 0x517CC1B727220A95ULL);
        h = MixState64(h);
    }
    return (uint32_t)(h ^ (h >> 32));
}

static std::string CustomArmorEncode(const std::vector<uint8_t>& data) {
    std::string out = "v9$";
    uint32_t acc = 0;
    int bits = 0;
    for (uint8_t b : data) {
        acc = (acc << 8) | b;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            out.push_back(CIPHER_ALPHABET[(acc >> bits) & 0x3F]);
        }
    }
    if (bits > 0) {
        out.push_back(CIPHER_ALPHABET[(acc << (6 - bits)) & 0x3F]);
    }
    return out;
}

static bool CustomArmorDecode(const std::string& str, std::vector<uint8_t>& out) {
    if (str.size() < 12 || str.compare(0, 3, "v9$") != 0) return false;
    out.clear();
    uint32_t acc = 0;
    int bits = 0;
    for (size_t i = 3; i < str.size(); ++i) {
        const char* p = std::strchr(CIPHER_ALPHABET, str[i]);
        if (!p) return false;
        uint32_t val = (uint32_t)(p - CIPHER_ALPHABET);
        acc = (acc << 6) | val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back((uint8_t)((acc >> bits) & 0xFF));
        }
    }
    return out.size() > 8;
}

std::string EncryptApiKey(const std::string& plain) {
    if (plain.empty()) return "";
    uint32_t salt = ComputeKeyMac(plain, 0xA5C3F179U) ^ 0x3C6EF372U;
    uint32_t mac = ComputeKeyMac(plain, salt);

    std::vector<uint8_t> buf(8 + plain.size());
    for (int i = 0; i < 4; ++i) buf[i] = (uint8_t)((salt >> (i * 8)) & 0xFF);

    uint64_t state = MixState64(0x4F2A8B1E9C7D6351ULL ^ salt);
    uint8_t prev = (uint8_t)(salt & 0xFF);

    for (int i = 0; i < 4; ++i) {
        state = MixState64(state + i + prev);
        uint8_t m = (uint8_t)((mac >> (i * 8)) & 0xFF);
        uint8_t k1 = (uint8_t)(state & 0xFF);
        uint8_t k2 = (uint8_t)((state >> 19) & 0xFF);
        uint8_t c = RotL8(m ^ k1 ^ prev, (unsigned int)(state >> 11)) + k2;
        buf[4 + i] = c;
        prev = c;
    }

    for (size_t i = 0; i < plain.size(); ++i) {
        state = MixState64(state ^ ((uint64_t)prev << 17) ^ (uint64_t)(i * 0x1F3D));
        uint8_t p = (uint8_t)plain[i];
        uint8_t k1 = (uint8_t)(state & 0xFF);
        uint8_t k2 = (uint8_t)((state >> 23) & 0xFF);
        uint8_t k3 = (uint8_t)((state >> 41) & 0xFF);
        uint8_t step = RotL8(p ^ k1, (unsigned int)(state >> 7)) + k2;
        uint8_t c = RotL8(step ^ prev, (unsigned int)(state >> 31)) ^ k3;
        buf[8 + i] = c;
        prev = c ^ (uint8_t)(state >> 53);
    }

    return CustomArmorEncode(buf);
}

std::string DecryptApiKeyOrRaw(const std::string& input) {
    std::vector<uint8_t> buf;
    if (!CustomArmorDecode(input, buf)) return input;

    uint32_t salt = 0;
    for (int i = 0; i < 4; ++i) salt |= ((uint32_t)buf[i] << (i * 8));

    uint64_t state = MixState64(0x4F2A8B1E9C7D6351ULL ^ salt);
    uint8_t prev = (uint8_t)(salt & 0xFF);
    uint32_t mac = 0;

    for (int i = 0; i < 4; ++i) {
        state = MixState64(state + i + prev);
        uint8_t c = buf[4 + i];
        uint8_t k1 = (uint8_t)(state & 0xFF);
        uint8_t k2 = (uint8_t)((state >> 19) & 0xFF);
        uint8_t m = RotR8((uint8_t)(c - k2), (unsigned int)(state >> 11)) ^ k1 ^ prev;
        mac |= ((uint32_t)m << (i * 8));
        prev = c;
    }

    std::string plain;
    plain.resize(buf.size() - 8);
    for (size_t i = 0; i < plain.size(); ++i) {
        state = MixState64(state ^ ((uint64_t)prev << 17) ^ (uint64_t)(i * 0x1F3D));
        uint8_t c = buf[8 + i];
        uint8_t k1 = (uint8_t)(state & 0xFF);
        uint8_t k2 = (uint8_t)((state >> 23) & 0xFF);
        uint8_t k3 = (uint8_t)((state >> 41) & 0xFF);
        uint8_t step = RotR8(c ^ k3, (unsigned int)(state >> 31)) ^ prev;
        uint8_t p = RotR8((uint8_t)(step - k2), (unsigned int)(state >> 7)) ^ k1;
        plain[i] = (char)p;
        prev = c ^ (uint8_t)(state >> 53);
    }

    if (ComputeKeyMac(plain, salt) != mac) return input;
    return plain;
}

std::string MaskKey(const std::string& key) {
    std::string enc = EncryptApiKey(key);
    if (enc.length() <= 14) return enc;
    return enc.substr(0, 10) + "..." + enc.substr(enc.length() - 4);
}

static std::string TrimString(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size);
    return wstr;
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

std::string EscapeIniNewlines(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\r') continue;
        if (c == '\n') out += "\\n";
        else if (c == '\\') out += "\\\\";
        else out.push_back(c);
    }
    return out;
}

std::string UnescapeIniNewlines(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            if (s[i + 1] == 'n') {
                out += "\r\n";
                ++i;
                continue;
            }
            if (s[i + 1] == '\\') {
                out.push_back('\\');
                ++i;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string GetActivePrompt() {
    std::string trimmed = TrimString(g_keys.customPrompt);
    if (!trimmed.empty()) return trimmed;
    return LOC[g_keys.lang].aiPrompt;
}

std::wstring GetModuleDir() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    return path.substr(0, pos + 1);
}

std::string Base64Encode(const BYTE* buf, unsigned int bufLen) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int val = 0, valb = -6;
    for (unsigned int i = 0; i < bufLen; ++i) {
        val = (val << 8) + buf[i];
        valb += 8;
        while (valb >= 0) {
            ret.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) ret.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    while (ret.size() % 4) ret.push_back('=');
    return ret;
}

std::string EscapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
        case '"':  ss << "\\\""; break;
        case '\\': ss << "\\\\"; break;
        case '\b': ss << "\\b";  break;
        case '\f': ss << "\\f";  break;
        case '\n': ss << "\\n";  break;
        case '\r': ss << "\\r";  break;
        case '\t': ss << "\\t";  break;
        default:   ss << c;      break;
        }
    }
    return ss.str();
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

bool CaptureScreenJpeg(std::vector<BYTE>& outBytes) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

    Gdiplus::Bitmap bitmap(hBitmap, NULL);
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid);

    Gdiplus::EncoderParameters encoderParams;
    encoderParams.Count = 1;
    encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
    encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    encoderParams.Parameter[0].NumberOfValues = 1;
    ULONG quality = 85;
    encoderParams.Parameter[0].Value = &quality;

    IStream* pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    bitmap.Save(pStream, &clsid, &encoderParams);

    STATSTG stat;
    pStream->Stat(&stat, STATFLAG_NONAME);
    DWORD size = (DWORD)stat.cbSize.QuadPart;

    outBytes.resize(size);
    LARGE_INTEGER liZero;
    liZero.QuadPart = 0;
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    ULONG bytesRead = 0;
    pStream->Read(outBytes.data(), size, &bytesRead);
    pStream->Release();

    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    return true;
}

bool SendGeminiRequest(const std::string& apiKey, const std::string& model, const std::string& jsonPayload, std::string& outResponse, int& outStatusCode) {
    HINTERNET hInternet = InternetOpenA("GeminiCppClient", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetConnectA(hInternet, "generativelanguage.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::string path = "/v1beta/models/" + model + ":generateContent?key=" + apiKey;
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    std::string headers = "Content-Type: application/json\r\n";
    BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)jsonPayload.c_str(), (DWORD)jsonPayload.length());

    if (!sent) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL);
    outStatusCode = (int)statusCode;

    char buffer[4096];
    DWORD bytesRead = 0;
    outResponse.clear();
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        outResponse += buffer;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return true;
}

static void AppendUtf8Codepoint(std::string& out, unsigned int cp) {
    if (cp <= 0x7F) {
        out.push_back((char)cp);
    }
    else if (cp <= 0x7FF) {
        out.push_back((char)(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0xFFFF) {
        out.push_back((char)(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0x10FFFF) {
        out.push_back((char)(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
}

static bool ParseHex4(const std::string& s, size_t pos, unsigned int& outVal) {
    if (pos + 4 > s.size()) return false;
    unsigned int val = 0;
    for (size_t k = 0; k < 4; ++k) {
        char c = s[pos + k];
        val <<= 4;
        if (c >= '0' && c <= '9') val |= (c - '0');
        else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
        else return false;
    }
    outVal = val;
    return true;
}

static void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

static bool ExtractBracedGroup(const std::string& str, size_t openBracePos, std::string& outInner, size_t& outAfterClosePos) {
    if (openBracePos >= str.size() || str[openBracePos] != '{') return false;
    int depth = 0;
    for (size_t i = openBracePos; i < str.size(); ++i) {
        if (str[i] == '{') depth++;
        else if (str[i] == '}') {
            depth--;
            if (depth == 0) {
                outInner = str.substr(openBracePos + 1, i - openBracePos - 1);
                outAfterClosePos = i + 1;
                return true;
            }
        }
    }
    return false;
}

static bool IsSimpleToken(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (!std::isalnum(c) && c != '.') return false;
    }
    return true;
}

std::string CleanResponseText(std::string text) {
    size_t pos = 0;
    while ((pos = text.find("\\frac", pos)) != std::string::npos) {
        size_t p1 = pos + 5;
        while (p1 < text.size() && (text[p1] == ' ' || text[p1] == '\t')) p1++;
        std::string num, den;
        size_t afterNum = 0, afterDen = 0;
        if (ExtractBracedGroup(text, p1, num, afterNum)) {
            size_t p2 = afterNum;
            while (p2 < text.size() && (text[p2] == ' ' || text[p2] == '\t')) p2++;
            if (ExtractBracedGroup(text, p2, den, afterDen)) {
                std::string lhs = IsSimpleToken(num) ? num : "(" + num + ")";
                std::string rhs = IsSimpleToken(den) ? den : "(" + den + ")";
                std::string repl = lhs + "/" + rhs;
                text.replace(pos, afterDen - pos, repl);
                pos += repl.size();
                continue;
            }
        }
        pos += 5;
    }

    const char* textMacros[] = { "\\text", "\\mathrm", "\\mathbf", "\\textit", "\\textbf" };
    for (const char* macro : textMacros) {
        size_t mLen = std::strlen(macro);
        pos = 0;
        while ((pos = text.find(macro, pos)) != std::string::npos) {
            size_t p1 = pos + mLen;
            while (p1 < text.size() && (text[p1] == ' ' || text[p1] == '\t')) p1++;
            std::string inner;
            size_t afterInner = 0;
            if (ExtractBracedGroup(text, p1, inner, afterInner)) {
                text.replace(pos, afterInner - pos, inner);
                pos += inner.size();
            }
            else {
                pos += mLen;
            }
        }
    }

    pos = 0;
    while ((pos = text.find("\\sqrt", pos)) != std::string::npos) {
        size_t p1 = pos + 5;
        while (p1 < text.size() && (text[p1] == ' ' || text[p1] == '\t')) p1++;
        std::string inner;
        size_t afterInner = 0;
        if (ExtractBracedGroup(text, p1, inner, afterInner)) {
            std::string repl = IsSimpleToken(inner) ? ("\xE2\x88\x9A" + inner) : ("\xE2\x88\x9A(" + inner + ")");
            text.replace(pos, afterInner - pos, repl);
            pos += repl.size();
        }
        else {
            pos += 5;
        }
    }

    ReplaceAll(text, "\\qquad", "  ");
    ReplaceAll(text, "\\quad", " ");
    ReplaceAll(text, "\\left", "");
    ReplaceAll(text, "\\right", "");
    ReplaceAll(text, "\\cdot", "\xC2\xB7");
    ReplaceAll(text, "\\times", "\xC3\x97");
    ReplaceAll(text, "\\div", "\xC3\xB7");
    ReplaceAll(text, "\\pm", "\xC2\xB1");
    ReplaceAll(text, "\\leq", "\xE2\x89\xA4");
    ReplaceAll(text, "\\geq", "\xE2\x89\xA5");
    ReplaceAll(text, "\\neq", "\xE2\x89\xA0");
    ReplaceAll(text, "\\le", "\xE2\x89\xA4");
    ReplaceAll(text, "\\ge", "\xE2\x89\xA5");
    ReplaceAll(text, "\\ne", "\xE2\x89\xA0");
    ReplaceAll(text, "\\approx", "\xE2\x89\x88");
    ReplaceAll(text, "\\infty", "\xE2\x88\x9E");
    ReplaceAll(text, "\\Rightarrow", "=>");
    ReplaceAll(text, "\\rightarrow", "->");
    ReplaceAll(text, "\\to", "->");
    ReplaceAll(text, "\\,", " ");
    ReplaceAll(text, "\\;", " ");
    ReplaceAll(text, "\\:", " ");
    ReplaceAll(text, "$$", "");
    ReplaceAll(text, "$", "");
    ReplaceAll(text, "**", "");

    std::string normalized;
    normalized.reserve(text.size());
    for (char c : text) {
        if (c != '\r') normalized.push_back(c);
    }

    std::istringstream lineStream(normalized);
    std::string line;
    std::string finalOut;
    bool firstLine = true;
    while (std::getline(lineStream, line)) {
        size_t hPos = 0;
        while (hPos < line.size() && line[hPos] == '#') hPos++;
        if (hPos > 0 && hPos < line.size() && line[hPos] == ' ') {
            line = line.substr(hPos + 1);
        }
        if (line.rfind("```", 0) == 0) continue;
        if (!firstLine) finalOut += "\r\n";
        finalOut += line;
        firstLine = false;
    }

    return finalOut;
}

std::string ExtractGeminiText(const std::string& json) {
    size_t pos = json.find("\"text\"");
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + 6);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    pos++;

    std::string raw;
    for (size_t i = pos; i < json.length(); ++i) {
        if (json[i] == '\\' && i + 1 < json.length()) {
            char next = json[++i];
            switch (next) {
            case 'n':  raw += '\n'; break;
            case 'r':  raw += '\r'; break;
            case 't':  raw += '\t'; break;
            case 'b':  raw += '\b'; break;
            case 'f':  raw += '\f'; break;
            case '"':  raw += '"';  break;
            case '/':  raw += '/';  break;
            case '\\': raw += '\\'; break;
            case 'u': {
                unsigned int cp = 0;
                if (ParseHex4(json, i + 1, cp)) {
                    i += 4;
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 < json.length() && json[i + 1] == '\\' && json[i + 2] == 'u') {
                        unsigned int low = 0;
                        if (ParseHex4(json, i + 3, low) && low >= 0xDC00 && low <= 0xDFFF) {
                            cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                            i += 6;
                        }
                    }
                    AppendUtf8Codepoint(raw, cp);
                }
                else {
                    raw += "\\u";
                }
                break;
            }
            default:
                raw += '\\';
                raw += next;
                break;
            }
        }
        else if (json[i] == '"') {
            break;
        }
        else {
            raw += json[i];
        }
    }
    return CleanResponseText(raw);
}

void NotifyPopupUpdate() {
    if (g_hPopupWnd != NULL && IsWindow(g_hPopupWnd)) {
        PostMessageW(g_hPopupWnd, WM_UPDATE_RESPONSE, 0, 0);
    }
}

void ProcessScreen() {
    if (API_KEYS.empty()) {
        latest_answer = "Error: No API keys configured.\r\nPress Insert to add your Gemini API keys.";
        std::cout << "[-] No API keys configured. Open settings (Insert).\n";
        NotifyPopupUpdate();
        return;
    }

    if (is_loading.exchange(true)) {
        std::cout << "[!] Request already processing, please wait...\n";
        return;
    }

    NotifyPopupUpdate();

    try {
        std::vector<BYTE> imgBytes;
        if (!CaptureScreenJpeg(imgBytes)) {
            latest_answer = "Error taking screenshot.";
            is_loading = false;
            NotifyPopupUpdate();
            return;
        }

        std::string base64Img = Base64Encode(imgBytes.data(), (unsigned int)imgBytes.size());
        std::string prompt = GetActivePrompt();

        std::string jsonPayload = "{\"contents\":[{\"parts\":[{\"text\":\"" + EscapeJsonString(prompt) + "\"},{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"" + base64Img + "\"}}]}]}";

        std::vector<std::string> keysToTry;
        if (!last_working_key.empty()) {
            keysToTry.push_back(last_working_key);
            for (const auto& k : API_KEYS) if (k != last_working_key) keysToTry.push_back(k);
        }
        else {
            keysToTry = API_KEYS;
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(keysToTry.begin(), keysToTry.end(), g);
        }

        bool success = false;
        std::string currentModel = g_keys.selectedModel.empty() ? "gemini-3.5-flash-lite" : g_keys.selectedModel;

        for (size_t idx = 0; idx < keysToTry.size(); ++idx) {
            const auto& key = keysToTry[idx];
            std::cout << "[*] Trying key " << (idx + 1) << "/" << keysToTry.size() << " [" << MaskKey(key) << "]...\n";
            std::cout << "   -> Model: " << currentModel << "...\n";

            std::string response;
            int statusCode = 0;

            if (SendGeminiRequest(key, currentModel, jsonPayload, response, statusCode)) {
                if (statusCode == 200) {
                    std::string parsed = ExtractGeminiText(response);
                    if (!parsed.empty()) {
                        latest_answer = parsed;
                        last_working_key = key;
                        std::cout << "[+] Response successfully received from [" << currentModel << "]!\n";
                        success = true;
                        break;
                    }
                }
                else if (statusCode == 429) {
                    std::cout << "[!] Rate limit reached on " << currentModel << "\n";
                }
                else {
                    std::cout << "[-] HTTP Error: " << statusCode << "\n";
                }
            }
        }

        if (!success) {
            latest_answer = "Error: Request failed. Check your API key or model name.";
            std::cout << "[-] Request failed.\n";
        }
    }
    catch (...) {
        latest_answer = "Unexpected error during request.";
    }

    is_loading = false;
    NotifyPopupUpdate();
}

void CopyTextToClipboard(HWND hOwner, const std::wstring& text) {
    if (!OpenClipboard(hOwner)) return;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        void* pMem = GlobalLock(hMem);
        if (pMem) {
            std::memcpy(pMem, text.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }
    CloseClipboard();
}

void SaveConfig();

LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL;
    static HWND hBtnTheme = NULL;
    static HWND hBtnCopy = NULL;
    static HWND hBtnClose = NULL;
    static HFONT hFontText = NULL;
    static HFONT hFontBtn = NULL;

    switch (msg) {
    case WM_CREATE: {
        InitThemeBrushes();
        const LocStrings& L = LOC[g_keys.lang];
        std::wstring wText = is_loading.load()
            ? L.wLoading
            : (latest_answer.empty() ? L.wNoResponse : Utf8ToWide(latest_answer));

        hFontText = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hFontBtn = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wText.c_str(),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            15, 15, 630, 415, hWnd, (HMENU)IDC_POPUP_EDIT, NULL, NULL);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFontText, TRUE);
        SendMessageW(hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

        hBtnTheme = CreateWindowW(L"BUTTON", g_keys.darkTheme ? L.wThemeDarkBtn : L.wThemeLightBtn,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            15, 440, 140, 32, hWnd, (HMENU)IDC_POPUP_THEME, NULL, NULL);
        SendMessageW(hBtnTheme, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

        hBtnCopy = CreateWindowW(L"BUTTON", L.wCopyBtn,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            425, 440, 105, 32, hWnd, (HMENU)IDC_POPUP_COPY, NULL, NULL);
        SendMessageW(hBtnCopy, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

        hBtnClose = CreateWindowW(L"BUTTON", L.wCloseBtn,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            540, 440, 105, 32, hWnd, (HMENU)IDC_POPUP_CLOSE, NULL, NULL);
        SendMessageW(hBtnClose, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

        ApplyThemeToWindowAndChildren(hWnd, g_keys.darkTheme);
        break;
    }
    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        int margin = 15;
        int btnHeight = 32;
        int btnWidth = 105;
        int bottomArea = btnHeight + 20;

        if (hEdit) {
            MoveWindow(hEdit, margin, margin,
                (std::max)(100, width - margin * 2),
                (std::max)(80, height - margin - bottomArea), TRUE);
        }
        int btnY = height - btnHeight - 12;
        if (hBtnTheme) {
            MoveWindow(hBtnTheme, margin, btnY, 140, btnHeight, TRUE);
        }
        if (hBtnCopy && hBtnClose) {
            MoveWindow(hBtnClose, width - margin - btnWidth, btnY, btnWidth, btnHeight, TRUE);
            MoveWindow(hBtnCopy, width - margin - btnWidth * 2 - 10, btnY, btnWidth, btnHeight, TRUE);
        }
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_keys.darkTheme ? g_hBrDarkBg : g_hBrLightBg);
        return 1;
    }
    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT* dis = (const DRAWITEMSTRUCT*)lParam;
        if (dis && dis->CtlType == ODT_BUTTON) {
            DrawThemedButton(dis, g_keys.darkTheme);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        if (g_keys.darkTheme) {
            SetBkColor(hdc, RGB(36, 36, 42));
            SetTextColor(hdc, RGB(238, 238, 242));
            return (LRESULT)g_hBrDarkCtrl;
        }
        else {
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, RGB(25, 25, 25));
            return (LRESULT)g_hBrLightCtrl;
        }
    }
    case WM_APPLY_THEME: {
        const LocStrings& L = LOC[g_keys.lang];
        if (hBtnTheme) SetWindowTextW(hBtnTheme, g_keys.darkTheme ? L.wThemeDarkBtn : L.wThemeLightBtn);
        ApplyThemeToWindowAndChildren(hWnd, g_keys.darkTheme);
        break;
    }
    case WM_UPDATE_RESPONSE: {
        const LocStrings& L = LOC[g_keys.lang];
        std::wstring wText = is_loading.load()
            ? L.wLoading
            : (latest_answer.empty() ? L.wNoResponse : Utf8ToWide(latest_answer));
        if (hEdit) {
            SetWindowTextW(hEdit, wText.c_str());
            SendMessageW(hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        }
        if (hBtnCopy) {
            SetWindowTextW(hBtnCopy, L.wCopyBtn);
            InvalidateRect(hBtnCopy, NULL, TRUE);
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_POPUP_COPY) {
            if (!latest_answer.empty()) {
                CopyTextToClipboard(hWnd, Utf8ToWide(latest_answer));
                SetWindowTextW(hBtnCopy, LOC[g_keys.lang].wCopiedBtn);
                InvalidateRect(hBtnCopy, NULL, TRUE);
            }
        }
        else if (LOWORD(wParam) == IDC_POPUP_THEME) {
            g_keys.darkTheme = !g_keys.darkTheme;
            SaveConfig();
            SendMessageW(hWnd, WM_APPLY_THEME, 0, 0);
            if (g_hSettingsWnd && IsWindow(g_hSettingsWnd)) {
                PostMessageW(g_hSettingsWnd, WM_APPLY_THEME, 0, 0);
            }
        }
        else if (LOWORD(wParam) == IDC_POPUP_CLOSE || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        if (hFontText) { DeleteObject(hFontText); hFontText = NULL; }
        if (hFontBtn) { DeleteObject(hFontBtn); hFontBtn = NULL; }
        hEdit = NULL;
        hBtnTheme = NULL;
        hBtnCopy = NULL;
        hBtnClose = NULL;
        g_hPopupWnd = NULL;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowPopup() {
    if (g_hPopupWnd != NULL && IsWindow(g_hPopupWnd)) {
        PostMessageW(g_hPopupWnd, WM_CLOSE, 0, 0);
        return;
    }

    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = PopupWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"GeminiPopupClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        RegisterClassW(&wc);
        classRegistered = true;
    }

    g_hPopupWnd = CreateWindowExW(
        WS_EX_TOPMOST, L"GeminiPopupClass", LOC[g_keys.lang].wTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 520,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    SetForegroundWindow(g_hPopupWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            DestroyWindow(g_hPopupWnd);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

UINT ParseKeyStringToVK(std::wstring str) {
    str = Utf8ToWide(TrimString(WideToUtf8(str)));
    for (wchar_t& c : str) {
        if (c >= L'A' && c <= L'Z') {
            c = (wchar_t)(c - L'A' + L'a');
        }
    }

    if (str.size() >= 2 && str[0] == L'f') {
        int fn = _wtoi(str.c_str() + 1);
        if (fn >= 1 && fn <= 12) return VK_F1 + (fn - 1);
    }
    if (str == L"insert") return VK_INSERT;
    if (str == L"delete") return VK_DELETE;
    if (str == L"home") return VK_HOME;
    if (str == L"end") return VK_END;
    if (str.length() == 1) {
        wchar_t ch = str[0];
        if (ch >= L'a' && ch <= L'z') ch = (wchar_t)(ch - L'a' + L'A');
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9')) return (UINT)ch;
    }
    return 0;
}

std::wstring VKToKeyString(UINT vk) {
    if (vk >= VK_F1 && vk <= VK_F12) return L"f" + std::to_wstring(vk - VK_F1 + 1);
    if (vk == VK_INSERT) return L"insert";
    if (vk == VK_DELETE) return L"delete";
    if (vk == VK_HOME) return L"home";
    if (vk == VK_END) return L"end";
    if (vk >= 'A' && vk <= 'Z') {
        wchar_t ch = (wchar_t)(vk - 'A' + L'a');
        return std::wstring(1, ch);
    }
    if (vk >= '0' && vk <= '9') {
        wchar_t ch = (wchar_t)vk;
        return std::wstring(1, ch);
    }
    return L"unknown";
}

void SaveConfig() {
    std::wstring cfgPath = GetModuleDir() + L"config.ini";
    std::ofstream file(cfgPath.c_str());
    if (file.is_open()) {
        file << "[Settings]\n";
        file << "capture=" << WideToUtf8(VKToKeyString(g_keys.vkCapture)) << "\n";
        file << "show=" << WideToUtf8(VKToKeyString(g_keys.vkShow)) << "\n";
        file << "exit=" << WideToUtf8(VKToKeyString(g_keys.vkExit)) << "\n";
        file << "settings=" << WideToUtf8(VKToKeyString(g_keys.vkSettings)) << "\n";
        file << "console=" << WideToUtf8(VKToKeyString(g_keys.vkConsole)) << "\n";
        file << "lang=" << (int)g_keys.lang << "\n";
        file << "dark_theme=" << (g_keys.darkTheme ? 1 : 0) << "\n";
        file << "model=" << g_keys.selectedModel << "\n";
        file << "prompt=" << EscapeIniNewlines(g_keys.customPrompt) << "\n\n";

        file << "[EncryptedKeys]\n";
        for (const auto& key : API_KEYS) {
            file << EncryptApiKey(key) << "\n";
        }
        file.close();
    }
}

void LoadConfig() {
    std::wstring cfgPath = GetModuleDir() + L"config.ini";
    std::ifstream file(cfgPath.c_str());
    if (!file.is_open()) {
        file.open("config.ini");
    }

    if (file.is_open()) {
        API_KEYS.clear();
        std::string line;
        bool readingKeys = false;

        while (std::getline(file, line)) {
            line = TrimString(line);
            if (line.empty() || line[0] == '#') continue;

            if (line == "[Keys]" || line == "[EncryptedKeys]") {
                readingKeys = true;
                continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                readingKeys = false;
                continue;
            }

            if (!readingKeys) {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = TrimString(line.substr(0, eq));
                    std::string val = TrimString(line.substr(eq + 1));

                    if (key == "capture") {
                        UINT v = ParseKeyStringToVK(Utf8ToWide(val));
                        if (v) g_keys.vkCapture = v;
                    }
                    else if (key == "show") {
                        UINT v = ParseKeyStringToVK(Utf8ToWide(val));
                        if (v) g_keys.vkShow = v;
                    }
                    else if (key == "exit") {
                        UINT v = ParseKeyStringToVK(Utf8ToWide(val));
                        if (v) g_keys.vkExit = v;
                    }
                    else if (key == "settings") {
                        UINT v = ParseKeyStringToVK(Utf8ToWide(val));
                        if (v) g_keys.vkSettings = v;
                    }
                    else if (key == "console") {
                        UINT v = ParseKeyStringToVK(Utf8ToWide(val));
                        if (v) g_keys.vkConsole = v;
                    }
                    else if (key == "lang") {
                        int l = std::atoi(val.c_str());
                        if (l >= 0 && l <= 2) g_keys.lang = (AppLanguage)l;
                    }
                    else if (key == "dark_theme") {
                        g_keys.darkTheme = (std::atoi(val.c_str()) != 0);
                    }
                    else if (key == "model" && !val.empty()) {
                        g_keys.selectedModel = val;
                    }
                    else if (key == "prompt") {
                        g_keys.customPrompt = UnescapeIniNewlines(val);
                    }
                }
            }
            else {
                size_t eq = line.find('=');
                std::string rawEntry = (eq != std::string::npos && line.compare(0, 3, "v9$") != 0)
                    ? TrimString(line.substr(eq + 1))
                    : line;
                std::string pureKey = DecryptApiKeyOrRaw(rawEntry);
                if (!pureKey.empty()) {
                    API_KEYS.push_back(pureKey);
                }
            }
        }
        file.close();
    }
}

void ReapplyHotkeys() {
    UnregisterHotKey(NULL, HK_CAPTURE);
    UnregisterHotKey(NULL, HK_SHOW);
    UnregisterHotKey(NULL, HK_EXIT);
    UnregisterHotKey(NULL, HK_SETTINGS);
    UnregisterHotKey(NULL, HK_CONSOLE);

    RegisterHotKey(NULL, HK_CAPTURE, 0, g_keys.vkCapture);
    RegisterHotKey(NULL, HK_SHOW, 0, g_keys.vkShow);
    RegisterHotKey(NULL, HK_EXIT, 0, g_keys.vkExit);
    RegisterHotKey(NULL, HK_SETTINGS, 0, g_keys.vkSettings);
    RegisterHotKey(NULL, HK_CONSOLE, 0, g_keys.vkConsole);

    std::wcout << L"\n=== Hotkeys Updated ===\n";
    std::wcout << L"[" << VKToKeyString(g_keys.vkCapture) << L"] - Capture & Query\n";
    std::wcout << L"[" << VKToKeyString(g_keys.vkShow) << L"] - Toggle Response\n";
    std::wcout << L"[" << VKToKeyString(g_keys.vkConsole) << L"] - Toggle Debug Console\n";
    std::wcout << L"[" << VKToKeyString(g_keys.vkSettings) << L"] - Settings Menu\n";
    std::wcout << L"[" << VKToKeyString(g_keys.vkExit) << L"] - Exit\n";
    std::wcout << L"Active Model: " << Utf8ToWide(g_keys.selectedModel) << L"\n\n";
}

// --- Windows 10 & Windows 11 (Windows Terminal) Console Hiding Support ---
static const wchar_t* CONSOLE_UNIQUE_TITLE = L"GeminiAI_DebugConsole_Host";

struct FindTermWindowData {
    HWND foundHwnd = NULL;
};

static BOOL CALLBACK EnumWindowsFindTerminalProc(HWND hWnd, LPARAM lParam) {
    wchar_t cls[64] = { 0 };
    if (!GetClassNameW(hWnd, cls, 63)) return TRUE;

    if (wcscmp(cls, L"CASCADIA_HOSTING_WINDOW_CLASS") == 0 ||
        wcscmp(cls, L"ConsoleWindowClass") == 0) {
        wchar_t title[256] = { 0 };
        GetWindowTextW(hWnd, title, 255);
        if (wcsstr(title, CONSOLE_UNIQUE_TITLE) != NULL) {
            FindTermWindowData* data = (FindTermWindowData*)lParam;
            data->foundHwnd = hWnd;
            return FALSE;
        }
    }
    return TRUE;
}

HWND ResolveConsoleHostWindow() {
    if (g_hConsoleHostWnd && IsWindow(g_hConsoleHostWnd)) {
        return g_hConsoleHostWnd;
    }

    FindTermWindowData data;
    EnumWindows(EnumWindowsFindTerminalProc, (LPARAM)&data);
    if (data.foundHwnd) {
        g_hConsoleHostWnd = data.foundHwnd;
        return g_hConsoleHostWnd;
    }

    HWND hCon = GetConsoleWindow();
    if (hCon) {
        wchar_t cls[64] = { 0 };
        GetClassNameW(hCon, cls, 63);
        if (wcscmp(cls, L"ConsoleWindowClass") == 0) {
            g_hConsoleHostWnd = hCon;
            return g_hConsoleHostWnd;
        }
        HWND hRoot = GetAncestor(hCon, GA_ROOTOWNER);
        if (hRoot && hRoot != hCon) {
            g_hConsoleHostWnd = hRoot;
            return g_hConsoleHostWnd;
        }
    }
    return hCon;
}

void HideConsoleReliableAtStartup() {
    SetConsoleTitleW(CONSOLE_UNIQUE_TITLE);

    // Check if foreground window right at startup is Windows Terminal
    HWND hFg = GetForegroundWindow();
    if (hFg) {
        wchar_t cls[64] = { 0 };
        GetClassNameW(hFg, cls, 63);
        if (wcscmp(cls, L"CASCADIA_HOSTING_WINDOW_CLASS") == 0) {
            g_hConsoleHostWnd = hFg;
            ShowWindow(hFg, SW_HIDE);
        }
    }

    HWND hCon = GetConsoleWindow();
    if (hCon) ShowWindow(hCon, SW_HIDE);

    // On Windows 11, WindowsTerminal.exe creates and shows its XAML host window
    // asynchronously (~30-350ms after main() starts). Poll briefly to catch & hide it.
    std::thread([]() {
        for (int i = 0; i < 35; ++i) {
            if (g_consoleVisible.load()) break;
            HWND hHost = ResolveConsoleHostWindow();
            if (hHost && IsWindowVisible(hHost)) {
                ShowWindow(hHost, SW_HIDE);
            }
            HWND hPseudo = GetConsoleWindow();
            if (hPseudo && IsWindowVisible(hPseudo)) {
                ShowWindow(hPseudo, SW_HIDE);
            }
            Sleep(15);
        }
    }).detach();
}

void ToggleConsole() {
    HWND hHost = ResolveConsoleHostWindow();
    HWND hCon = GetConsoleWindow();

    if (g_consoleVisible.load()) {
        g_consoleVisible = false;
        if (hHost) ShowWindow(hHost, SW_HIDE);
        if (hCon && hCon != hHost) ShowWindow(hCon, SW_HIDE);
    }
    else {
        g_consoleVisible = true;
        if (hCon && hCon != hHost) ShowWindow(hCon, SW_SHOW);
        if (hHost) {
            ShowWindow(hHost, SW_RESTORE);
            ShowWindow(hHost, SW_SHOW);
            SetForegroundWindow(hHost);
        }
    }
}

static bool IsDefaultPromptText(const std::string& p) {
    std::string t = TrimString(p);
    if (t.empty()) return true;
    for (int i = 0; i < 3; ++i) {
        if (t == LOC[i].aiPrompt) return true;
    }
    return false;
}

LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdCapture, hEdShow, hEdExit, hEdSettings, hEdConsole;
    static HWND hComboLang, hComboModel, hEdPrompt, hEdApiKeys;
    static HWND hBtnTheme, hBtnResetPrompt, hBtnSave, hBtnCancel;
    static HFONT hFont = NULL;

    switch (msg) {
    case WM_CREATE: {
        InitThemeBrushes();
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        const LocStrings& L = LOC[g_keys.lang];

        auto CreateLabelAndEdit = [&](const wchar_t* label, int y, HWND& hEdit, UINT vk) {
            HWND hLbl = CreateWindowW(L"STATIC", label, WS_CHILD | WS_VISIBLE, 20, y, 175, 22, hWnd, NULL, NULL, NULL);
            SendMessageW(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);
            hEdit = CreateWindowW(L"EDIT", VKToKeyString(vk).c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 200, y, 180, 22, hWnd, NULL, NULL, NULL);
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        };

        CreateLabelAndEdit(L.wCapLabel, 20, hEdCapture, g_keys.vkCapture);
        CreateLabelAndEdit(L.wShowLabel, 50, hEdShow, g_keys.vkShow);
        CreateLabelAndEdit(L.wConsoleLabel, 80, hEdConsole, g_keys.vkConsole);
        CreateLabelAndEdit(L.wSettingsLabel, 110, hEdSettings, g_keys.vkSettings);
        CreateLabelAndEdit(L.wExitLabel, 140, hEdExit, g_keys.vkExit);

        HWND hLangLbl = CreateWindowW(L"STATIC", L.wLangLabel, WS_CHILD | WS_VISIBLE, 20, 170, 175, 22, hWnd, NULL, NULL, NULL);
        SendMessageW(hLangLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        hComboLang = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 200, 170, 180, 120, hWnd, (HMENU)IDC_SET_LANG_COMBO, NULL, NULL);
        SendMessageW(hComboLang, WM_SETFONT, (WPARAM)hFont, TRUE);
        // Explicit Unicode escapes for "English", "Українська", "Русский"
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"\u0423\u043A\u0440\u0430\u0457\u043D\u0441\u044C\u043A\u0430");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"\u0420\u0443\u0441\u0441\u043A\u0438\u0439");
        SendMessageW(hComboLang, CB_SETCURSEL, (WPARAM)g_keys.lang, 0);

        HWND hModelLbl = CreateWindowW(L"STATIC", L.wModelLabel, WS_CHILD | WS_VISIBLE, 20, 202, 175, 22, hWnd, NULL, NULL, NULL);
        SendMessageW(hModelLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        hComboModel = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL, 200, 202, 180, 140, hWnd, NULL, NULL, NULL);
        SendMessageW(hComboModel, WM_SETFONT, (WPARAM)hFont, TRUE);
        for (const auto& m : PRESET_MODELS) {
            SendMessageW(hComboModel, CB_ADDSTRING, 0, (LPARAM)m.c_str());
        }
        SetWindowTextW(hComboModel, Utf8ToWide(g_keys.selectedModel).c_str());

        // AI Prompt Label + Reset Default Button + Multiline Edit
        HWND hPromptLbl = CreateWindowW(L"STATIC", L.wPromptLabel, WS_CHILD | WS_VISIBLE, 20, 238, 260, 22, hWnd, NULL, NULL, NULL);
        SendMessageW(hPromptLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        hBtnResetPrompt = CreateWindowW(L"BUTTON", L.wResetPromptBtn, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 285, 234, 95, 24, hWnd, (HMENU)IDC_SET_RESET_PRM, NULL, NULL);
        SendMessageW(hBtnResetPrompt, WM_SETFONT, (WPARAM)hFont, TRUE);

        std::string currentPromptText = GetActivePrompt();
        hEdPrompt = CreateWindowExW(0, L"EDIT", Utf8ToWide(currentPromptText).c_str(),
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            20, 262, 360, 85, hWnd, NULL, NULL, NULL);
        SendMessageW(hEdPrompt, WM_SETFONT, (WPARAM)hFont, TRUE);

        // API Keys Label + Edit
        HWND hKeysLbl = CreateWindowW(L"STATIC", L.wKeysLabel, WS_CHILD | WS_VISIBLE, 20, 355, 360, 20, hWnd, NULL, NULL, NULL);
        SendMessageW(hKeysLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        std::string keysDisplay;
        for (const auto& k : API_KEYS) {
            keysDisplay += EncryptApiKey(k) + "\r\n";
        }

        hEdApiKeys = CreateWindowExW(0, L"EDIT", Utf8ToWide(keysDisplay).c_str(),
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            20, 378, 360, 90, hWnd, NULL, NULL, NULL);
        SendMessageW(hEdApiKeys, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Theme Toggle + Save + Cancel Buttons
        hBtnTheme = CreateWindowW(L"BUTTON", g_keys.darkTheme ? L.wThemeDarkBtn : L.wThemeLightBtn,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 20, 482, 125, 32, hWnd, (HMENU)IDC_SET_THEME, NULL, NULL);
        hBtnSave = CreateWindowW(L"BUTTON", L.wSaveBtn,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 160, 482, 105, 32, hWnd, (HMENU)IDC_SET_SAVE, NULL, NULL);
        hBtnCancel = CreateWindowW(L"BUTTON", L.wCancelBtn,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 275, 482, 105, 32, hWnd, (HMENU)IDC_SET_CANCEL, NULL, NULL);

        SendMessageW(hBtnTheme, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hBtnSave, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

        ApplyThemeToWindowAndChildren(hWnd, g_keys.darkTheme);
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_keys.darkTheme ? g_hBrDarkBg : g_hBrLightBg);
        return 1;
    }
    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT* dis = (const DRAWITEMSTRUCT*)lParam;
        if (dis && dis->CtlType == ODT_BUTTON) {
            DrawThemedButton(dis, g_keys.darkTheme);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        if (g_keys.darkTheme) {
            SetBkColor(hdc, RGB(24, 24, 28));
            SetTextColor(hdc, RGB(235, 235, 240));
            return (LRESULT)g_hBrDarkBg;
        }
        else {
            SetBkColor(hdc, RGB(240, 240, 240));
            SetTextColor(hdc, RGB(20, 20, 24));
            return (LRESULT)g_hBrLightBg;
        }
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        if (g_keys.darkTheme) {
            SetBkColor(hdc, RGB(36, 36, 42));
            SetTextColor(hdc, RGB(240, 240, 245));
            return (LRESULT)g_hBrDarkCtrl;
        }
        else {
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, RGB(20, 20, 24));
            return (LRESULT)g_hBrLightCtrl;
        }
    }
    case WM_APPLY_THEME: {
        const LocStrings& L = LOC[g_keys.lang];
        if (hBtnTheme) SetWindowTextW(hBtnTheme, g_keys.darkTheme ? L.wThemeDarkBtn : L.wThemeLightBtn);
        ApplyThemeToWindowAndChildren(hWnd, g_keys.darkTheme);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_SET_LANG_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            int selLang = (int)SendMessageW(hComboLang, CB_GETCURSEL, 0, 0);
            if (selLang >= 0 && selLang <= 2) {
                int pLen = GetWindowTextLengthW(hEdPrompt);
                std::vector<wchar_t> pBuf(pLen + 1);
                GetWindowTextW(hEdPrompt, pBuf.data(), pLen + 1);
                std::string curP = WideToUtf8(pBuf.data());
                if (IsDefaultPromptText(curP)) {
                    SetWindowTextW(hEdPrompt, Utf8ToWide(LOC[selLang].aiPrompt).c_str());
                }
            }
        }
        else if (LOWORD(wParam) == IDC_SET_RESET_PRM) {
            int selLang = (int)SendMessageW(hComboLang, CB_GETCURSEL, 0, 0);
            if (selLang < 0 || selLang > 2) selLang = (int)g_keys.lang;
            SetWindowTextW(hEdPrompt, Utf8ToWide(LOC[selLang].aiPrompt).c_str());
        }
        else if (LOWORD(wParam) == IDC_SET_THEME) {
            g_keys.darkTheme = !g_keys.darkTheme;
            SaveConfig();
            SendMessageW(hWnd, WM_APPLY_THEME, 0, 0);
            if (g_hPopupWnd && IsWindow(g_hPopupWnd)) {
                PostMessageW(g_hPopupWnd, WM_APPLY_THEME, 0, 0);
            }
        }
        else if (LOWORD(wParam) == IDC_SET_SAVE) {
            wchar_t b1[32], b2[32], b3[32], b4[32], b5[32], bModel[64];
            GetWindowTextW(hEdCapture, b1, 32);
            GetWindowTextW(hEdShow, b2, 32);
            GetWindowTextW(hEdConsole, b3, 32);
            GetWindowTextW(hEdSettings, b4, 32);
            GetWindowTextW(hEdExit, b5, 32);
            GetWindowTextW(hComboModel, bModel, 64);

            UINT k1 = ParseKeyStringToVK(b1);
            UINT k2 = ParseKeyStringToVK(b2);
            UINT k3 = ParseKeyStringToVK(b3);
            UINT k4 = ParseKeyStringToVK(b4);
            UINT k5 = ParseKeyStringToVK(b5);
            int selLang = (int)SendMessageW(hComboLang, CB_GETCURSEL, 0, 0);
            if (selLang < 0 || selLang > 2) selLang = 0;

            std::string chosenModel = TrimString(WideToUtf8(bModel));
            if (chosenModel.empty()) chosenModel = "gemini-3.5-flash-lite";

            int promptLen = GetWindowTextLengthW(hEdPrompt);
            std::vector<wchar_t> promptBuf(promptLen + 1);
            GetWindowTextW(hEdPrompt, promptBuf.data(), promptLen + 1);
            std::string editedPrompt = TrimString(WideToUtf8(promptBuf.data()));

            int textLen = GetWindowTextLengthW(hEdApiKeys);
            std::vector<wchar_t> keyBuffer(textLen + 1);
            GetWindowTextW(hEdApiKeys, keyBuffer.data(), textLen + 1);
            std::string rawKeys = WideToUtf8(keyBuffer.data());

            std::vector<std::string> newKeys;
            std::istringstream stream(rawKeys);
            std::string kLine;
            while (std::getline(stream, kLine)) {
                kLine = TrimString(kLine);
                if (kLine.empty()) continue;
                std::string dec = DecryptApiKeyOrRaw(kLine);
                if (!dec.empty()) newKeys.push_back(dec);
            }

            if (k1 && k2 && k3 && k4 && k5) {
                API_KEYS = newKeys;
                g_keys.vkCapture = k1;
                g_keys.vkShow = k2;
                g_keys.vkConsole = k3;
                g_keys.vkSettings = k4;
                g_keys.vkExit = k5;
                g_keys.lang = (AppLanguage)selLang;
                g_keys.selectedModel = chosenModel;
                g_keys.customPrompt = (editedPrompt == LOC[selLang].aiPrompt) ? "" : editedPrompt;

                SaveConfig();
                ReapplyHotkeys();
                if (g_hPopupWnd && IsWindow(g_hPopupWnd)) {
                    SetWindowTextW(g_hPopupWnd, LOC[g_keys.lang].wTitle);
                    PostMessageW(g_hPopupWnd, WM_UPDATE_RESPONSE, 0, 0);
                    PostMessageW(g_hPopupWnd, WM_APPLY_THEME, 0, 0);
                }
                MessageBoxW(hWnd, LOC[g_keys.lang].wSuccessSave, L"OK", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hWnd);
            }
            else {
                MessageBoxW(hWnd, LOC[g_keys.lang].wErrKey, L"Error", MB_OK | MB_ICONERROR);
            }
        }
        else if (LOWORD(wParam) == IDC_SET_CANCEL || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        if (hFont) { DeleteObject(hFont); hFont = NULL; }
        g_hSettingsWnd = NULL;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void OpenSettingsGui() {
    // Toggle behavior: pressing the Settings key again closes the Settings window
    if (g_hSettingsWnd != NULL && IsWindow(g_hSettingsWnd)) {
        PostMessageW(g_hSettingsWnd, WM_CLOSE, 0, 0);
        return;
    }

    static bool settingsClassRegistered = false;
    if (!settingsClassRegistered) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"GeminiSettingsClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        RegisterClassW(&wc);
        settingsClassRegistered = true;
    }

    g_hSettingsWnd = CreateWindowExW(
        WS_EX_TOPMOST, L"GeminiSettingsClass", LOC[g_keys.lang].wSettingsTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 415, 565,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    SetForegroundWindow(g_hSettingsWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            DestroyWindow(g_hSettingsWnd);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

int main() {
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Local\\p_liy_SingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HideConsoleReliableAtStartup();

    LoadConfig();

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    ReapplyHotkeys();

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
            case HK_CAPTURE:
                std::thread(ProcessScreen).detach();
                break;
            case HK_SHOW:
                std::thread(ShowPopup).detach();
                break;
            case HK_CONSOLE:
                ToggleConsole();
                break;
            case HK_SETTINGS:
                std::thread(OpenSettingsGui).detach();
                break;
            case HK_EXIT:
                if (hMutex) {
                    ReleaseMutex(hMutex);
                    CloseHandle(hMutex);
                }
                Gdiplus::GdiplusShutdown(gdiplusToken);
                ExitProcess(0);
            }
        }
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}