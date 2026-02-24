#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#pragma comment(lib, "comctl32.lib")


#define ID_BTN_START    101
#define ID_BTN_STOP     102
#define ID_BTN_EXIT     103
#define ID_RB_RADAR     201
#define ID_RB_FULLSCREEN 202
#define ID_COMBO_AUDIO  203
#define ID_COMBO_POS    301
#define ID_COMBO_PRESET 302
#define ID_SLIDER_SENS  401
#define ID_SLIDER_OPACITY 402
#define ID_SLIDER_ENTITY_OPACITY 403
#define ID_SLIDER_ENTITIES 404
#define ID_SLIDER_POLLRATE 405
#define ID_SLIDER_SEPARATION 406
#define ID_SLIDER_RANGE 407
#define ID_EDIT_HWPORT  408
#define ID_LABEL_STATUS 501

static PROCESS_INFORMATION overlay_proc = {0};
static HWND hStatus = NULL;

static void save_settings(HWND hwnd) {
    FILE *f = fopen("settings_win.cfg", "w");
    if (!f) return;
    fprintf(f, "pos=%d\n", (int)SendDlgItemMessage(hwnd, ID_COMBO_POS, CB_GETCURSEL, 0, 0));
    fprintf(f, "opacity=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_OPACITY, TBM_GETPOS, 0, 0));
    fprintf(f, "sens=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_SENS, TBM_GETPOS, 0, 0));
    fprintf(f, "poll=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_POLLRATE, TBM_GETPOS, 0, 0));
    fprintf(f, "dot_opacity=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_ENTITY_OPACITY, TBM_GETPOS, 0, 0));
    fprintf(f, "max_entities=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_ENTITIES, TBM_GETPOS, 0, 0));
    fprintf(f, "separation=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_SEPARATION, TBM_GETPOS, 0, 0));
    fprintf(f, "range=%d\n", (int)SendDlgItemMessage(hwnd, ID_SLIDER_RANGE, TBM_GETPOS, 0, 0));
    fprintf(f, "preset=%d\n", (int)SendDlgItemMessage(hwnd, ID_COMBO_PRESET, CB_GETCURSEL, 0, 0));
    fprintf(f, "audio_cfg=%d\n", (int)SendDlgItemMessage(hwnd, ID_COMBO_AUDIO, CB_GETCURSEL, 0, 0));
    fprintf(f, "fullscreen=%d\n", (IsDlgButtonChecked(hwnd, ID_RB_FULLSCREEN) == BST_CHECKED));
    char hw[64];
    GetDlgItemTextA(hwnd, ID_EDIT_HWPORT, hw, sizeof(hw));
    fprintf(f, "hw_port=%s\n", hw);
    fclose(f);
}

static void load_settings(HWND hwnd) {
    FILE *f = fopen("settings_win.cfg", "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int val;
        if (sscanf(line, "pos=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_COMBO_POS, CB_SETCURSEL, val, 0);
        else if (sscanf(line, "opacity=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_OPACITY, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "sens=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_SENS, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "poll=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_POLLRATE, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "dot_opacity=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_ENTITY_OPACITY, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "max_entities=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_ENTITIES, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "separation=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_SEPARATION, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "range=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_SLIDER_RANGE, TBM_SETPOS, TRUE, val);
        else if (sscanf(line, "preset=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_COMBO_PRESET, CB_SETCURSEL, val, 0);
        else if (sscanf(line, "audio_cfg=%d", &val) == 1) SendDlgItemMessage(hwnd, ID_COMBO_AUDIO, CB_SETCURSEL, val, 0);
        else if (sscanf(line, "fullscreen=%d", &val) == 1) {
            CheckDlgButton(hwnd, ID_RB_RADAR, val ? BST_UNCHECKED : BST_CHECKED);
            CheckDlgButton(hwnd, ID_RB_FULLSCREEN, val ? BST_CHECKED : BST_UNCHECKED);
        }
        else if (strncmp(line, "hw_port=", 8) == 0) {
            char *p = line + 8;
            p[strcspn(p, "\r\n")] = 0;
            SetDlgItemTextA(hwnd, ID_EDIT_HWPORT, p);
        }
    }
    fclose(f);
}

static void LaunchOverlay(HWND hwnd) {
    if (overlay_proc.hProcess) return; 

    
    BOOL is_fullscreen = (IsDlgButtonChecked(hwnd, ID_RB_FULLSCREEN) == BST_CHECKED);
    int pos_idx = (int)SendDlgItemMessage(hwnd, ID_COMBO_POS, CB_GETCURSEL, 0, 0);
    if (pos_idx < 0) pos_idx = 0;
    int sens = (int)SendDlgItemMessage(hwnd, ID_SLIDER_SENS, TBM_GETPOS, 0, 0);
    int opacity = (int)SendDlgItemMessage(hwnd, ID_SLIDER_OPACITY, TBM_GETPOS, 0, 0);
    int pollrate = (int)SendDlgItemMessage(hwnd, ID_SLIDER_POLLRATE, TBM_GETPOS, 0, 0);
    int dot_opacity = (int)SendDlgItemMessage(hwnd, ID_SLIDER_ENTITY_OPACITY, TBM_GETPOS, 0, 0);
    int max_entities = (int)SendDlgItemMessage(hwnd, ID_SLIDER_ENTITIES, TBM_GETPOS, 0, 0);
    int separation = (int)SendDlgItemMessage(hwnd, ID_SLIDER_SEPARATION, TBM_GETPOS, 0, 0);
    int range_val = (int)SendDlgItemMessage(hwnd, ID_SLIDER_RANGE, TBM_GETPOS, 0, 0);

    int audio_idx = (int)SendDlgItemMessage(hwnd, ID_COMBO_AUDIO, CB_GETCURSEL, 0, 0);
    int ch_count = 2;
    if (audio_idx == 1) ch_count = 6;
    else if (audio_idx == 2) ch_count = 8;

    save_settings(hwnd);

    int preset_idx = (int)SendDlgItemMessage(hwnd, ID_COMBO_PRESET, CB_GETCURSEL, 0, 0);
    const char* preset_str = (preset_idx == 1) ? "pubg" : "none";

    char hw_port[64];
    GetDlgItemTextA(hwnd, ID_EDIT_HWPORT, hw_port, sizeof(hw_port));
    char hw_arg[128] = "";
    if (strlen(hw_port) > 0) {
        snprintf(hw_arg, sizeof(hw_arg), " --hw-port=%s", hw_port);
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ODC-overlay-win.exe --pos=%d --opacity=%d --sensitivity=%d --pollrate=%d --dot-opacity=%d --max-entities=%d --separation=%d --range=%d --channels=%d --preset=%s%s%s",
             pos_idx, opacity, sens, pollrate, dot_opacity, max_entities, separation, range_val, ch_count, preset_str, is_fullscreen ? " --fullscreen" : "", hw_arg);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        overlay_proc = pi;
        SetWindowTextA(hStatus, "Engine: Running");
    } else { 
        snprintf(cmd, sizeof(cmd), ".\\ODC-overlay-win.exe --pos=%d --opacity=%d --sensitivity=%d --pollrate=%d --dot-opacity=%d --max-entities=%d --separation=%d --range=%d --channels=%d --preset=%s%s%s",
                 pos_idx, opacity, sens, pollrate, dot_opacity, max_entities, separation, range_val, ch_count, preset_str, is_fullscreen ? " --fullscreen" : "", hw_arg);
        if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            overlay_proc = pi;
            SetWindowTextA(hStatus, "Engine: Running");
        } else {
            SetWindowTextA(hStatus, "Engine: Failed to start");
        }
    }
}

static void StopOverlay() {
    if (overlay_proc.hProcess) {
        TerminateProcess(overlay_proc.hProcess, 0);
        CloseHandle(overlay_proc.hProcess);
        CloseHandle(overlay_proc.hThread);
        memset(&overlay_proc, 0, sizeof(overlay_proc));
    }
    if (hStatus) SetWindowTextA(hStatus, "Engine: Stopped");
}

static HWND CreateLabel(HWND parent, const char* text, int x, int y, int w, int h) {
    return CreateWindowA("STATIC", text, WS_VISIBLE | WS_CHILD, x, y, w, h, parent, NULL,
                         (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), NULL);
}

static HWND CreateTrackbar(HWND parent, int id, int x, int y, int w, int h, int min_val, int max_val, int default_val) {
    HWND tb = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS,
                            x, y, w, h, parent, (HMENU)(intptr_t)id,
                            (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), NULL);
    SendMessage(tb, TBM_SETRANGE, TRUE, MAKELPARAM(min_val, max_val));
    SendMessage(tb, TBM_SETPOS, TRUE, default_val);
    return tb;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_START: LaunchOverlay(hwnd); break;
                case ID_BTN_STOP: StopOverlay(); break;
                case ID_BTN_EXIT: StopOverlay(); PostQuitMessage(0); break;
            }
            break;
        case WM_DESTROY:
            StopOverlay();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance; (void)pCmdLine;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    const wchar_t CLASS_NAME[] = L"ODCoreWindow";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"oneDirectionCore",
                                WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT, 440, 680,
                                NULL, NULL, hInstance, NULL);
    if (!hwnd) return 0;

    int y = 15;
    int lx = 20;   
    int cx = 20;   
    int cw = 380;  

    
    HWND hTitle = CreateLabel(hwnd, "oneDirectionCore", lx, y, cw, 24);
    HFONT hBoldFont = CreateFontA(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, "Segoe UI");
    SendMessage(hTitle, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
    y += 26;
    CreateLabel(hwnd, "Spatial Audio Radar - v0.1", lx, y, cw, 16);
    y += 28;

    HFONT hHintFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Segoe UI");

    
    CreateLabel(hwnd, "Engine", lx, y, cw, 16); y += 20;

    CreateWindowA("BUTTON", "Start", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  cx, y, 80, 28, hwnd, (HMENU)ID_BTN_START, hInstance, NULL);
    CreateWindowA("BUTTON", "Stop", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  cx + 90, y, 80, 28, hwnd, (HMENU)ID_BTN_STOP, hInstance, NULL);
    y += 34;

    hStatus = CreateLabel(hwnd, "Engine: Stopped", lx, y, cw, 16);
    y += 26;

    CreateLabel(hwnd, "Overlay Polling Rate (Hz)", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_POLLRATE, cx, y, cw, 28, 30, 240, 144); y += 28;
    HWND hHintPoll = CreateLabel(hwnd, "Radar update speed. Higher = smoother animation.", lx, y, cw, 14);
    SendMessage(hHintPoll, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Audio Configuration (Input)", lx, y, cw, 16); y += 18;
    HWND hAudioCombo = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                      cx, y, cw, 200, hwnd, (HMENU)ID_COMBO_AUDIO, hInstance, NULL);
    SendMessageA(hAudioCombo, CB_ADDSTRING, 0, (LPARAM)"Stereo (2.0)");
    SendMessageA(hAudioCombo, CB_ADDSTRING, 0, (LPARAM)"Surround (5.1)");
    SendMessageA(hAudioCombo, CB_ADDSTRING, 0, (LPARAM)"Surround (7.1)");
    SendMessageA(hAudioCombo, CB_SETCURSEL, 2, 0); 
    y += 22;
    HWND hHintAudio = CreateLabel(hwnd, "Match this to your system's speaker settings.", lx, y, cw, 14);
    SendMessage(hHintAudio, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Sound Classifier Preset", lx, y, cw, 16); y += 18;
    HWND hPresetCombo = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                      cx, y, cw, 200, hwnd, (HMENU)ID_COMBO_PRESET, hInstance, NULL);
    SendMessageA(hPresetCombo, CB_ADDSTRING, 0, (LPARAM)"None (Raw Energy)");
    SendMessageA(hPresetCombo, CB_ADDSTRING, 0, (LPARAM)"PUBG (Experimental)");
    SendMessageA(hPresetCombo, CB_SETCURSEL, 0, 0); y += 22;
    HWND hHintPreset = CreateLabel(hwnd, "Pattern matching for specific games like PUBG.", lx, y, cw, 14);
    SendMessage(hHintPreset, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Sensitivity", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_SENS, cx, y, cw, 28, 0, 100, 70); y += 28;
    HWND hHintSens = CreateLabel(hwnd, "Detection threshold. Higher = picks up quieter sounds.", lx, y, cw, 14);
    SendMessage(hHintSens, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Wavelength Separation", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_SEPARATION, cx, y, cw, 28, 0, 100, 50); y += 28;
    HWND hHintSep = CreateLabel(hwnd, "Angular sensitivity. Higher = more distinct dots.", lx, y, cw, 14);
    SendMessage(hHintSep, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Radar Range (Zoom)", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_RANGE, cx, y, cw, 28, 0, 100, 100); y += 28;
    HWND hHintRange = CreateLabel(hwnd, "Scales blip distance. Higher = zoom out.", lx, y, cw, 14);
    SendMessage(hHintRange, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 24;

    
    CreateLabel(hwnd, "Display", lx, y, cw, 16); y += 20;

    CreateWindowA("BUTTON", "Gaming Radar (Mini-Map)", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                  cx, y, cw, 20, hwnd, (HMENU)ID_RB_RADAR, hInstance, NULL);
    CheckDlgButton(hwnd, ID_RB_RADAR, BST_CHECKED);
    y += 22;

    CreateWindowA("BUTTON", "Full Screen Overlay", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                  cx, y, cw, 20, hwnd, (HMENU)ID_RB_FULLSCREEN, hInstance, NULL);
    y += 26;

    CreateLabel(hwnd, "Radar Position", lx, y, cw, 16); y += 18;
    HWND hCombo = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                cx, y, cw, 200, hwnd, (HMENU)ID_COMBO_POS, hInstance, NULL);
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Top-Left");
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Top-Center");
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Top-Right");
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Bottom-Left");
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Bottom-Center");
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"Bottom-Right");
    SendMessageA(hCombo, CB_SETCURSEL, 2, 0); 
    y += 30;

    CreateLabel(hwnd, "OSD Opacity", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_OPACITY, cx, y, cw, 28, 0, 100, 80); y += 28;
    HWND hHintOpa = CreateLabel(hwnd, "Transparency of the radar background.", lx, y, cw, 14);
    SendMessage(hHintOpa, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Entity Opacity (Dots)", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_ENTITY_OPACITY, cx, y, cw, 28, 10, 100, 100); y += 28;
    HWND hHintDotOpa = CreateLabel(hwnd, "Transparency of the blip icons.", lx, y, cw, 14);
    SendMessage(hHintDotOpa, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;

    CreateLabel(hwnd, "Max Entities (1-10)", lx, y, cw, 16); y += 18;
    CreateTrackbar(hwnd, ID_SLIDER_ENTITIES, cx, y, cw, 28, 1, 10, 6); y += 28;
    HWND hHintEnt = CreateLabel(hwnd, "Limits how many sounds are shown at once.", lx, y, cw, 14);
    SendMessage(hHintEnt, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 20;
    y += 20;

    CreateLabel(hwnd, "Hardware", lx, y, cw, 16); y += 20;
    CreateLabel(hwnd, "Serial Port (e.g. COM3)", lx, y, cw, 16); y += 18;
    HWND hEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                               cx, y, cw, 22, hwnd, (HMENU)ID_EDIT_HWPORT, NULL, NULL);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    y += 24;
    HWND hHintHW = CreateLabel(hwnd, "Device port for external LED/Haptic hardware.", lx, y, cw, 14);
    SendMessage(hHintHW, WM_SETFONT, (WPARAM)hHintFont, TRUE);
    y += 30;

    CreateWindowA("BUTTON", "Exit App", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                  cx + cw - 80, y, 80, 28, hwnd, (HMENU)ID_BTN_EXIT, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    load_settings(hwnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
