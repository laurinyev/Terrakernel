#ifndef GPXWM_H
#define GPXWM_H 1

#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <psf/psf.h>

#define MAX_WINDOWS 6
#define TILE_MARGIN 4
#define TITLE_HEIGHT 24

typedef struct {
    int x, y;
    int width, height;
} Rect;

typedef enum {
    KEY_UNKNOWN = 0,
    KEY_A = 4,
    KEY_B = 5,
    KEY_C = 6,
    KEY_D = 7,
    KEY_E = 8,
    KEY_F = 9,
    KEY_G = 10,
    KEY_H = 11,
    KEY_I = 12,
    KEY_J = 13,
    KEY_K = 14,
    KEY_L = 15,
    KEY_M = 16,
    KEY_N = 17,
    KEY_O = 18,
    KEY_P = 19,
    KEY_Q = 20,
    KEY_R = 21,
    KEY_S = 22,
    KEY_T = 23,
    KEY_U = 24,
    KEY_V = 25,
    KEY_W = 26,
    KEY_X = 27,
    KEY_Y = 28,
    KEY_Z = 29,
    KEY_1 = 30,
    KEY_2 = 31,
    KEY_3 = 32,
    KEY_4 = 33,
    KEY_5 = 34,
    KEY_6 = 35,
    KEY_7 = 36,
    KEY_8 = 37,
    KEY_9 = 38,
    KEY_0 = 39,
    KEY_ENTER = 40,
    KEY_ESC = 41,
    KEY_BACKSPACE = 42,
    KEY_TAB = 43,
    KEY_SPACE = 44,
    KEY_MINUS = 45,
    KEY_EQUAL = 46,
    KEY_LEFTBRACE = 47,
    KEY_RIGHTBRACE = 48,
    KEY_BACKSLASH = 49,
    KEY_NONUS_HASH = 50,
    KEY_SEMICOLON = 51,
    KEY_APOSTROPHE = 52,
    KEY_GRAVE = 53,
    KEY_COMMA = 54,
    KEY_DOT = 55,
    KEY_SLASH = 56,
    KEY_CAPSLOCK = 57,
    KEY_F1 = 58,
    KEY_F2 = 59,
    KEY_F3 = 60,
    KEY_F4 = 61,
    KEY_F5 = 62,
    KEY_F6 = 63,
    KEY_F7 = 64,
    KEY_F8 = 65,
    KEY_F9 = 66,
    KEY_F10 = 67,
    KEY_F11 = 68,
    KEY_F12 = 69,
    KEY_PRINTSCREEN = 70,
    KEY_SCROLLLOCK = 71,
    KEY_PAUSE = 72,
    KEY_INSERT = 73,
    KEY_HOME = 74,
    KEY_PAGEUP = 75,
    KEY_DELETE = 76,
    KEY_END = 77,
    KEY_PAGEDOWN = 78,
    KEY_RIGHT = 79,
    KEY_LEFT = 80,
    KEY_DOWN = 81,
    KEY_UP = 82,
    KEY_NUMLOCK = 83,
    KEY_KP_SLASH = 84,
    KEY_KP_ASTERISK = 85,
    KEY_KP_MINUS = 86,
    KEY_KP_PLUS = 87,
    KEY_KP_ENTER = 88,
    KEY_KP_1 = 89,
    KEY_KP_2 = 90,
    KEY_KP_3 = 91,
    KEY_KP_4 = 92,
    KEY_KP_5 = 93,
    KEY_KP_6 = 94,
    KEY_KP_7 = 95,
    KEY_KP_8 = 96,
    KEY_KP_9 = 97,
    KEY_KP_0 = 98,
    KEY_KP_DOT = 99,
    KEY_NONUS_BACKSLASH = 100,
    KEY_APPLICATION = 101,
    KEY_POWER = 102,
    KEY_KP_EQUAL = 103,
    KEY_F13 = 104,
    KEY_F14 = 105,
    KEY_F15 = 106,
    KEY_F16 = 107,
    KEY_F17 = 108,
    KEY_F18 = 109,
    KEY_F19 = 110,
    KEY_F20 = 111,
    KEY_F21 = 112,
    KEY_F22 = 113,
    KEY_F23 = 114,
    KEY_F24 = 115,
    KEY_EXECUTE = 116,
    KEY_HELP = 117,
    KEY_MENU = 118,
    KEY_SELECT = 119,
    KEY_STOP = 120,
    KEY_AGAIN = 121,
    KEY_UNDO = 122,
    KEY_CUT = 123,
    KEY_COPY = 124,
    KEY_PASTE = 125,
    KEY_FIND = 126,
    KEY_MUTE = 127,
    KEY_VOLUMEUP = 128,
    KEY_VOLUMEDOWN = 129,
    KEY_LOCKING_CAPSLOCK = 130,
    KEY_LOCKING_NUMLOCK = 131,
    KEY_LOCKING_SCROLLLOCK = 132,
    KEY_KP_COMMA = 133,
    KEY_KP_EQUAL_AS400 = 134,
    KEY_INTERNATIONAL1 = 135,
    KEY_INTERNATIONAL2 = 136,
    KEY_INTERNATIONAL3 = 137,
    KEY_INTERNATIONAL4 = 138,
    KEY_INTERNATIONAL5 = 139,
    KEY_INTERNATIONAL6 = 140,
    KEY_INTERNATIONAL7 = 141,
    KEY_INTERNATIONAL8 = 142,
    KEY_INTERNATIONAL9 = 143,
    KEY_LANG1 = 144,
    KEY_LANG2 = 145,
    KEY_LANG3 = 146,
    KEY_LANG4 = 147,
    KEY_LANG5 = 148,
    KEY_LANG6 = 149,
    KEY_LANG7 = 150,
    KEY_LANG8 = 151,
    KEY_LANG9 = 152,
    KEY_LEFTCTRL = 224,
    KEY_LEFTSHIFT = 225,
    KEY_LEFTALT = 226,
    KEY_LEFTMETA = 227,
    KEY_RIGHTCTRL = 228,
    KEY_RIGHTSHIFT = 229,
    KEY_RIGHTALT = 230,
    KEY_RIGHTMETA = 231
} EV_KEYCODE;

typedef struct {
    void (*on_key_down)(int keycode);
    void (*on_key_up)(int keycode);
    void (*on_key_press)(char c);
} WMEventTable;

typedef enum {
    WM_EVENT_KEY_DOWN,
    WM_EVENT_KEY_UP,
    WM_EVENT_KEY_PRESS
} WMEventType;

typedef struct {
    const char *title;
    uint32_t color;
    Rect rect;
    int tile_x, tile_y;
    void (*WMPutPx)(struct WMWindow *win, int x, int y, uint32_t color);
    WMEventTable* event_table;
} WMWindow;

typedef struct {
    struct {
        int xlen, ylen;
        int pitch;
        int bpp;
        size_t fblen;
        volatile uint32_t *fbbase;
        volatile uint32_t *fbbase1;
        volatile uint32_t *fbbase2;
    } dpy;

    WMWindow *windows[MAX_WINDOWS];
    int count;

    int red_shift;
    int green_shift;
    int blue_shift;
} WMManager;

extern WMManager g_WM_ins;
extern WMManager *g_WM;

uint32_t RGB(uint8_t r, uint8_t g, uint8_t b);
void PutPx(int x, int y, uint32_t color);
uint32_t GetPx(int x, int y);
void Flip(void);

void WMInit(struct limine_framebuffer *fb);
void WMPutPxWindow(WMWindow *win, int x, int y, uint32_t color);
WMWindow *WMCreateWindow(const char *title, uint32_t color);
void WMDrawAll(void);

#endif /* GPXWM_H */
