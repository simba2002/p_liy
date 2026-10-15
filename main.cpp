#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <wininet.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cwctype>
#include <cctype>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")

#define WM_UPDATE_RESPONSE (WM_USER + 101)
#define IDC_POPUP_EDIT     2001
#define IDC_POPUP_COPY     2002
#define IDC_POPUP_CLOSE    2003

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
    std::string selectedModel = "gemini-3.5-flash-lite";
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
    const wchar_t* wKeysLabel;
    const wchar_t* wSaveBtn;
    const wchar_t* wCancelBtn;
    const wchar_t* wSuccessSave;
    const wchar_t* wErrKey;
    const char* aiPrompt;
};

const LocStrings LOC[] = {
    // EN
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
        L"Gemini API Keys (One per line):",
        L"Save",
        L"Cancel",
        L"Settings successfully saved to config.ini!",
        L"Invalid key name (e.g. f1-f12, insert, delete, home, end, a-z)",
        "Analyze this screenshot carefully. If there is a question, test, or task, provide a concise, direct, and correct answer in English. If it contains code or an error, explain the fix concisely in English. IMPORTANT: Format your response as clean plain text only. Do NOT use Markdown (no **, *, #) and do NOT use LaTeX math syntax (no $, $$, \\frac, \\text, \\quad). Use standard plain-text math notation and Unicode symbols (e.g., x^2, /, *, <, >, <=, >=, !=)."
    },
    // UK
    {
        L"Відповідь ШІ",
        L"Відповіді ще немає. Натисніть клавішу аналізу екрана.",
        L"Аналізую скріншот, зачекайте...",
        L"Копіювати",
        L"Скопійовано!",
        L"Закрити",
        L"Налаштування та Ключі",
        L"Зробити скріншот:",
        L"Показати вікно:",
        L"Консоль / Дебаг:",
        L"Меню біндів:",
        L"Вихід з коду:",
        L"Мова інтерфейсу:",
        L"Модель Gemini:",
        L"API-ключі Gemini (по одному на рядок):",
        L"Зберегти",
        L"Скасувати",
        L"Налаштування успішно збережено у config.ini!",
        L"Некоректна назва клавіші (f1-f12, insert, delete, home, end, a-z)",
        "Analyze this screenshot carefully. If there is a question, test, or task, provide a concise, direct, and correct answer in Ukrainian. If it contains code or an error, explain the fix concisely in Ukrainian. IMPORTANT: Format your response as clean plain text only. Do NOT use Markdown (no **, *, #) and do NOT use LaTeX math syntax (no $, $$, \\frac, \\text, \\quad). Use standard plain-text math notation and Unicode symbols (e.g., x^2, /, *, <, >, <=, >=, !=)."
    },
    // RU
    {
        L"Ответ ИИ",
        L"Ответа пока нет. Нажмите клавишу захвата экрана.",
        L"Анализирую скриншот, подождите...",
        L"Копировать",
        L"Скопировано!",
        L"Закрыть",
        L"Настройки и Ключи",
        L"Сделать скриншот:",
        L"Показать окно:",
        L"Консоль / Дебаг:",
        L"Меню биндов:",
        L"Выход из кода:",
        L"Язык интерфейса:",
        L"Модель Gemini:",
        L"API-ключи Gemini (по одному на строку):",
        L"Сохранить",
        L"Отмена",
        L"Настройки успешно сохранены в config.ini!",
        L"Неверная клавиша (f1-f12, insert, delete, home, end, a-z)",
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

std::string MaskKey(const std::string& key) {
    if (key.length() <= 12) return "********";
    return key.substr(0, 8) + "...****";
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

// Finds balanced {...} starting at openBracePos (where str[openBracePos] == '{')
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

// Cleans Markdown, LaTeX, and any remaining \uXXXX sequences so responses display cleanly in a Win32 EDIT control
std::string CleanResponseText(std::string text) {
    // 0. Decode any remaining literal \uXXXX sequences (e.g. \u003e -> >, \u003c -> <)
    std::string unescaped;
    unescaped.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 5 < text.size() && text[i + 1] == 'u') {
            unsigned int cp = 0;
            if (ParseHex4(text, i + 2, cp)) {
                AppendUtf8Codepoint(unescaped, cp);
                i += 5;
                continue;
            }
        }
        unescaped.push_back(text[i]);
    }
    text.swap(unescaped);

    // 1. Handle \frac{a}{b} -> a/b or (a)/(b)
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

    // 2. Handle \text{...}, \mathrm{...}, \mathbf{...}, \textit{...} -> inner text
    const char* textMacros[] = { "\\text", "\\mathrm", "\\mathbf", "\\textit", "\\textbf" };
    for (const char* macro : textMacros) {
        size_t mLen = strlen(macro);
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

    // 3. Handle \sqrt{...} -> √(...)
    pos = 0;
    while ((pos = text.find("\\sqrt", pos)) != std::string::npos) {
        size_t p1 = pos + 5;
        while (p1 < text.size() && (text[p1] == ' ' || text[p1] == '\t')) p1++;
        std::string inner;
        size_t afterInner = 0;
        if (ExtractBracedGroup(text, p1, inner, afterInner)) {
            std::string repl = IsSimpleToken(inner) ? ("√" + inner) : ("√(" + inner + ")");
            text.replace(pos, afterInner - pos, repl);
            pos += repl.size();
        }
        else {
            pos += 5;
        }
    }

    // 4. Replace common LaTeX symbols & spacing commands
    ReplaceAll(text, "\\qquad", "  ");
    ReplaceAll(text, "\\quad", " ");
    ReplaceAll(text, "\\left", "");
    ReplaceAll(text, "\\right", "");
    ReplaceAll(text, "\\cdot", "·");
    ReplaceAll(text, "\\times", "×");
    ReplaceAll(text, "\\div", "÷");
    ReplaceAll(text, "\\pm", "±");
    ReplaceAll(text, "\\leq", "≤");
    ReplaceAll(text, "\\geq", "≥");
    ReplaceAll(text, "\\neq", "≠");
    ReplaceAll(text, "\\le", "≤");
    ReplaceAll(text, "\\ge", "≥");
    ReplaceAll(text, "\\ne", "≠");
    ReplaceAll(text, "\\approx", "≈");
    ReplaceAll(text, "\\infty", "∞");
    ReplaceAll(text, "\\Rightarrow", "=>");
    ReplaceAll(text, "\\rightarrow", "->");
    ReplaceAll(text, "\\to", "->");
    ReplaceAll(text, "\\,", " ");
    ReplaceAll(text, "\\;", " ");
    ReplaceAll(text, "\\:", " ");

    // 5. Remove LaTeX math delimiters ($$ and $) and Markdown bold (**)
    ReplaceAll(text, "$$", "");
    ReplaceAll(text, "$", "");
    ReplaceAll(text, "**", "");

    // 6. Clean Markdown headings at start of lines (e.g. ### Title -> Title) and normalize line endings to \r\n
    std::string normalized;
    normalized.reserve(text.size() + 64);
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') continue;
        normalized.push_back(text[i]);
    }

    std::istringstream lineStream(normalized);
    std::string line;
    std::string finalOut;
    bool firstLine = true;
    while (std::getline(lineStream, line)) {
        // Strip leading ### or ## if followed by space
        size_t hPos = 0;
        while (hPos < line.size() && line[hPos] == '#') hPos++;
        if (hPos > 0 && hPos < line.size() && line[hPos] == ' ') {
            line = line.substr(hPos + 1);
        }
        // Skip ``` code fence lines
        if (line.rfind("```", 0) == 0) {
            continue;
        }
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
    pos++; // Move past opening quote

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
                    // Handle UTF-16 surrogate pairs (\uD800..\uDBFF followed by \uDC00..\uDFFF)
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
        std::string prompt = LOC[g_keys.lang].aiPrompt;

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
            std::string keyPreview = MaskKey(key);
            std::cout << "[*] Trying key " << (idx + 1) << "/" << keysToTry.size() << " [" << keyPreview << "]...\n";
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
            if (success) break;
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
            memcpy(pMem, text.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }
    CloseClipboard();
}

LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL;
    static HWND hBtnCopy = NULL;
    static HWND hBtnClose = NULL;
    static HFONT hFontText = NULL;
    static HFONT hFontBtn = NULL;

    switch (msg) {
    case WM_CREATE: {
        const LocStrings& L = LOC[g_keys.lang];
        std::wstring wText;
        if (is_loading.load()) {
            wText = L.wLoading;
        }
        else {
            wText = latest_answer.empty() ? L.wNoResponse : Utf8ToWide(latest_answer);
        }

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
        // Remove automatic full-text selection
        SendMessageW(hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

        hBtnCopy = CreateWindowW(L"BUTTON", L.wCopyBtn,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            425, 440, 105, 32, hWnd, (HMENU)IDC_POPUP_COPY, NULL, NULL);
        SendMessageW(hBtnCopy, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

        hBtnClose = CreateWindowW(L"BUTTON", L.wCloseBtn,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            540, 440, 105, 32, hWnd, (HMENU)IDC_POPUP_CLOSE, NULL, NULL);
        SendMessageW(hBtnClose, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
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
        if (hBtnCopy && hBtnClose) {
            int btnY = height - btnHeight - 12;
            MoveWindow(hBtnClose, width - margin - btnWidth, btnY, btnWidth, btnHeight, TRUE);
            MoveWindow(hBtnCopy, width - margin - btnWidth * 2 - 10, btnY, btnWidth, btnHeight, TRUE);
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(25, 25, 25));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
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
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_POPUP_COPY) {
            if (!latest_answer.empty()) {
                CopyTextToClipboard(hWnd, Utf8ToWide(latest_answer));
                SetWindowTextW(hBtnCopy, LOC[g_keys.lang].wCopiedBtn);
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
    if (g_hPopupWnd != NULL) {
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
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
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

UINT ParseKeyStringToVK(const std::wstring& str) {
    if (str == L"f1") return VK_F1;
    if (str == L"f2") return VK_F2;
    if (str == L"f3") return VK_F3;
    if (str == L"f4") return VK_F4;
    if (str == L"f5") return VK_F5;
    if (str == L"f6") return VK_F6;
    if (str == L"f7") return VK_F7;
    if (str == L"f8") return VK_F8;
    if (str == L"f9") return VK_F9;
    if (str == L"f10") return VK_F10;
    if (str == L"f11") return VK_F11;
    if (str == L"f12") return VK_F12;
    if (str == L"insert") return VK_INSERT;
    if (str == L"delete") return L"delete" == str ? VK_DELETE : 0;
    if (str == L"home") return VK_HOME;
    if (str == L"end") return VK_END;
    if (str.length() == 1) {
        wchar_t ch = towupper(str[0]);
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
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        return std::wstring(1, (wchar_t)towlower(vk));
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
        file << "model=" << g_keys.selectedModel << "\n\n";

        // Зберігаємо ключі відкрито
        file << "[Keys]\n";
        for (const auto& key : API_KEYS) {
            file << key << "\n";
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
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            size_t last = line.find_last_not_of(" \t\r\n");
            if (last != std::string::npos) line.erase(last + 1);
            if (line.empty() || line[0] == '#') continue;

            // Підтримуємо і [Keys], і старий розділ [EncryptedKeys] без падінь
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
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);

                    if (key == "capture") g_keys.vkCapture = ParseKeyStringToVK(Utf8ToWide(val));
                    else if (key == "show") g_keys.vkShow = ParseKeyStringToVK(Utf8ToWide(val));
                    else if (key == "exit") g_keys.vkExit = ParseKeyStringToVK(Utf8ToWide(val));
                    else if (key == "settings") g_keys.vkSettings = ParseKeyStringToVK(Utf8ToWide(val));
                    else if (key == "console") g_keys.vkConsole = ParseKeyStringToVK(Utf8ToWide(val));
                    else if (key == "lang") {
                        int l = std::atoi(val.c_str());
                        if (l >= 0 && l <= 2) g_keys.lang = (AppLanguage)l;
                    }
                    else if (key == "model") {
                        if (!val.empty()) g_keys.selectedModel = val;
                    }
                }
            }
            else {
                // Якщо є формат key1=AIzaSy... або просто AIzaSy...
                size_t eq = line.find('=');
                std::string pureKey = (eq != std::string::npos) ? line.substr(eq + 1) : line;
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

void ToggleConsole() {
    HWND hConsole = GetConsoleWindow();
    if (!hConsole) return;
    if (IsWindowVisible(hConsole)) {
        ShowWindow(hConsole, SW_HIDE);
    }
    else {
        ShowWindow(hConsole, SW_SHOW);
        SetForegroundWindow(hConsole);
    }
}

LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdCapture, hEdShow, hEdExit, hEdSettings, hEdConsole, hComboLang, hComboModel, hEdApiKeys;
    static HFONT hFont = NULL;
    switch (msg) {
    case WM_CREATE: {
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        const LocStrings& L = LOC[g_keys.lang];

        auto CreateLabelAndEdit = [&](const wchar_t* label, int y, HWND& hEdit, UINT vk) {
            HWND hLbl = CreateWindowW(L"STATIC", label, WS_CHILD | WS_VISIBLE, 20, y, 160, 22, hWnd, NULL, NULL, NULL);
            SendMessageW(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);
            hEdit = CreateWindowW(L"EDIT", VKToKeyString(vk).c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 190, y, 150, 22, hWnd, NULL, NULL, NULL);
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        };

        CreateLabelAndEdit(L.wCapLabel, 20, hEdCapture, g_keys.vkCapture);
        CreateLabelAndEdit(L.wShowLabel, 50, hEdShow, g_keys.vkShow);
        CreateLabelAndEdit(L.wConsoleLabel, 80, hEdConsole, g_keys.vkConsole);
        CreateLabelAndEdit(L.wSettingsLabel, 110, hEdSettings, g_keys.vkSettings);
        CreateLabelAndEdit(L.wExitLabel, 140, hEdExit, g_keys.vkExit);

        // Мова інтерфейсу
        HWND hLangLbl = CreateWindowW(L"STATIC", L.wLangLabel, WS_CHILD | WS_VISIBLE, 20, 170, 160, 22, hWnd, NULL, NULL, NULL);
        SendMessageW(hLangLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        hComboLang = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 190, 170, 150, 120, hWnd, NULL, NULL, NULL);
        SendMessageW(hComboLang, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Українська");
        SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Русский");
        SendMessageW(hComboLang, CB_SETCURSEL, (WPARAM)g_keys.lang, 0);

        // Вибір / ручне введення моделі
        HWND hModelLbl = CreateWindowW(L"STATIC", L.wModelLabel, WS_CHILD | WS_VISIBLE, 20, 200, 160, 22, hWnd, NULL, NULL, NULL);
        SendMessageW(hModelLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        hComboModel = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL, 190, 200, 150, 140, hWnd, NULL, NULL, NULL);
        SendMessageW(hComboModel, WM_SETFONT, (WPARAM)hFont, TRUE);
        for (const auto& m : PRESET_MODELS) {
            SendMessageW(hComboModel, CB_ADDSTRING, 0, (LPARAM)m.c_str());
        }
        SetWindowTextW(hComboModel, Utf8ToWide(g_keys.selectedModel).c_str());

        // Блок ключів API (показуємо відкриті ключі для можливості редагування)
        HWND hKeysLbl = CreateWindowW(L"STATIC", L.wKeysLabel, WS_CHILD | WS_VISIBLE, 20, 235, 320, 20, hWnd, NULL, NULL, NULL);
        SendMessageW(hKeysLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

        std::string keysDisplay = "";
        for (const auto& k : API_KEYS) {
            keysDisplay += k + "\r\n";
        }

        hEdApiKeys = CreateWindowExW(0, L"EDIT", Utf8ToWide(keysDisplay).c_str(),
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            20, 260, 320, 100, hWnd, NULL, NULL, NULL);
        SendMessageW(hEdApiKeys, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hBtnSave = CreateWindowW(L"BUTTON", L.wSaveBtn, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 65, 380, 100, 32, hWnd, (HMENU)1001, NULL, NULL);
        HWND hBtnCancel = CreateWindowW(L"BUTTON", L.wCancelBtn, WS_CHILD | WS_VISIBLE, 195, 380, 100, 32, hWnd, (HMENU)1002, NULL, NULL);
        SendMessageW(hBtnSave, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1001) {
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

            std::string chosenModel = WideToUtf8(bModel);
            chosenModel.erase(0, chosenModel.find_first_not_of(" \t\r\n"));
            size_t lModel = chosenModel.find_last_not_of(" \t\r\n");
            if (lModel != std::string::npos) chosenModel.erase(lModel + 1);
            if (chosenModel.empty()) chosenModel = "gemini-3.5-flash-lite";

            int textLen = GetWindowTextLengthW(hEdApiKeys);
            std::vector<wchar_t> keyBuffer(textLen + 1);
            GetWindowTextW(hEdApiKeys, keyBuffer.data(), textLen + 1);
            std::string rawKeys = WideToUtf8(keyBuffer.data());

            std::vector<std::string> newKeys;
            std::istringstream stream(rawKeys);
            std::string kLine;
            while (std::getline(stream, kLine)) {
                kLine.erase(0, kLine.find_first_not_of(" \t\r\n"));
                size_t last = kLine.find_last_not_of(" \t\r\n");
                if (last != std::string::npos) kLine.erase(last + 1);
                if (kLine.empty()) continue;

                newKeys.push_back(kLine);
            }

            API_KEYS = newKeys;

            if (k1 && k2 && k3 && k4 && k5) {
                g_keys.vkCapture = k1;
                g_keys.vkShow = k2;
                g_keys.vkConsole = k3;
                g_keys.vkSettings = k4;
                g_keys.vkExit = k5;
                g_keys.lang = (AppLanguage)selLang;
                g_keys.selectedModel = chosenModel;
                SaveConfig();
                ReapplyHotkeys();
                MessageBoxW(hWnd, LOC[g_keys.lang].wSuccessSave, L"OK", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hWnd);
            }
            else {
                MessageBoxW(hWnd, LOC[g_keys.lang].wErrKey, L"Error", MB_OK | MB_ICONERROR);
            }
        }
        else if (LOWORD(wParam) == 1002) {
            DestroyWindow(hWnd);
        }
        break;
    }
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
    if (g_hSettingsWnd != NULL) {
        SetForegroundWindow(g_hSettingsWnd);
        return;
    }

    static bool settingsClassRegistered = false;
    if (!settingsClassRegistered) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"GeminiSettingsClass";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassW(&wc);
        settingsClassRegistered = true;
    }

    g_hSettingsWnd = CreateWindowExW(
        WS_EX_TOPMOST, L"GeminiSettingsClass", LOC[g_keys.lang].wSettingsTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 480,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
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

    ShowWindow(GetConsoleWindow(), SW_HIDE);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

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