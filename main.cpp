/*
* =========================================================================================
* 项目名称：某班电脑助手
* 版本号：v4.0.2.debug1
* 开发者：城南 
* * 【v4.0.2.debug1 深度重构日志 (2026-06-07)】：
* 1. 惩罚单选互斥重构 (Bug 1)：加入原生强制清空机制，彻底解决 Win32 Radio 幽灵复选问题。
* 2. 厕所7天完整性校验 (Bug 2)：日历引擎升级，只有处于完整未来时间的周才允许分配打扫厕所。
* 3. 预设扣分与组件升级 (Bug 3)：扣分框升级为 COMBOBOX（支持输入和下拉选预设），违纪/卫生自动带入 -0.2。
* 4. 精准日期溯源 (Bug 4)：摒弃“第X周”记录，运用 <ctime> 精确计算并记录类似“06月08日-06月14日”的物理日期。
* 5. 极简 UI 与新布局 (Req & Bug 5)：消除冗余啰嗦文本，增加控件呼吸感，重排 Tab 顺序，完善托盘分类菜单。
* =========================================================================================
*/

#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <ctime>
#include <stdio.h> 
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <regex>       // 引入正则表达式库，用于处理剪贴板智能排版
#include <commctrl.h>  // 引入通用控件库，用于 Tab 标签页和 ListView 列表
#include <commdlg.h>   // 引入通用对话框库，用于选择音频文件
#include <map>         // 引入 map，用于汇总统计学生总扣分
#include <fstream>     // 引入 fstream，用于输出 Excel CSV 电子表格
#include <sstream>     // 引入 sstream，用于组装电子表格字符串

#define DEBUG_FORCE_SHOW_DUTY 0  // 调试宏

// 链接必须的系统库
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib") 
#pragma comment(lib, "ole32.lib") 
#pragma comment(lib, "comctl32.lib") // 必须链接：提供高级UI控件支持

#pragma comment(linker, "/subsystem:windows")
#pragma comment(linker, "/entry:wWinMainCRTStartup")

// ======================= [ 常量及 ID 定义 ] =======================
#define IDI_APP_ICON    101  
#define TIMER_ID        1

// 托盘菜单 ID (更新分类菜单 ID)
#define WM_TRAYICON          (WM_USER + 1)
#define IDM_SHOW_MAIN        1001 // 打开主面板 (首页)
#define IDM_SHOW_OPT         1002 // 打开系统设置
#define IDM_ABOUT            1003
#define IDM_EXIT             1004
#define IDM_OPEN_DIR         1005
#define IDM_TOGGLE_DUTY      1010
#define IDM_NEXT_GROUP       1011
#define IDM_LOCK_SCREEN      1012
#define IDM_RESTART_EXPLORER 1013
#define IDM_FORMAT_TXT       1014 // 托盘快捷：排版剪贴板
#define IDM_OPEN_SEAT        1015 // 托盘快捷：打开座次表
#define IDM_GEN_REPORT       1016 // 托盘快捷：生成报表

// UI 控件 ID
#define IDC_TAB           1999
#define IDC_CHK_MORNING   2001
#define IDC_CHK_NOON      2002
#define IDC_CHK_EVENING   2003
#define IDC_CHK_NIGHT     2004
#define IDC_RDO_PLAN_A    2005
#define IDC_RDO_PLAN_B    2006
#define IDC_BTN_DUTY      2007
#define IDC_BTN_NEXT_GROUP 2008
#define IDC_BTN_FORMAT_TXT 2009 // 排版按钮
#define IDC_LNK_GITHUB    2010  // 新增的 GitHub 超链接控件 ID

// 自定义任务相关 UI ID
#define IDC_LIST_TASKS    3001
#define IDC_TIME_PICKER   3002
#define IDC_EDIT_FILEPATH 3003
#define IDC_BTN_BROWSE    3004
#define IDC_BTN_ADD_TASK  3005
#define IDC_BTN_DEL_TASK  3006

// 惩罚系统 UI ID
#define IDC_CHK_PUNISH_ENABLE 6001
#define IDC_BTN_OPEN_PUNISH   6002
#define IDC_BTN_QUERY_PUNISH  6003 // 报表查询与生成按钮

// 惩罚登记表单 ID
#define IDC_RDO_REASON_1  7001
#define IDC_RDO_REASON_2  7002
#define IDC_RDO_REASON_3  7003
#define IDC_EDIT_SCORE    7004
#define IDC_RDO_ACTION_1  7005
#define IDC_RDO_ACTION_2  7006
#define IDC_RDO_ACTION_3  7007
#define IDC_BTN_SUBMIT_PUNISH 7008
#define IDC_EDIT_OPERATOR     7009 // 操作员框
#define IDC_RDO_ACTION_4  7010 // 新增无惩罚选项 ID

// ======================= [ 文件路径常量 ] =======================
const wchar_t* RING_AUDIO = L"D:\\yuan_Timing\\ring.wav";
const wchar_t* LOG_FILE = L"D:\\yuan_Timing\\yuan_Timing_Log.log";
const wchar_t* DUTY_FILE = L"D:\\yuan_Timing\\duty.txt";
const wchar_t* TASKS_FILE = L"D:\\yuan_Timing\\custom_tasks.txt"; // 自定义音频任务存储
const wchar_t* SEATS_FILE = L"D:\\yuan_Timing\\seats.txt"; // 座次表解析文件
const wchar_t* PUNISH_LOG = L"D:\\yuan_Timing\\punishment_log.txt"; // 惩罚记录存储

const int TIME_MORNING_H = 6;  const int TIME_MORNING_M = 1;
const int TIME_NOON_H = 11;    const int TIME_NOON_M = 59;
const int TIME_EVENING_H = 17; const int TIME_EVENING_M = 50;
const int TIME_NIGHT_H = 21;   const int TIME_NIGHT_M = 38;
const int TIMEOUT_MS = 60000;

// ======================= [ 自定义播报任务结构体 ] =======================
struct CustomTask {
    int hour;
    int min;
    std::wstring filepath;
};
std::vector<CustomTask> g_Tasks; // 存储所有自定义任务

// 惩罚查询用的结构体
struct PunishRecord {
    std::wstring date;
    std::wstring student;
    std::wstring reason;
    double score;
    std::wstring action;
    std::wstring operatorName;
    std::wstring execTime; // 新增执行时间字段
};

// ======================= [ 全局状态与句柄 ] =======================
NOTIFYICONDATAW nid;
HFONT hFontTitle, hFontNormal, hFontBig;
HWND g_hMainWnd = NULL;
HWND hTabControl;

// 存储不同 Tab 页面对应的控件句柄，方便切换时批量隐藏/显示
std::vector<HWND> g_TabHomeCtrls;
std::vector<HWND> g_TabTaskCtrls;
std::vector<HWND> g_TabPunishCtrls; // 调整后的 Tab 2 (惩罚系统)
std::vector<HWND> g_TabOptCtrls;    // 调整后的 Tab 3 (系统设置)

// 具体的控件句柄
HWND hChkMorning, hChkNoon, hChkEvening, hChkNight, hRdoPlanA, hRdoPlanB, hDutyLabel;
HWND hListView, hTimePicker, hEditPath;
HWND hDutyOverlay = NULL;

DWORD g_Morning = 1, g_Noon = 1, g_Evening = 1, g_Night = 1;
DWORD g_ReadingPlan = 0; 
DWORD g_CurrentDutyGroup = 0; 
DWORD g_PunishEnable = 1; // 惩罚系统总开关

bool g_DutyWindowClosedByUser = false;
bool g_UserOpenedManually = false;
std::vector<std::wstring> g_DutyList;
bool g_opened_first_session = false;
bool g_opened_second_session = false;

// 座次表与惩罚登记管理
std::vector<std::vector<std::wstring>> g_SeatGrid; // 存储清洗后的二维座次
HWND hSeatingWnd = NULL;
HWND hPunishDlg = NULL;
std::wstring g_CurrentPunishStudent = L""; // 当前正在被处理的学生

// 惩罚窗口内部状态
int p_Reason = 0; // 0:无, 1:迟到, 2:宿舍卫生, 3:宿舍违纪
int p_Action = 0; // 0:无, 1:厕所, 2:检讨, 3:宿舍卫生, 4:无惩罚
int p_SelWeek = -1; // 选中的周(0~2)
int p_SelDay = -1;  // 选中的日(0~20)

typedef int(__stdcall* MSGBOXTIMEOUTW)(HWND, LPCWSTR, LPCWSTR, UINT, WORD, DWORD);

// ======================= [ 前置函数声明 ] =======================
DWORD WINAPI PlaySoundThread(LPVOID lpParam); 
void ShowSilentShutdownBox();
void SwitchTab(int index);
void SaveCustomTasks();
void LoadCustomTasks();
void RefreshTaskListView();
void GenerateAndOpenPunishReport(HWND hwnd); // 提前声明用于托盘
LRESULT CALLBACK SeatingWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PunishDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ======================= [ 工具核心函数 ] =======================

/* 获取当天的默认值班组 (星期一为0，以此类推) */
int GetDefaultGroupForToday() {
    time_t t = time(nullptr); tm now; localtime_s(&now, &t);
    int wday = now.tm_wday; 
    if (wday == 0) return 6; // 星期日
    return wday - 1; 
}

/* 字符串分割工具 */
std::vector<std::wstring> SplitString(const std::wstring& str, const std::wstring& delim) {
    std::vector<std::wstring> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == std::wstring::npos) pos = str.length();
        std::wstring token = str.substr(prev, pos - prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

/* 日志记录工具 */
void WriteLog(const wchar_t* message) {
    CreateDirectoryW(L"D:\\yuan_Timing", NULL);
    FILE* fp = nullptr;
    _wfopen_s(&fp, LOG_FILE, L"a, ccs=UTF-8");
    if (fp) {
        time_t t = time(nullptr); tm now; localtime_s(&now, &t);
        fwprintf(fp, L"[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
            now.tm_hour, now.tm_min, now.tm_sec, message);
        fclose(fp);
    }
}

// 剔除姓名中的数字（例如“张三1”变为“张三”）
std::wstring CleanStudentName(const std::wstring& raw) {
    std::wstring res = L"";
    for (wchar_t c : raw) {
        if (c < L'0' || c > L'9') res += c;
    }
    return res;
}

// ======================= [ 剪贴板智能排版神器 ] =======================
/*
* 函数名称：FormatClipboardTextTool
* 作用：读取剪贴板，使用逐行扫描强力删除空行，使用正则智能为英语单词和句子排版换行。
*/
void FormatClipboardTextTool(HWND hwnd) {
    if (!OpenClipboard(hwnd)) return;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return; }
    
    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
    if (!pszText) { CloseClipboard(); return; }
    std::wstring text(pszText);
    GlobalUnlock(hData);
    CloseClipboard();

    try {
        // 1. 去除 QQ 提示信息
        std::wregex qq_reg(L".*?\\d{4}/\\d{1,2}/\\d{1,2}\\s+\\d{1,2}:\\d{2}:?\\d*\\s*\\r?\\n");
        text = std::regex_replace(text, qq_reg, L"");

        // 2. 词汇分隔 
        std::wregex vocab_reg(L"\\s+(\\d+\\.[a-zA-Z]+)");
        text = std::regex_replace(text, vocab_reg, L"\r\n$1");

        // 3. 英语句子分隔 
        std::wregex sentence_reg(L"(\\.)\\s*([A-Z我你他她])");
        text = std::regex_replace(text, sentence_reg, L"$1\r\n$2");
    }
    catch (const std::regex_error&) {
        WriteLog(L"错误：剪贴板正则排版处理异常");
        MessageBoxW(NULL, L"文本内容过于复杂导致排版失败。", L"某班电脑助手", MB_OK | MB_ICONERROR);
        return;
    }

    // 使用逐行扫描强力删除所有空行，彻底解决正则对不可见空白字符无效的问题
    std::wstring resultText = L"";
    size_t startPos = 0;
    while (startPos < text.length()) {
        size_t endPos = text.find(L'\n', startPos);
        if (endPos == std::wstring::npos) endPos = text.length();

        std::wstring line = text.substr(startPos, endPos - startPos);
        startPos = endPos + 1;

        // 剥离末尾的 \r
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }

        // 智能判断：只要包含非空白字符，就认为不是空行
        bool isReallyEmpty = true;
        for (wchar_t c : line) {
            // 强力过滤普通空格、制表符、中文全角空格(0x3000)
            if (c != L' ' && c != L'\t' && c != 0x3000) { 
                isReallyEmpty = false;
                break;
            }
        }

        // 只有真正有文字的行，才拼接到最终结果里，完美消除中间所有的空换行
        if (!isReallyEmpty) {
            resultText += line + L"\r\n"; 
        }
    }

    // 去除排版后最后面多余的换行
    if (resultText.length() >= 2) {
        resultText = resultText.substr(0, resultText.length() - 2);
    }
    text = resultText;

    // 将排版后的文本重新写回剪贴板
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
        if (hGlobal) {
            wchar_t* pGlobal = (wchar_t*)GlobalLock(hGlobal);
            wcscpy_s(pGlobal, text.length() + 1, text.c_str());
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
        WriteLog(L"用户执行了一次剪贴板智能排版。");
        MessageBoxW(hwnd, L"✂️ 文本排版成功！\n\n已自动清除QQ时间戳、多余空行，并对句子/词汇智能换行。\n\n请直接按 Ctrl+V 粘贴查看效果。", L"某班电脑助手", MB_OK | MB_ICONINFORMATION);
    }
}

// ======================= [ 值日表与配置存取 ] =======================
static void InitDutyFile() {
    CreateDirectoryW(L"D:\\yuan_Timing", NULL);
    FILE* fp = nullptr;
    _wfopen_s(&fp, DUTY_FILE, L"r, ccs=UTF-8");
    if (!fp) {
        _wfopen_s(&fp, DUTY_FILE, L"w, ccs=UTF-8");
        if (fp) {
            const wchar_t* defaultList =
                L"组长-同学A；走廊和窗台-同学B、同学C、同学D；教室-同学E、同学F、同学G；垃圾桶-同学H；黑板和讲台-同学I、同学J\n\
组长-同学K；走廊和窗台-同学L、同学M、同学N；教室-同学O、同学P、同学Q；垃圾桶-同学R；黑板和讲台-同学S、同学T\n\
组长-同学U；走廊和窗台-同学V、同学W、同学X；教室-同学Y、同学Z、同学AA；垃圾桶-同学AB；黑板和讲台-同学AC、同学AD\n\
组长-同学AE；走廊和窗台-同学AF、同学AG、同学AH；教室-同学AI、同学AJ、同学AK；垃圾桶-同学AL；黑板和讲台-同学AM、同学AN\n\
组长-同学AO；走廊和窗台-同学AP、同学AQ、同学AR；教室-同学AS、同学AT、同学AU；垃圾桶-同学AV；黑板和讲台-同学AW、同学AX\n\
组长-同学AY；走廊和窗台-同学AZ、同学BA；教室-同学BB、同学BC；垃圾桶-同学BD；黑板和讲台-同学BE\n\
组长-同学BF；走廊和窗台-同学BG、同学BH；教室-同学BI、同学BJ；垃圾桶-同学BK；黑板和讲台-同学BL";
            fputws(defaultList, fp);
            fclose(fp);
        }
    } else fclose(fp);
}

// 生成用于座次表的默认 txt (教导用户如何从 word 复制粘贴)
static void InitSeatFile() {
    FILE* fp = nullptr;
    _wfopen_s(&fp, SEATS_FILE, L"r, ccs=UTF-8");
    if (!fp) {
        _wfopen_s(&fp, SEATS_FILE, L"w, ccs=UTF-8");
        if (fp) {
            const wchar_t* hint = 
                L"说明：请直接打开 Word 座次表，全选表格(Ctrl+A)，复制(Ctrl+C)，然后覆盖粘贴到这里保存即可！\n\n\
同学A1 同学B2 讲台 同学C3 同学D4\n\
同学E1 同学F2 走廊 同学G3 同学H4\n\
班委名单：\n这行以下的内容会被系统自动忽略，不会显示在座次表里。";
            fputws(hint, fp);
            fclose(fp);
        }
    } else fclose(fp);
}

void LoadDutyData() {
    g_DutyList.clear();
    FILE* fp = nullptr;
    _wfopen_s(&fp, DUTY_FILE, L"r, ccs=UTF-8");
    if (fp) {
        wchar_t line[1024];
        while (fgetws(line, 1024, fp)) {
            std::wstring s = line;
            while (!s.empty() && (s.back() == L'\n' || s.back() == L'\r')) s.pop_back();
            if (!s.empty()) g_DutyList.push_back(s);
        }
        fclose(fp);
    }
}

void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\YuanTimingAssistant", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"Morning", 0, REG_DWORD, (BYTE*)&g_Morning, sizeof(DWORD));
        RegSetValueExW(hKey, L"Noon", 0, REG_DWORD, (BYTE*)&g_Noon, sizeof(DWORD));
        RegSetValueExW(hKey, L"Evening", 0, REG_DWORD, (BYTE*)&g_Evening, sizeof(DWORD));
        RegSetValueExW(hKey, L"Night", 0, REG_DWORD, (BYTE*)&g_Night, sizeof(DWORD));
        RegSetValueExW(hKey, L"ReadingPlan", 0, REG_DWORD, (BYTE*)&g_ReadingPlan, sizeof(DWORD));
        RegSetValueExW(hKey, L"DutyGroup", 0, REG_DWORD, (BYTE*)&g_CurrentDutyGroup, sizeof(DWORD));
        RegSetValueExW(hKey, L"PunishEnable", 0, REG_DWORD, (BYTE*)&g_PunishEnable, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\YuanTimingAssistant", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(DWORD);
        RegQueryValueExW(hKey, L"Morning", NULL, NULL, (BYTE*)&g_Morning, &size);
        RegQueryValueExW(hKey, L"Noon", NULL, NULL, (BYTE*)&g_Noon, &size);
        RegQueryValueExW(hKey, L"Evening", NULL, NULL, (BYTE*)&g_Evening, &size);
        RegQueryValueExW(hKey, L"Night", NULL, NULL, (BYTE*)&g_Night, &size);
        RegQueryValueExW(hKey, L"ReadingPlan", NULL, NULL, (BYTE*)&g_ReadingPlan, &size);
        RegQueryValueExW(hKey, L"DutyGroup", NULL, NULL, (BYTE*)&g_CurrentDutyGroup, &size);
        RegQueryValueExW(hKey, L"PunishEnable", NULL, NULL, (BYTE*)&g_PunishEnable, &size);
        RegCloseKey(hKey);
    }
}

void UpdateDutyLabelUI() {
    if (g_hMainWnd && hDutyLabel) {
        int total = g_DutyList.empty() ? 1 : (int)g_DutyList.size();
        wchar_t buf[100];
        wsprintfW(buf, L"🌟 当前值班：第 %d 组 (共 %d 组)", g_CurrentDutyGroup + 1, total);
        SetWindowTextW(hDutyLabel, buf);
    }
}

// ======================= [ 自定义音频任务读写 ] =======================
void LoadCustomTasks() {
    g_Tasks.clear();
    FILE* fp = nullptr;
    _wfopen_s(&fp, TASKS_FILE, L"r, ccs=UTF-8");
    if (fp) {
        wchar_t line[1024];
        while (fgetws(line, 1024, fp)) {
            std::wstring s = line;
            while (!s.empty() && (s.back() == L'\n' || s.back() == L'\r')) s.pop_back();
            if (s.empty()) continue;
            
            size_t sep1 = s.find(L':');
            size_t sep2 = s.find(L'|');
            if (sep1 != std::wstring::npos && sep2 != std::wstring::npos) {
                CustomTask tk;
                tk.hour = _wtoi(s.substr(0, sep1).c_str());
                tk.min = _wtoi(s.substr(sep1 + 1, sep2 - sep1 - 1).c_str());
                tk.filepath = s.substr(sep2 + 1);
                g_Tasks.push_back(tk);
            }
        }
        fclose(fp);
    }
}

void SaveCustomTasks() {
    FILE* fp = nullptr;
    _wfopen_s(&fp, TASKS_FILE, L"w, ccs=UTF-8");
    if (fp) {
        for (const auto& t : g_Tasks) {
            fwprintf(fp, L"%02d:%02d|%s\n", t.hour, t.min, t.filepath.c_str());
        }
        fclose(fp);
    }
}

void RefreshTaskListView() {
    if (!hListView) return;
    ListView_DeleteAllItems(hListView);
    for (size_t i = 0; i < g_Tasks.size(); ++i) {
        wchar_t timeBuf[32];
        wsprintfW(timeBuf, L"%02d:%02d", g_Tasks[i].hour, g_Tasks[i].min);
        
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.pszText = timeBuf;
        ListView_InsertItem(hListView, &lvi);
        ListView_SetItemText(hListView, (int)i, 1, (LPWSTR)g_Tasks[i].filepath.c_str());
    }
}

// ======================= [ 一键生成 Excel 表格报表引擎 ] =======================
/*
* 函数名称：GenerateAndOpenPunishReport
* 作用：读取本地 txt 台账文件，根据学生姓名进行分数汇总聚合，输出为标准的带 BOM 的 UTF-8 csv文件，并唤醒 Excel。
*/
void GenerateAndOpenPunishReport(HWND hwnd) {
    std::vector<PunishRecord> records;
    FILE* fp = nullptr;
    _wfopen_s(&fp, PUNISH_LOG, L"r, ccs=UTF-8");
    if (!fp) {
        MessageBoxW(hwnd, L"暂无任何惩罚记录可以统计。", L"某班电脑助手", MB_OK);
        return;
    }

    wchar_t line[1024];
    while (fgetws(line, 1024, fp)) {
        std::wstring s = line;
        // 匹配新日志结构，解析执行时间（格式：xx月xx日-xx月xx日，无空格，完美契合 \S+）
        std::wregex reg(L"\\[(.*?)\\] 学生:(\\S+)\\s+原因:(\\S+)\\s+扣分:([-\\.\\d]*)\\s+措施:(\\S+)(?:\\s+操作员:(\\S+))?(?:\\s+执行时间:(\\S+))?");
        std::wsmatch match;
        if (std::regex_search(s, match, reg)) {
            PunishRecord rec;
            rec.date = match[1];
            rec.student = match[2];
            rec.reason = match[3];
            try {
                std::wstring scStr = match[4].str();
                rec.score = scStr.empty() ? 0.0 : std::stod(scStr);
            } catch (...) { rec.score = 0.0; }
            rec.action = match[5];
            rec.operatorName = match[6].matched ? match[6].str() : L"未知";
            rec.execTime = match[7].matched ? match[7].str() : L"无"; 
            records.push_back(rec);
        }
    }
    fclose(fp);

    // 智能数据聚合：汇总每人的扣分总和
    std::map<std::wstring, double> totalScores;
    for (const auto& r : records) {
        totalScores[r.student] += r.score;
    }

    // 每次生成时都直接覆盖旧报表，实现“看完就删/每次重新生成”的假象
    const wchar_t* csvPath = L"D:\\yuan_Timing\\punishment_report.csv";
    
    // std::ios::binary 是为了后续注入 UTF-8 BOM，防止 Excel 打开 CSV 时产生恶心的乱码
    std::ofstream out(csvPath, std::ios::binary);
    if (!out) {
        MessageBoxW(hwnd, L"生成报表失败！文件可能正被你的 Excel 占用，请先关闭原来的报表再点击生成！", L"某班电脑助手", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 强行注入 UTF-8 BOM 签名
    unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    out.write(reinterpret_cast<char*>(bom), sizeof(bom));
    
    // 封装转码写入器
    auto writeLine = [&out](const std::wstring& line) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, NULL, 0, NULL, NULL);
        std::string utf8_line(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, &utf8_line[0], size_needed, NULL, NULL);
        utf8_line.pop_back(); 
        out << utf8_line << "\n";
    };

    writeLine(L"=== 某班惩罚系统统计报表 ===");
    writeLine(L"");
    writeLine(L"【每人累计扣分汇总 (按总分合并)】");
    writeLine(L"姓名,累计扣分总和");
    for (const auto& kv : totalScores) {
        std::wstringstream ss;
        ss << kv.first << L"," << kv.second;
        writeLine(ss.str());
    }
    
    writeLine(L"");
    writeLine(L"【惩罚流水明细记录】");
    writeLine(L"发生日期,违纪学生,扣分原因,扣除分值,执行措施,本次记录操作员,执行日期/周期");
    for (const auto& r : records) {
        std::wstringstream ss;
        ss << r.date << L"," << r.student << L"," << r.reason << L"," << r.score << L"," << r.action << L"," << r.operatorName << L"," << r.execTime;
        writeLine(ss.str());
    }
    
    out.close();
    
    // 一键直接用系统默认表格程序（Excel 或 WPS）打开 CSV
    ShellExecuteW(NULL, L"open", csvPath, NULL, NULL, SW_SHOW);
    WriteLog(L"生成并打开了惩罚系统统计报表。");
}

// ======================= [ 定时任务与系统引擎 ] =======================
void SetSystemVolumeTo30() {
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        IMMDeviceEnumerator* pEnumerator = NULL;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (SUCCEEDED(hr)) {
            IMMDevice* pDevice = NULL;
            hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
            if (SUCCEEDED(hr)) {
                IAudioEndpointVolume* pAudioEndpointVolume = NULL;
                hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);
                if (SUCCEEDED(hr)) {
                    pAudioEndpointVolume->SetMute(FALSE, NULL);
                    pAudioEndpointVolume->SetMasterVolumeLevelScalar(0.30f, NULL);
                    pAudioEndpointVolume->Release();
                }
                pDevice->Release();
            }
            pEnumerator->Release();
        }
        CoUninitialize();
    }
}

std::wstring GetDesktopPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path))) return std::wstring(path);
    return L"C:\\";
}

void ExecuteMorningReading(int current_hour, int current_min) {
    if (current_hour >= 8) return;

    bool trigger_first = (current_hour < 7) && !g_opened_first_session;
    bool trigger_second = (current_hour == 7 && current_min >= 15) && !g_opened_second_session;

    if (!trigger_first && !trigger_second) return;

    std::wstring searchDir = GetDesktopPath();
    std::wstring searchPattern = searchDir + L"\\*早读*.pptx";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    std::vector<std::wstring> allFiles;
    do { 
        std::wstring fname = findData.cFileName;
        // 智能过滤 Office 自动生成的 ~$ 临时备份/锁定文件
        if (fname.length() >= 2 && fname.compare(0, 2, L"~$") == 0) continue; 
        allFiles.push_back(fname); 
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    std::wstring targetFile = L"";

    for (const auto& file : allFiles) {
        bool has6 = (file.find(L"6点") != std::wstring::npos || file.find(L"六点") != std::wstring::npos);
        bool has7 = (file.find(L"7点") != std::wstring::npos || file.find(L"七点") != std::wstring::npos || file.find(L"7:20") != std::wstring::npos);
        if (trigger_first && has6) targetFile = file;
        if (trigger_second && has7) targetFile = file;
    }

    if (targetFile.empty()) {
        std::wstring fileChinese = L"", fileEnglish = L"", fileUnknown = L"";
        for (const auto& file : allFiles) {
            if (file.find(L"语文") != std::wstring::npos) fileChinese = file;
            else if (file.find(L"英语") != std::wstring::npos) fileEnglish = file;
            else if (fileUnknown.empty()) fileUnknown = file;
        }
        if (!fileChinese.empty() && fileEnglish.empty() && !fileUnknown.empty()) fileEnglish = fileUnknown;
        if (!fileEnglish.empty() && fileChinese.empty() && !fileUnknown.empty()) fileChinese = fileUnknown;
        if (fileChinese.empty() && fileEnglish.empty()) return;

        if (trigger_first) targetFile = (g_ReadingPlan == 0) ? fileChinese : fileEnglish;
        else if (trigger_second) targetFile = (g_ReadingPlan == 0) ? fileEnglish : fileChinese;
    }

    if (!targetFile.empty()) {
        std::wstring fullPath = searchDir + L"\\" + targetFile;
        ShellExecuteW(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_SHOW);
        if (trigger_first) g_opened_first_session = true;
        else g_opened_second_session = true;
        WriteLog((L"执行早读自动化 (已过滤临时文件，成功打开: " + targetFile + L")").c_str());
    }
}

/*
* 函数名称：CheckTimeAndExecute
* 作用：中央调度引擎，每秒钟执行一次。统筹管理 8:00 A/B 方案轮换、值日绑定、自定义音频调度、系统打铃。
*/
void CheckTimeAndExecute() {
    static int last_triggered_min = -1;
    static int last_day = -1;
    static int last_read_rotate_yday = -1; 
    static int last_shutdown_min = -1; // 用于锁定同一分钟内不重复触发关机弹窗

    time_t t = time(nullptr); tm now; localtime_s(&now, &t);

    // 1. 每天跨天重置与星期绑定 (零点执行)
    if (now.tm_yday != last_day) {
        g_CurrentDutyGroup = GetDefaultGroupForToday(); 
        SaveSettings();
        WriteLog(L"--- 跨天重置：值班组已按星期自动匹配 ---");

        if (g_hMainWnd && IsWindowVisible(g_hMainWnd)) UpdateDutyLabelUI();

        g_opened_first_session = false;
        g_opened_second_session = false;
        g_DutyWindowClosedByUser = false;
        g_UserOpenedManually = false;
        last_day = now.tm_yday;
    }

    // 2. 早读方案精准 08:00 轮换
    if (now.tm_hour == 8 && now.tm_min == 0 && last_read_rotate_yday != now.tm_yday) {
        g_ReadingPlan = (g_ReadingPlan == 0) ? 1 : 0;
        SaveSettings();
        last_read_rotate_yday = now.tm_yday;
        WriteLog(L"--- 触发 08:00 动作：早读方案 A/B 已自动轮换 ---");
        
        if (g_hMainWnd && IsWindowVisible(g_hMainWnd)) {
            SendMessage(hRdoPlanA, BM_SETCHECK, g_ReadingPlan == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(hRdoPlanB, BM_SETCHECK, g_ReadingPlan == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }

    // 3. 检测早读任务
    ExecuteMorningReading(now.tm_hour, now.tm_min);

    // 4. 悬浮窗时间段控制逻辑
    int current_minutes = now.tm_hour * 60 + now.tm_min;
#if DEBUG_FORCE_SHOW_DUTY
    bool isDutyTime = true;
#else
    bool isDutyTime = (current_minutes >= 6 * 60 + 50 && current_minutes <= 7 * 60 + 20) ||
        (current_minutes >= 18 * 60 + 25 && current_minutes <= 18 * 60 + 55);
#endif

    if (isDutyTime) {
        g_UserOpenedManually = false; 
        if (!g_DutyWindowClosedByUser && hDutyOverlay != NULL && !IsWindowVisible(hDutyOverlay)) {
            LoadDutyData();
            int sw = GetSystemMetrics(SM_CXSCREEN);
            SetWindowPos(hDutyOverlay, HWND_TOPMOST, sw - 370, 50, 340, 520, SWP_SHOWWINDOW);
            InvalidateRect(hDutyOverlay, NULL, TRUE);
        }
    } else {
        if (!g_UserOpenedManually && hDutyOverlay != NULL && IsWindowVisible(hDutyOverlay)) {
            ShowWindow(hDutyOverlay, SW_HIDE);
        }
        g_DutyWindowClosedByUser = false; 
    }

    // 中午和晚上自动关机逻辑 (独立出来，使用分钟锁，防止 MessageBox 阻塞导致循环)
    if ((g_Noon && now.tm_hour == TIME_NOON_H && now.tm_min == TIME_NOON_M) || 
        (g_Night && now.tm_hour == TIME_NIGHT_H && now.tm_min == TIME_NIGHT_M)) {
        if (now.tm_min != last_shutdown_min) {
            last_shutdown_min = now.tm_min; // 立刻锁定这一分钟，彻底防止 MessageBox 带来的死循环重入
            WriteLog(L"自动任务：触发关机弹窗");
            ShowSilentShutdownBox();
        }
    } else {
        if (now.tm_min != last_shutdown_min) last_shutdown_min = -1;
    }

    // 5. 常规打铃与自定义播报
    if (now.tm_min != last_triggered_min) {
        // 把时间锁定提前到最前面！防止 MessageBox 等可能的阻塞操作导致定时器陷入死循环
        last_triggered_min = now.tm_min; 
        
        // 5.1 检查并触发自定义音频任务 
        for (const auto& task : g_Tasks) {
            if (task.hour == now.tm_hour && task.min == now.tm_min) {
                WriteLog((L"触发自定义播放任务：" + task.filepath).c_str());
                std::wstring mciClose = L"close myaudio";
                std::wstring mciOpen = L"open \"" + task.filepath + L"\" type mpegvideo alias myaudio";
                std::wstring mciPlay = L"play myaudio";
                mciSendStringW(mciClose.c_str(), NULL, 0, NULL);
                mciSendStringW(mciOpen.c_str(), NULL, 0, NULL);
                mciSendStringW(mciPlay.c_str(), NULL, 0, NULL);
            }
        }

        // 5.2 系统默认打铃任务
        if (g_Morning && now.tm_hour == TIME_MORNING_H && now.tm_min == TIME_MORNING_M) {
            WriteLog(L"自动任务：早晨考勤铃");
            CreateThread(NULL, 0, PlaySoundThread, NULL, 0, NULL);
        }
        else if (g_Evening && now.tm_hour == TIME_EVENING_H && now.tm_min == TIME_EVENING_M) {
            WriteLog(L"自动任务：傍晚活动铃");
            CreateThread(NULL, 0, PlaySoundThread, NULL, 0, NULL);
        }
    }
}

// ======================= [ 值班名单透明悬浮窗 ] =======================
LRESULT CALLBACK DutyOverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
        PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); // 实现按住任意位置拖动
        return 0;
    case WM_LBUTTONDBLCLK:
        g_DutyWindowClosedByUser = true;
        g_UserOpenedManually = false;
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect; GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(RGB(35, 35, 45));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        SetBkMode(hdc, TRANSPARENT);

        HFONT hFontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        SelectObject(hdc, hFontTitle);
        SetTextColor(hdc, RGB(255, 255, 255));

        wchar_t title[100];
        wsprintfW(title, L"✨ 某班今日值日 (%d组)", g_CurrentDutyGroup + 1);
        RECT titleRect = { 20, 15, rect.right, 45 };
        DrawTextW(hdc, title, -1, &titleRect, DT_LEFT | DT_VCENTER);

        HFONT hFontTip = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        SelectObject(hdc, hFontTip);
        SetTextColor(hdc, RGB(180, 180, 180));
        RECT tipRect = { 0, 15, rect.right - 20, 45 };
        DrawTextW(hdc, L"[双击悬浮窗任意处关闭]", -1, &tipRect, DT_RIGHT | DT_VCENTER);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 120));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 20, 50, NULL);
        LineTo(hdc, rect.right - 20, 50);
        SelectObject(hdc, hOldPen); DeleteObject(hPen);

        HFONT hFontRole = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        HFONT hFontName = CreateFontW(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");

        if (g_CurrentDutyGroup < g_DutyList.size()) {
            std::wstring todayStr = g_DutyList[g_CurrentDutyGroup];
            std::vector<std::wstring> items = SplitString(todayStr, L"；");

            int yPos = 65;
            for (const auto& item : items) {
                size_t dash = item.find(L"-");
                if (dash != std::wstring::npos) {
                    std::wstring role = item.substr(0, dash);
                    std::wstring names = item.substr(dash + 1);

                    SelectObject(hdc, hFontRole);
                    SetTextColor(hdc, RGB(255, 215, 0));
                    TextOutW(hdc, 25, yPos, role.c_str(), (int)role.length());
                    yPos += 28;

                    SelectObject(hdc, hFontName);
                    SetTextColor(hdc, RGB(255, 255, 255));
                    TextOutW(hdc, 25, yPos, names.c_str(), (int)names.length());
                    yPos += 38;
                }
            }
        }
        DeleteObject(hFontTitle); DeleteObject(hFontTip);
        DeleteObject(hFontRole); DeleteObject(hFontName);
        EndPaint(hwnd, &ps);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ======================= [ 惩罚系统核心逻辑 ] =======================
// 解析座位配置 txt，剥离名字中的数字，并渲染间距
void ParseSeatsFile() {
    g_SeatGrid.clear();
    FILE* fp = nullptr;
    _wfopen_s(&fp, SEATS_FILE, L"r, ccs=UTF-8");
    if (!fp) return;
    
    wchar_t line[1024];
    while (fgetws(line, 1024, fp)) {
        std::wstring s = line;
        while (!s.empty() && (s.back() == L'\n' || s.back() == L'\r')) s.pop_back();
        if (s.empty()) continue;
        
        // 智能忽略从Word粘贴来时的指导说明文字
        if (s.find(L"说明：") != std::wstring::npos || s.find(L"Word") != std::wstring::npos) continue;

        // 遇到"班委名单"停止解析，忽略下面的所有内容
        if (s.find(L"班委名单") != std::wstring::npos) break;

        // 以空格/制表符切分行内姓名
        std::wregex ws_re(L"\\s+");
        std::wsregex_token_iterator it(s.begin(), s.end(), ws_re, -1);
        std::wsregex_token_iterator end;
        
        std::vector<std::wstring> rowSeats;
        for (; it != end; ++it) {
            std::wstring rawName = *it;
            if (!rawName.empty()) {
                rowSeats.push_back(CleanStudentName(rawName)); // 去除数字
            }
        }
        if (!rowSeats.empty()) g_SeatGrid.push_back(rowSeats);
    }
    fclose(fp);
}

// 座次表全屏窗口过程
LRESULT CALLBACK SeatingWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        ParseSeatsFile();
        if (g_SeatGrid.empty()) {
            MessageBoxW(hwnd, L"未检测到座位数据！请先修改 seats.txt 并粘贴 Word 表格内容。", L"某班电脑助手", MB_OK);
            return -1; 
        }

        // 动态生成物理座次网格按钮
        int marginX = 50, marginY = 50;
        int btnW = 100, btnH = 50;
        int gapX = 10, gapY = 20;
        int aisleGap = 40; // 走廊空隙宽度

        for (size_t r = 0; r < g_SeatGrid.size(); ++r) {
            int currentX = marginX; // 每行的X坐标起点
            for (size_t c = 0; c < g_SeatGrid[r].size(); ++c) {
                std::wstring name = g_SeatGrid[r][c];
                if (name != L"空" && name != L"空位" && name.find(L"讲台") == std::wstring::npos && name.find(L"走廊") == std::wstring::npos) {
                    HWND hBtn = CreateWindowW(L"BUTTON", name.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        currentX, marginY + r * (btnH + gapY), btnW, btnH,
                        hwnd, (HMENU)(4000 + r * 100 + c), NULL, NULL);
                    SendMessage(hBtn, WM_SETFONT, (WPARAM)hFontBig, TRUE);
                }
                else if (name.find(L"讲台") != std::wstring::npos) {
                     HWND hStatic = CreateWindowW(L"STATIC", L"讲台", WS_CHILD | WS_VISIBLE | SS_CENTER,
                        currentX, marginY + r * (btnH + gapY), btnW, btnH, hwnd, NULL, NULL, NULL);
                     SendMessage(hStatic, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
                }
                
                // 推进坐标 (按钮宽度 + 基础间隙)
                currentX += btnW + gapX;

                // 每两排同学空出一个走廊的空隙
                if ((c + 1) % 2 == 0) {
                    currentX += aisleGap;
                }
            }
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= 4000) {
            int r = (id - 4000) / 100;
            int c = (id - 4000) % 100;
            if (r < g_SeatGrid.size() && c < g_SeatGrid[r].size()) {
                g_CurrentPunishStudent = g_SeatGrid[r][c];
                
                // 初始化惩罚弹窗默认状态
                p_Reason = 0; p_Action = 4; // 默认置为“无惩罚”
                p_SelWeek = -1; p_SelDay = -1;
                
                if (hPunishDlg) DestroyWindow(hPunishDlg);
                hPunishDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"PunishDlgClass", (L"违纪惩罚登记 - " + g_CurrentPunishStudent).c_str(),
                    WS_POPUPWINDOW | WS_CAPTION, GetSystemMetrics(SM_CXSCREEN)/2 - 300, GetSystemMetrics(SM_CYSCREEN)/2 - 250, 
                    600, 500, hwnd, NULL, GetModuleHandle(NULL), NULL);
                ShowWindow(hPunishDlg, SW_SHOW);
            }
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hSeatingWnd = NULL;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 惩罚登记弹窗窗口过程 (核心联动与日历绘制)
LRESULT CALLBACK PunishDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int by = 20;
        HWND hTarget = CreateWindowW(L"STATIC", (L"处分对象：【" + g_CurrentPunishStudent + L"】").c_str(), WS_CHILD|WS_VISIBLE, 20, by, 250, 30, hwnd, NULL, NULL, NULL);
        
        // 【Bug 4修复】：操作员升级为下拉编辑框（可自动补全，也可手动覆盖）
        HWND l_op = CreateWindowW(L"STATIC", L"操作员：", WS_CHILD|WS_VISIBLE, 300, by+5, 60, 25, hwnd, NULL, NULL, NULL);
        HWND cb_op = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|CBS_DROPDOWN, 360, by+2, 100, 200, hwnd, (HMENU)IDC_EDIT_OPERATOR, NULL, NULL);
        
        time_t t_now = time(nullptr); tm today_op; localtime_s(&today_op, &t_now);
        const wchar_t* ops[] = { L"操作员A", L"操作员B", L"操作员C", L"操作员D", L"操作员E", L"操作员F", L"操作员G", L"系统管理员" };
        for (int i = 0; i < 8; ++i) {
            SendMessageW(cb_op, CB_ADDSTRING, 0, (LPARAM)ops[i]);
        }
        SetWindowTextW(cb_op, ops[today_op.tm_wday]);

        // --- 第一行 ---
        by += 50;
        HWND l1 = CreateWindowW(L"STATIC", L"扣分原因：", WS_CHILD|WS_VISIBLE|WS_GROUP, 20, by, 100, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"迟到", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP, 120, by, 80, 25, hwnd, (HMENU)IDC_RDO_REASON_1, NULL, NULL);
        CreateWindowW(L"BUTTON", L"宿舍卫生", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 210, by, 100, 25, hwnd, (HMENU)IDC_RDO_REASON_2, NULL, NULL);
        CreateWindowW(L"BUTTON", L"宿舍违纪", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 320, by, 100, 25, hwnd, (HMENU)IDC_RDO_REASON_3, NULL, NULL);
        
        HWND l1b = CreateWindowW(L"STATIC", L"扣分分值：", WS_CHILD|WS_VISIBLE|WS_GROUP, 430, by, 80, 25, hwnd, NULL, NULL, NULL); 
        // 【Bug 3修复】：将扣分改为 Combobox 下拉框
        HWND cb_score = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|CBS_DROPDOWN, 510, by-2, 60, 200, hwnd, (HMENU)IDC_EDIT_SCORE, NULL, NULL);
        const wchar_t* scores[] = { L"-0.2", L"-0.5", L"-2", L"-5" };
        for (int i = 0; i < 4; ++i) SendMessageW(cb_score, CB_ADDSTRING, 0, (LPARAM)scores[i]);
        
        // --- 第二行 ---
        by += 40;
        HWND l2 = CreateWindowW(L"STATIC", L"惩罚措施：", WS_CHILD|WS_VISIBLE|WS_GROUP, 20, by, 100, 25, hwnd, NULL, NULL, NULL);
        // 新增无惩罚选项
        CreateWindowW(L"BUTTON", L"无惩罚", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP, 120, by, 80, 25, hwnd, (HMENU)IDC_RDO_ACTION_4, NULL, NULL);
        CreateWindowW(L"BUTTON", L"打扫厕所一周", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 200, by, 120, 25, hwnd, (HMENU)IDC_RDO_ACTION_1, NULL, NULL);
        CreateWindowW(L"BUTTON", L"写检讨", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 330, by, 80, 25, hwnd, (HMENU)IDC_RDO_ACTION_2, NULL, NULL);
        CreateWindowW(L"BUTTON", L"打扫宿舍卫生", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 420, by, 120, 25, hwnd, (HMENU)IDC_RDO_ACTION_3, NULL, NULL);

        // 初始化时强行选中“无惩罚”
        SendDlgItemMessageW(hwnd, IDC_RDO_ACTION_4, BM_SETCHECK, BST_CHECKED, 0);

        // --- 第三行 (执行时间) ---
        by += 40;
        HWND l3 = CreateWindowW(L"STATIC", L"执行时间 (三周日历)：", WS_CHILD|WS_VISIBLE|WS_GROUP, 20, by, 200, 25, hwnd, NULL, NULL, NULL); // 结束第二组单选

        HWND btn = CreateWindowW(L"BUTTON", L"确认登记处罚", WS_CHILD|WS_VISIBLE, 200, 400, 180, 40, hwnd, (HMENU)IDC_BTN_SUBMIT_PUNISH, NULL, NULL);

        // 设置字体
        EnumChildWindows(hwnd, [](HWND child, LPARAM)->BOOL{ SendMessage(child, WM_SETFONT, (WPARAM)hFontNormal, TRUE); return TRUE; }, 0);
        SendMessage(hTarget, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
        SendMessage(btn, WM_SETFONT, (WPARAM)hFontBig, TRUE);
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        
        // 【Bug 1修复】：定义闭包强制清除互斥组的状态
        auto ClearActionRadios = [&](HWND hDlg) {
            SendDlgItemMessageW(hDlg, IDC_RDO_ACTION_1, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessageW(hDlg, IDC_RDO_ACTION_2, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessageW(hDlg, IDC_RDO_ACTION_3, BM_SETCHECK, BST_UNCHECKED, 0);
            SendDlgItemMessageW(hDlg, IDC_RDO_ACTION_4, BM_SETCHECK, BST_UNCHECKED, 0);
        };

        if (id >= IDC_RDO_REASON_1 && id <= IDC_RDO_REASON_3) {
            p_Reason = id - 7000;
            // 严格联动逻辑
            if (p_Reason == 1) { // 迟到
                SetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_SCORE), L"-2");
                ClearActionRadios(hwnd);
                SendDlgItemMessageW(hwnd, IDC_RDO_ACTION_4, BM_SETCHECK, BST_CHECKED, 0);
                p_Action = 4; // 无惩罚
            } 
            else if (p_Reason == 2) { // 宿舍卫生
                SetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_SCORE), L"-0.2");
                ClearActionRadios(hwnd);
                SendDlgItemMessageW(hwnd, IDC_RDO_ACTION_3, BM_SETCHECK, BST_CHECKED, 0);
                p_Action = 3;
            } 
            else if (p_Reason == 3) { // 宿舍违纪
                // 【Bug 3修复】：联动自动填充扣分值
                SetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_SCORE), L"-2");
                ClearActionRadios(hwnd);
                SendDlgItemMessageW(hwnd, IDC_RDO_ACTION_1, BM_SETCHECK, BST_CHECKED, 0);
                p_Action = 1;
            }
            p_SelWeek = -1; p_SelDay = -1; // 切换选项清空选中状态
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if ((id >= IDC_RDO_ACTION_1 && id <= IDC_RDO_ACTION_3) || id == IDC_RDO_ACTION_4) {
            ClearActionRadios(hwnd);
            SendDlgItemMessageW(hwnd, id, BM_SETCHECK, BST_CHECKED, 0);
            if (id == IDC_RDO_ACTION_4) p_Action = 4;
            else p_Action = id - 7004;
            p_SelWeek = -1; p_SelDay = -1;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (id == IDC_BTN_SUBMIT_PUNISH) {
            // 【Bug 3修复】：必须验证是否填写了扣分原因
            if (p_Reason == 0) {
                MessageBoxW(hwnd, L"必须选择一项扣分原因！", L"某班电脑助手", MB_OK | MB_ICONWARNING);
                return 0; 
            }

            // 保存至台账文件
            FILE* fp = nullptr; _wfopen_s(&fp, PUNISH_LOG, L"a, ccs=UTF-8");
            if (fp) {
                time_t t = time(nullptr); tm now; localtime_s(&now, &t);
                wchar_t score[20]; GetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_SCORE), score, 20);
                wchar_t opName[50]; GetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_OPERATOR), opName, 50);
                
                const wchar_t* reasons[] = {L"未指定", L"迟到", L"宿舍卫生", L"宿舍违纪"};
                const wchar_t* actions[] = {L"未指定", L"打扫厕所一周", L"写检讨", L"打扫宿舍卫生", L"无惩罚"};

                std::wstring execTimeStr = L"无";
                // 【Bug 4修复】：计算并导出精确的时间区间（XX月XX日-XX月XX日）
                if (p_Action == 1 && p_SelWeek != -1) {
                    int daysSinceMon = (now.tm_wday == 0) ? 6 : (now.tm_wday - 1);
                    time_t mon_t = t - daysSinceMon * 24 * 3600;
                    time_t target_mon_t = mon_t + p_SelWeek * 7 * 24 * 3600;
                    time_t target_sun_t = target_mon_t + 6 * 24 * 3600;
                    
                    tm tm_start, tm_end;
                    localtime_s(&tm_start, &target_mon_t);
                    localtime_s(&tm_end, &target_sun_t);
                    
                    wchar_t dateBuf[100];
                    wsprintfW(dateBuf, L"%02d月%02d日-%02d月%02d日", tm_start.tm_mon+1, tm_start.tm_mday, tm_end.tm_mon+1, tm_end.tm_mday);
                    execTimeStr = dateBuf;
                }

                fwprintf(fp, L"[%04d-%02d-%02d] 学生:%s 原因:%s 扣分:%s 措施:%s 操作员:%s 执行时间:%s\n",
                    now.tm_year+1900, now.tm_mon+1, now.tm_mday, g_CurrentPunishStudent.c_str(), 
                    reasons[p_Reason], score, actions[p_Action], opName, execTimeStr.c_str());
                fclose(fp);
                MessageBoxW(hwnd, L"处罚登记已成功录入档案！", L"某班电脑助手", MB_OK);
                DestroyWindow(hwnd); hPunishDlg = NULL;
            }
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        SetBkMode(hdc, TRANSPARENT); SelectObject(hdc, hFontNormal);

        time_t t = time(nullptr); tm today; localtime_s(&today, &t);
        int daysSinceMon = (today.tm_wday == 0) ? 6 : (today.tm_wday - 1);
        time_t mon_t = t - daysSinceMon * 24 * 3600;

        int cx = 20, cy = 180, cw = 75, ch = 60; // 绘图基准
        
        // 只有选择措施1(打扫厕所一周)才能选日历
        bool disabled = (p_Action != 1);

        const wchar_t* wdays[] = {L"星期一",L"星期二",L"星期三",L"星期四",L"星期五",L"星期六",L"星期日"};
        
        for (int w = 0; w < 3; ++w) {
            
            // 【Bug 2修复】：扫厕所7天完整性验证：如果该周包含任何过去日期(daysSinceMon>0表示本周一二已经过去)，则本周(w=0)视为不完整，必须整体置灰
            bool isInvalidWeek = false;
            if (w == 0 && daysSinceMon > 0 && p_Action == 1) {
                isInvalidWeek = true;
            }

            for (int d = 0; d < 7; ++d) {
                time_t d_t = mon_t + (w * 7 + d) * 24 * 3600;
                tm d_tm; localtime_s(&d_tm, &d_t);
                
                bool isPast = (d_tm.tm_year < today.tm_year) || 
                              (d_tm.tm_year == today.tm_year && d_tm.tm_yday < today.tm_yday);
                
                bool isSelected = (p_Action == 1) ? (w == p_SelWeek && !disabled && !isPast && !isInvalidWeek) : ((w*7+d) == p_SelDay && !disabled && !isPast);
                
                RECT cell = { cx + d*cw, cy + w*ch, cx + d*cw + cw - 2, cy + w*ch + ch - 2 };
                
                // 背景色判定
                HBRUSH hb = NULL;
                if (isPast || disabled || isInvalidWeek) hb = CreateSolidBrush(RGB(220, 220, 220)); // 置灰
                else if (isSelected) hb = CreateSolidBrush(RGB(173, 216, 230)); // 选中高亮蓝
                else hb = CreateSolidBrush(RGB(255, 255, 255)); // 默认白
                
                FillRect(hdc, &cell, hb); DeleteObject(hb);
                
                HPEN hp = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
                HPEN oldP = (HPEN)SelectObject(hdc, hp);
                MoveToEx(hdc, cell.left, cell.top, NULL); LineTo(hdc, cell.right, cell.top);
                LineTo(hdc, cell.right, cell.bottom); LineTo(hdc, cell.left, cell.bottom);
                LineTo(hdc, cell.left, cell.top);
                SelectObject(hdc, oldP); DeleteObject(hp);

                SetTextColor(hdc, (isPast || disabled || isInvalidWeek) ? RGB(120, 120, 120) : RGB(0, 0, 0));
                wchar_t txt[50]; wsprintfW(txt, L"%s\n%d/%d", wdays[d], d_tm.tm_mon+1, d_tm.tm_mday);
                DrawTextW(hdc, txt, -1, &cell, DT_CENTER | DT_VCENTER);
            }
        }

        // 联动高亮文本 
        if (p_Action == 3) {
            RECT tipR = { cx, cy + 3*ch + 5, cx + 7*cw, cy + 3*ch + 35 };
            SelectObject(hdc, hFontBig);
            SetTextColor(hdc, RGB(220, 20, 20));
            DrawTextW(hdc, L"“请通知舍长，舍长负责落实”", -1, &tipR, DT_LEFT);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_LBUTTONDOWN: {
        int mouseX = LOWORD(lParam); int mouseY = HIWORD(lParam);
        int cx = 20, cy = 180, cw = 75, ch = 60;
        
        bool disabled = (p_Action != 1); 
        
        if (!disabled && mouseX >= cx && mouseX <= cx + 7*cw && mouseY >= cy && mouseY <= cy + 3*ch) {
            int c = (mouseX - cx) / cw; int r = (mouseY - cy) / ch;
            
            time_t t = time(nullptr); tm today; localtime_s(&today, &t);
            int daysSinceMon = (today.tm_wday == 0) ? 6 : (today.tm_wday - 1);
            
            // 【Bug 2修复】：如果点击的是第一周（当前周），且本周已有残缺的天数，直接拦截点击操作
            if (r == 0 && daysSinceMon > 0) {
                break;
            }

            int targetDays = (r * 7 + c) - daysSinceMon;
            
            if (targetDays >= 0) { 
                if (p_Action == 1) { 
                    p_SelWeek = r; 
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    }
    case WM_CLOSE: DestroyWindow(hwnd); hPunishDlg = NULL; return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


// ======================= [ UI Tab 切换引擎 ] =======================
/* * 【重新排列的 Tab 顺序】 */
void SwitchTab(int index) {
    // 隐藏所有
    for (HWND h : g_TabHomeCtrls) ShowWindow(h, SW_HIDE);
    for (HWND h : g_TabTaskCtrls) ShowWindow(h, SW_HIDE);
    for (HWND h : g_TabOptCtrls)  ShowWindow(h, SW_HIDE);
    for (HWND h : g_TabPunishCtrls) ShowWindow(h, SW_HIDE);

    // 显示目标
    if (index == 0) for (HWND h : g_TabHomeCtrls) ShowWindow(h, SW_SHOW);
    if (index == 1) for (HWND h : g_TabTaskCtrls) ShowWindow(h, SW_SHOW);
    if (index == 2) for (HWND h : g_TabPunishCtrls) ShowWindow(h, SW_SHOW); // Tab 2: 惩罚系统
    if (index == 3) for (HWND h : g_TabOptCtrls)  ShowWindow(h, SW_SHOW);   // Tab 3: 系统设置
}

// ======================= [ 托盘菜单增强 ] =======================
void UpdateTrayMenu(HWND hwnd) {
    POINT pt; GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();

    // 【需求6】 托盘极简归类
    AppendMenuW(hMenu, MF_STRING, IDM_SHOW_MAIN, L"💻 主面板");
    AppendMenuW(hMenu, MF_STRING, IDM_SHOW_OPT, L"⚙️ 系统设置");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    
    AppendMenuW(hMenu, MF_STRING, IDM_OPEN_SEAT, L"🚨 打开全班座次表");
    AppendMenuW(hMenu, MF_STRING, IDM_GEN_REPORT, L"📊 一键生成惩罚报表");
    AppendMenuW(hMenu, MF_STRING, IDM_FORMAT_TXT, L"✂️ 智能排版剪贴板文本");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(hMenu, MF_STRING, IDM_TOGGLE_DUTY, IsWindowVisible(hDutyOverlay) ? L"👁️ 隐藏值日悬浮窗" : L"👁️ 显示值日悬浮窗");
    AppendMenuW(hMenu, MF_STRING, IDM_NEXT_GROUP, L"⏭️ 切下一组值班");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(hMenu, MF_STRING, IDM_LOCK_SCREEN, L"🔒 一键锁屏");
    AppendMenuW(hMenu, MF_STRING, IDM_RESTART_EXPLORER, L"🔄 重启资源管理器");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"ℹ️ 关于 v4.0.2.debug1");
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"❌ 彻底退出");

    SetMenuDefaultItem(hMenu, IDM_SHOW_MAIN, FALSE);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// ======================= [ 杂项辅助函数 ] =======================
DWORD WINAPI PlaySoundThread(LPVOID lpParam) {
    PlaySoundW(RING_AUDIO, NULL, SND_FILENAME | SND_NODEFAULT);
    return 0;
}
void ShowSilentShutdownBox() {
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32) {
        MSGBOXTIMEOUTW MsgBoxTimeout = (MSGBOXTIMEOUTW)GetProcAddress(hUser32, "MessageBoxTimeoutW");
        if (MsgBoxTimeout) {
            int result = MsgBoxTimeout(NULL, L"放学了，需要关机吗？\n\n（如不点击取消,将在60秒后自动关机）", L"某班电脑助手 - 关机", MB_OKCANCEL | MB_TOPMOST | MB_SETFOREGROUND, 0, TIMEOUT_MS);
            if (result == 32000 || result == IDOK) system("shutdown -s -f -t 0");
        }
        FreeLibrary(hUser32);
    }
}

// ======================= [ 主窗口消息处理中心 ] =======================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hMainWnd = hwnd;
        InitDutyFile();
        InitSeatFile(); 
        LoadDutyData();
        LoadSettings();
        LoadCustomTasks();

        g_CurrentDutyGroup = GetDefaultGroupForToday();
        SetTimer(hwnd, TIMER_ID, 1000, NULL);
        WriteLog(L"--- 某班电脑助手 (v4.0.2.debug1) 已启动 ---");

        hFontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        hFontBig = CreateFontW(19, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        hFontNormal = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");

        // ------------------ 构建多标签系统 (Tab Control) ------------------
        hTabControl = CreateWindowExW(0, WC_TABCONTROL, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 10, 10, 500, 540, hwnd, (HMENU)IDC_TAB, NULL, NULL);
        SendMessage(hTabControl, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

        // 【极简主义优化】：删减啰嗦文本，重新排序
        TCITEMW tie = { 0 };
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"  首页  "; TabCtrl_InsertItem(hTabControl, 0, &tie);
        tie.pszText = (LPWSTR)L"  定时播报  "; TabCtrl_InsertItem(hTabControl, 1, &tie);
        tie.pszText = (LPWSTR)L"  惩罚系统  "; TabCtrl_InsertItem(hTabControl, 2, &tie);
        tie.pszText = (LPWSTR)L"  系统设置  "; TabCtrl_InsertItem(hTabControl, 3, &tie);

        // UI 坐标调整，增加呼吸感
        int baseX = 25, baseY = 55; 
        
        // ------------------ [ Tab 0: 首页 ] ------------------
        HWND hGrpDuty = CreateWindowW(L"BUTTON", L" 今日值日 ", WS_CHILD | BS_GROUPBOX, baseX, baseY, 450, 150, hwnd, NULL, NULL, NULL);
        hDutyLabel = CreateWindowW(L"STATIC", L"🌟 当前值班：读取中...", WS_CHILD, baseX+20, baseY+35, 380, 25, hwnd, NULL, NULL, NULL);
        HWND hBtnDuty = CreateWindowW(L"BUTTON", L"更改名单", WS_CHILD, baseX+20, baseY+75, 120, 32, hwnd, (HMENU)IDC_BTN_DUTY, NULL, NULL);
        HWND hBtnToggle = CreateWindowW(L"BUTTON", L"悬浮窗起降", WS_CHILD, baseX+150, baseY+75, 120, 32, hwnd, (HMENU)IDM_TOGGLE_DUTY, NULL, NULL);
        HWND hBtnNext = CreateWindowW(L"BUTTON", L"切下一组", WS_CHILD, baseX+280, baseY+75, 120, 32, hwnd, (HMENU)IDC_BTN_NEXT_GROUP, NULL, NULL);
        HWND hTipDuty = CreateWindowW(L"STATIC", L"已于零点按星期自动匹配完毕。", WS_CHILD, baseX+20, baseY+120, 380, 20, hwnd, NULL, NULL, NULL);

        HWND hGrpTools = CreateWindowW(L"BUTTON", L" 效率工具 ", WS_CHILD | BS_GROUPBOX, baseX, baseY+170, 450, 160, hwnd, NULL, NULL, NULL);
        HWND hBtnFormat = CreateWindowW(L"BUTTON", L"智能排版剪贴板", WS_CHILD, baseX+20, baseY+205, 400, 35, hwnd, (HMENU)IDC_BTN_FORMAT_TXT, NULL, NULL);
        HWND hBtnLock = CreateWindowW(L"BUTTON", L"一键锁屏", WS_CHILD, baseX+20, baseY+260, 190, 32, hwnd, (HMENU)IDM_LOCK_SCREEN, NULL, NULL);
        HWND hBtnResEx = CreateWindowW(L"BUTTON", L"重启资源管理器", WS_CHILD, baseX+230, baseY+260, 190, 32, hwnd, (HMENU)IDM_RESTART_EXPLORER, NULL, NULL);
        
        HWND hLnkGitHub = CreateWindowW(L"STATIC", L"🔗 访问项目 GitHub", WS_CHILD | WS_VISIBLE | SS_NOTIFY, baseX+20, baseY+350, 300, 25, hwnd, (HMENU)IDC_LNK_GITHUB, NULL, NULL);

        g_TabHomeCtrls = { hGrpDuty, hDutyLabel, hBtnDuty, hBtnToggle, hBtnNext, hTipDuty, hGrpTools, hBtnFormat, hBtnLock, hBtnResEx, hLnkGitHub };

        // ------------------ [ Tab 1: 定时播报 ] ------------------
        HWND hGrpTask = CreateWindowW(L"BUTTON", L" 定时任务 ", WS_CHILD | BS_GROUPBOX, baseX, baseY, 450, 440, hwnd, NULL, NULL, NULL);
        
        hListView = CreateWindowW(WC_LISTVIEW, L"", WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL, baseX+15, baseY+30, 420, 280, hwnd, (HMENU)IDC_LIST_TASKS, NULL, NULL);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMNW lvc = { 0 }; lvc.mask = LVCF_TEXT | LVCF_WIDTH;
        lvc.pszText = (LPWSTR)L"时间"; lvc.cx = 60; ListView_InsertColumn(hListView, 0, &lvc);
        lvc.pszText = (LPWSTR)L"音频路径"; lvc.cx = 350; ListView_InsertColumn(hListView, 1, &lvc);
        
        HWND hLblAdd = CreateWindowW(L"STATIC", L"新增任务：", WS_CHILD, baseX+15, baseY+325, 80, 25, hwnd, NULL, NULL, NULL);
        hTimePicker = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_BORDER | DTS_TIMEFORMAT, baseX+95, baseY+320, 100, 28, hwnd, (HMENU)IDC_TIME_PICKER, NULL, NULL);
        hEditPath = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, baseX+15, baseY+355, 320, 28, hwnd, (HMENU)IDC_EDIT_FILEPATH, NULL, NULL);
        HWND hBtnBrowse = CreateWindowW(L"BUTTON", L"选择", WS_CHILD, baseX+345, baseY+355, 90, 28, hwnd, (HMENU)IDC_BTN_BROWSE, NULL, NULL);
        HWND hBtnAdd = CreateWindowW(L"BUTTON", L"添加任务", WS_CHILD, baseX+15, baseY+395, 200, 32, hwnd, (HMENU)IDC_BTN_ADD_TASK, NULL, NULL);
        HWND hBtnDel = CreateWindowW(L"BUTTON", L"删除选中", WS_CHILD, baseX+235, baseY+395, 200, 32, hwnd, (HMENU)IDC_BTN_DEL_TASK, NULL, NULL);

        g_TabTaskCtrls = { hGrpTask, hListView, hLblAdd, hTimePicker, hEditPath, hBtnBrowse, hBtnAdd, hBtnDel };

        // ------------------ [ Tab 2: 惩罚系统 ] ------------------
        HWND hGrpPunish = CreateWindowW(L"BUTTON", L" 惩罚管理 ", WS_CHILD | BS_GROUPBOX, baseX, baseY, 450, 220, hwnd, NULL, NULL, NULL);
        HWND hChkPunish = CreateWindowW(L"BUTTON", L"启用惩罚管理系统", WS_CHILD | BS_AUTOCHECKBOX, baseX+30, baseY+40, 380, 25, hwnd, (HMENU)IDC_CHK_PUNISH_ENABLE, NULL, NULL);
        HWND hBtnOpenPunish = CreateWindowW(L"BUTTON", L"全班座次表", WS_CHILD, baseX+60, baseY+85, 300, 45, hwnd, (HMENU)IDC_BTN_OPEN_PUNISH, NULL, NULL);
        HWND hBtnQueryPunish = CreateWindowW(L"BUTTON", L"生成惩罚报表", WS_CHILD, baseX+60, baseY+145, 300, 45, hwnd, (HMENU)IDC_BTN_QUERY_PUNISH, NULL, NULL);
        g_TabPunishCtrls = { hGrpPunish, hChkPunish, hBtnOpenPunish, hBtnQueryPunish };

        // ------------------ [ Tab 3: 选项 ] ------------------
        HWND hGrpPlan = CreateWindowW(L"BUTTON", L" 早读预案 ", WS_CHILD | BS_GROUPBOX, baseX, baseY, 450, 90, hwnd, NULL, NULL, NULL);
        hRdoPlanA = CreateWindowW(L"BUTTON", L"方案A: 语文先读", WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, baseX+40, baseY+35, 160, 25, hwnd, (HMENU)IDC_RDO_PLAN_A, NULL, NULL);
        hRdoPlanB = CreateWindowW(L"BUTTON", L"方案B: 英语先读", WS_CHILD | BS_AUTORADIOBUTTON, baseX+230, baseY+35, 160, 25, hwnd, (HMENU)IDC_RDO_PLAN_B, NULL, NULL);

        HWND hGrpSys = CreateWindowW(L"BUTTON", L" 基础系统管控 ", WS_CHILD | BS_GROUPBOX, baseX, baseY+110, 450, 220, hwnd, NULL, NULL, NULL);
        int sy = baseY + 145, gp = 40;
        hChkMorning = CreateWindowW(L"BUTTON", L"开启 06:01 早晨考勤静音铃", WS_CHILD | BS_AUTOCHECKBOX, baseX+30, sy, 380, 25, hwnd, (HMENU)IDC_CHK_MORNING, NULL, NULL);
        hChkNoon = CreateWindowW(L"BUTTON", L"开启 11:59 中午自动息屏关机", WS_CHILD | BS_AUTOCHECKBOX, baseX+30, sy+gp, 380, 25, hwnd, (HMENU)IDC_CHK_NOON, NULL, NULL);
        hChkEvening = CreateWindowW(L"BUTTON", L"开启 17:50 课外活动清场铃", WS_CHILD | BS_AUTOCHECKBOX, baseX+30, sy+gp*2, 380, 25, hwnd, (HMENU)IDC_CHK_EVENING, NULL, NULL);
        hChkNight = CreateWindowW(L"BUTTON", L"开启 21:38 晚上自动静默关机", WS_CHILD | BS_AUTOCHECKBOX, baseX+30, sy+gp*3, 380, 25, hwnd, (HMENU)IDC_CHK_NIGHT, NULL, NULL);

        g_TabOptCtrls = { hGrpPlan, hRdoPlanA, hRdoPlanB, hGrpSys, hChkMorning, hChkNoon, hChkEvening, hChkNight };

        // 统一设置字体
        auto SetFont = [](const std::vector<HWND>& v, HFONT f) { for (HWND h : v) SendMessage(h, WM_SETFONT, (WPARAM)f, TRUE); };
        SetFont(g_TabHomeCtrls, hFontNormal); SetFont(g_TabTaskCtrls, hFontNormal); SetFont(g_TabOptCtrls, hFontNormal); SetFont(g_TabPunishCtrls, hFontNormal);
        SendMessage(hDutyLabel, WM_SETFONT, (WPARAM)hFontBig, TRUE);
        SendMessage(hRdoPlanA, WM_SETFONT, (WPARAM)hFontBig, TRUE);
        SendMessage(hRdoPlanB, WM_SETFONT, (WPARAM)hFontBig, TRUE);
        SendMessage(hBtnOpenPunish, WM_SETFONT, (WPARAM)hFontBig, TRUE);
        SendMessage(hBtnQueryPunish, WM_SETFONT, (WPARAM)hFontBig, TRUE);

        SendMessage(hLnkGitHub, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

        // 初始化状态
        SendMessage(g_ReadingPlan == 0 ? hRdoPlanA : hRdoPlanB, BM_SETCHECK, BST_CHECKED, 0);
        HWND chks[] = { hChkMorning, hChkNoon, hChkEvening, hChkNight };
        DWORD states[] = { g_Morning, g_Noon, g_Evening, g_Night };
        for (int i = 0; i < 4; i++) SendMessage(chks[i], BM_SETCHECK, states[i] ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(hChkPunish, BM_SETCHECK, g_PunishEnable ? BST_CHECKED : BST_UNCHECKED, 0);

        UpdateDutyLabelUI();
        RefreshTaskListView();
        SwitchTab(0); // 默认显示首页
        break;
    }
    case WM_NOTIFY: {
        if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
            SwitchTab(TabCtrl_GetCurSel(hTabControl));
        }
        break;
    }
    case WM_TIMER: CheckTimeAndExecute(); break;
    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == IDC_LNK_GITHUB && HIWORD(wParam) == STN_CLICKED) {
            ShellExecuteW(NULL, L"open", L"https://github.com", NULL, NULL, SW_SHOW);
        }

        else if (id == IDM_TOGGLE_DUTY) {
            if (IsWindowVisible(hDutyOverlay)) {
                ShowWindow(hDutyOverlay, SW_HIDE);
                g_DutyWindowClosedByUser = true;
                g_UserOpenedManually = false;
            } else {
                LoadDutyData();
                int sw = GetSystemMetrics(SM_CXSCREEN);
                SetWindowPos(hDutyOverlay, HWND_TOPMOST, sw - 370, 50, 340, 520, SWP_SHOWWINDOW);
                g_DutyWindowClosedByUser = false;
                g_UserOpenedManually = true; 
            }
        }
        else if (id == IDC_BTN_NEXT_GROUP || id == IDM_NEXT_GROUP) {
            if (!g_DutyList.empty()) {
                g_CurrentDutyGroup = (g_CurrentDutyGroup + 1) % g_DutyList.size();
                SaveSettings();
                UpdateDutyLabelUI();
                if (hDutyOverlay && IsWindowVisible(hDutyOverlay)) InvalidateRect(hDutyOverlay, NULL, TRUE);
            }
        }
        else if (id == IDC_BTN_DUTY) { ShellExecuteW(NULL, L"open", L"notepad.exe", DUTY_FILE, NULL, SW_SHOW); }
        else if (id == IDC_BTN_FORMAT_TXT || id == IDM_FORMAT_TXT) { FormatClipboardTextTool(hwnd); } 
        else if (id == IDM_LOCK_SCREEN) { LockWorkStation(); }
        else if (id == IDM_RESTART_EXPLORER) { system("taskkill /f /im explorer.exe & start explorer.exe"); }
        
        else if (id == IDC_BTN_BROWSE) {
            OPENFILENAMEW ofn = { 0 };
            wchar_t szFile[260] = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"音频文件 (*.mp3;*.wav)\0*.mp3;*.wav\0所有文件 (*.*)\0*.*\0";
            if (GetOpenFileNameW(&ofn)) SetWindowTextW(hEditPath, szFile);
        }
        else if (id == IDC_BTN_ADD_TASK) {
            SYSTEMTIME st; SendMessage(hTimePicker, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
            wchar_t path[260]; GetWindowTextW(hEditPath, path, 260);
            if (wcslen(path) > 0) {
                CustomTask tk = { st.wHour, st.wMinute, path };
                g_Tasks.push_back(tk);
                SaveCustomTasks(); RefreshTaskListView();
                SetWindowTextW(hEditPath, L""); 
            } else MessageBoxW(hwnd, L"请先选择一个音频文件！", L"某班电脑助手", MB_OK);
        }
        else if (id == IDC_BTN_DEL_TASK) {
            int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (sel != -1) {
                g_Tasks.erase(g_Tasks.begin() + sel);
                SaveCustomTasks(); RefreshTaskListView();
            } else MessageBoxW(hwnd, L"请先在列表中选中要删除的任务。", L"某班电脑助手", MB_OK);
        }

        else if (id == IDC_BTN_OPEN_PUNISH || id == IDM_OPEN_SEAT) {
            if (!g_PunishEnable) {
                MessageBoxW(hwnd, L"惩罚系统未开启，请先启用。", L"某班电脑助手", MB_OK|MB_ICONWARNING);
                break;
            }
            if (hSeatingWnd) { SetForegroundWindow(hSeatingWnd); }
            else {
                hSeatingWnd = CreateWindowExW(WS_EX_APPWINDOW, L"SeatingChartClass", L"全班座次表", 
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, GetModuleHandle(NULL), NULL);
            }
        }
        else if (id == IDC_BTN_QUERY_PUNISH || id == IDM_GEN_REPORT) {
            if (!g_PunishEnable) {
                MessageBoxW(hwnd, L"惩罚系统未开启，请先启用。", L"某班电脑助手", MB_OK);
                break;
            }
            GenerateAndOpenPunishReport(hwnd); 
        }

        else if (HIWORD(wParam) == BN_CLICKED) {
            if (id == IDC_CHK_MORNING) { g_Morning = (SendMessage(hChkMorning, BM_GETCHECK, 0, 0) == BST_CHECKED); SaveSettings(); }
            if (id == IDC_CHK_NOON) { g_Noon = (SendMessage(hChkNoon, BM_GETCHECK, 0, 0) == BST_CHECKED); SaveSettings(); }
            if (id == IDC_CHK_EVENING) { g_Evening = (SendMessage(hChkEvening, BM_GETCHECK, 0, 0) == BST_CHECKED); SaveSettings(); }
            if (id == IDC_CHK_NIGHT) { g_Night = (SendMessage(hChkNight, BM_GETCHECK, 0, 0) == BST_CHECKED); SaveSettings(); }
            if (id == IDC_RDO_PLAN_A) { g_ReadingPlan = 0; SaveSettings(); }
            if (id == IDC_RDO_PLAN_B) { g_ReadingPlan = 1; SaveSettings(); }
            if (id == IDC_CHK_PUNISH_ENABLE) { g_PunishEnable = (SendMessage(GetDlgItem(hwnd,IDC_CHK_PUNISH_ENABLE), BM_GETCHECK,0,0) == BST_CHECKED); SaveSettings(); }
        }

        if (id == IDM_SHOW_MAIN) {
            ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd);
            TabCtrl_SetCurSel(hTabControl, 0); SwitchTab(0);
        }
        else if (id == IDM_SHOW_OPT) {
            ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd);
            TabCtrl_SetCurSel(hTabControl, 3); SwitchTab(3); // 指向正确的 Tab 3
        }
        // 【Bug 5 修复】：强制规范标题，去掉冗余文字
        else if (id == IDM_ABOUT) { MessageBoxW(hwnd, L"某班电脑助手 v4.0.2.debug1\n\n- UI深度极简重构与系统重排\n- 惩罚防乱点击与表单精准校验升级\n- 日历组件完整周限制突破\n\nby 城南", L"某班电脑助手", MB_OK | MB_ICONINFORMATION); }
        else if (id == IDM_EXIT) { DestroyWindow(hwnd); }
        break;
    }
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP) { 
            ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); 
            TabCtrl_SetCurSel(hTabControl, 0); SwitchTab(0);
        }
        else if (lParam == WM_RBUTTONUP) { UpdateTrayMenu(hwnd); }
        break;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == GetDlgItem(hwnd, IDC_LNK_GITHUB)) {
            SetTextColor((HDC)wParam, RGB(0, 102, 204));
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        Shell_NotifyIconW(NIM_DELETE, &nid);
        DeleteObject(hFontTitle); DeleteObject(hFontNormal); DeleteObject(hFontBig);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ======================= [ wWinMain 入口函数 ] =======================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Local\\yuan_Timing_Mutex_Unique_ID_v4.0.2"); 
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex != NULL) CloseHandle(hMutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    SetSystemVolumeTo30();

    HICON hIconLarge = (HICON)LoadImageW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    HICON hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc; wc.hInstance = hInstance; wc.lpszClassName = L"ControlTaskWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wc.hIcon = hIconLarge;
    RegisterClassW(&wc);

    WNDCLASSW wcOverlay = { };
    wcOverlay.style = CS_DBLCLKS; wcOverlay.lpfnWndProc = DutyOverlayProc;
    wcOverlay.hInstance = hInstance; wcOverlay.lpszClassName = L"DutyOverlayClass";
    wcOverlay.hCursor = LoadCursor(NULL, IDC_ARROW); wcOverlay.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wcOverlay);

    WNDCLASSW wcSeat = { };
    wcSeat.lpfnWndProc = SeatingWndProc; wcSeat.hInstance = hInstance; wcSeat.lpszClassName = L"SeatingChartClass";
    wcSeat.hCursor = LoadCursor(NULL, IDC_ARROW); wcSeat.hbrBackground = CreateSolidBrush(RGB(240, 245, 250));
    wcSeat.hIcon = hIconLarge;
    RegisterClassW(&wcSeat);

    WNDCLASSW wcPunish = { };
    wcPunish.lpfnWndProc = PunishDlgProc; wcPunish.hInstance = hInstance; wcPunish.lpszClassName = L"PunishDlgClass";
    wcPunish.hCursor = LoadCursor(NULL, IDC_ARROW); wcPunish.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wcPunish);

    hDutyOverlay = CreateWindowExW( WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"DutyOverlayClass", L"某班值日生", WS_POPUP, 0, 0, 340, 520, NULL, NULL, hInstance, NULL);
    SetLayeredWindowAttributes(hDutyOverlay, 0, 215, LWA_ALPHA);

    int screenW = GetSystemMetrics(SM_CXSCREEN), screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 535, winH = 610; 

    HWND hwnd = CreateWindowExW(0, L"ControlTaskWindow", L"某班电脑助手",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (screenW - winW) / 2, (screenH - winH) / 2, winW, winH, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) return 0;

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hIconSmall ? hIconSmall : hIconLarge;
    wcscpy_s(nid.szTip, L"某班电脑助手");
    Shell_NotifyIconW(NIM_ADD, &nid);

    ShowWindow(hwnd, SW_HIDE);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}