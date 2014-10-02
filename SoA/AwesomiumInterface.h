#if !defined(_AWESOMIUM_INTERFACE_H_)
#define _AWESOMIUM_INTERFACE_H_
#pragma once

#include <Awesomium/BitmapSurface.h>
#include <Awesomium/DataPak.h>
#include <Awesomium/DataSource.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/WebCore.h>
#include "Awesomium/WebKeyboardCodes.h"
#include <SDL/SDL_keycode.h>


#define mapWebKey(a, b) case Key_##a: return Awesomium::KeyCodes::AK_##b;

enum Key {
    Key_Unknown = SDL_SCANCODE_UNKNOWN,

    Key_A = SDL_SCANCODE_A,
    Key_B = SDL_SCANCODE_B,
    Key_C = SDL_SCANCODE_C,
    Key_D = SDL_SCANCODE_D,
    Key_E = SDL_SCANCODE_E,
    Key_F = SDL_SCANCODE_F,
    Key_G = SDL_SCANCODE_G,
    Key_H = SDL_SCANCODE_H,
    Key_I = SDL_SCANCODE_I,
    Key_J = SDL_SCANCODE_J,
    Key_K = SDL_SCANCODE_K,
    Key_L = SDL_SCANCODE_L,
    Key_M = SDL_SCANCODE_M,
    Key_N = SDL_SCANCODE_N,
    Key_O = SDL_SCANCODE_O,
    Key_P = SDL_SCANCODE_P,
    Key_Q = SDL_SCANCODE_Q,
    Key_R = SDL_SCANCODE_R,
    Key_S = SDL_SCANCODE_S,
    Key_T = SDL_SCANCODE_T,
    Key_U = SDL_SCANCODE_U,
    Key_V = SDL_SCANCODE_V,
    Key_W = SDL_SCANCODE_W,
    Key_X = SDL_SCANCODE_X,
    Key_Y = SDL_SCANCODE_Y,
    Key_Z = SDL_SCANCODE_Z,

    Key_1 = SDL_SCANCODE_1,
    Key_2 = SDL_SCANCODE_2,
    Key_3 = SDL_SCANCODE_3,
    Key_4 = SDL_SCANCODE_4,
    Key_5 = SDL_SCANCODE_5,
    Key_6 = SDL_SCANCODE_6,
    Key_7 = SDL_SCANCODE_7,
    Key_8 = SDL_SCANCODE_8,
    Key_9 = SDL_SCANCODE_9,
    Key_0 = SDL_SCANCODE_0,

    Key_Return = SDL_SCANCODE_RETURN,
    Key_Escape = SDL_SCANCODE_ESCAPE,
    Key_Backspace = SDL_SCANCODE_BACKSPACE,
    Key_Tab = SDL_SCANCODE_TAB,
    Key_Space = SDL_SCANCODE_SPACE,

    Key_Minus = SDL_SCANCODE_MINUS,
    Key_Equals = SDL_SCANCODE_EQUALS,
    Key_Leftbracket = SDL_SCANCODE_LEFTBRACKET,
    Key_Rightbracket = SDL_SCANCODE_RIGHTBRACKET,

    Key_Backslash = SDL_SCANCODE_BACKSLASH,
    /*
     *   Located at the lower left of the return
     *   key on ISO keyboards and at the right end
     *   of the QWERTY row on ANSI keyboards.
     *   Produces REVERSE SOLIDUS (backslash) and
     *   VERTICAL LINE in a US layout, REVERSE
     *   SOLIDUS and VERTICAL LINE in a UK Mac
     *   layout, NUMBER SIGN and TILDE in a UK
     *   Windows layout, DOLLAR SIGN and POUND SIGN
     *   in a Swiss German layout, NUMBER SIGN and
     *   APOSTROPHE in a German layout, GRAVE
     *   ACCENT and POUND SIGN in a French Mac
     *   layout, and ASTERISK and MICRO SIGN in a
     *   French Windows layout.
     */

    Key_Nonushash = SDL_SCANCODE_NONUSHASH,
    /*
     *   ISO USB keyboards actually use this code
     *   instead of 49 for the same key, but all
     *   OSes I've seen treat the two codes
     *   identically. So, as an implementor, unless
     *   your keyboard generates both of those
     *   codes and your OS treats them differently,
     *   you should generate SDL_SCANCODE_BACKSLASH
     *   instead of this code. As a user, you
     *   should not rely on this code because SDL
     *   will never generate it with most (all?)
     *   keyboards.
     */

    Key_Semicolon = SDL_SCANCODE_SEMICOLON,
    Key_Apostrophe = SDL_SCANCODE_APOSTROPHE,

    Key_Grave = SDL_SCANCODE_GRAVE,
    /*
     *   Located in the top left corner (on both ANSI
     *   and ISO keyboards). Produces GRAVE ACCENT and
     *   TILDE in a US Windows layout and in US and UK
     *   Mac layouts on ANSI keyboards, GRAVE ACCENT
     *   and NOT SIGN in a UK Windows layout, SECTION
     *   SIGN and PLUS-MINUS SIGN in US and UK Mac
     *   layouts on ISO keyboards, SECTION SIGN and
     *   DEGREE SIGN in a Swiss German layout (Mac:
     *   only on ISO keyboards), CIRCUMFLEX ACCENT and
     *   DEGREE SIGN in a German layout (Mac: only on
     *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
     *   French Windows layout, COMMERCIAL AT and
     *   NUMBER SIGN in a French Mac layout on ISO
     *   keyboards, and LESS-THAN SIGN and GREATER-THAN
     *   SIGN in a Swiss German, German, or French Mac
     *   layout on ANSI keyboards.
     */

    Key_Comma = SDL_SCANCODE_COMMA,
    Key_Period = SDL_SCANCODE_PERIOD,
    Key_Slash = SDL_SCANCODE_SLASH,

    Key_Capslock = SDL_SCANCODE_CAPSLOCK,

    Key_F1 = SDL_SCANCODE_F1,
    Key_F2 = SDL_SCANCODE_F2,
    Key_F3 = SDL_SCANCODE_F3,
    Key_F4 = SDL_SCANCODE_F4,
    Key_F5 = SDL_SCANCODE_F5,
    Key_F6 = SDL_SCANCODE_F6,
    Key_F7 = SDL_SCANCODE_F7,
    Key_F8 = SDL_SCANCODE_F8,
    Key_F9 = SDL_SCANCODE_F9,
    Key_F10 = SDL_SCANCODE_F10,
    Key_F11 = SDL_SCANCODE_F11,
    Key_F12 = SDL_SCANCODE_F12,

    Key_Printscreen = SDL_SCANCODE_PRINTSCREEN,
    Key_Scrolllock = SDL_SCANCODE_SCROLLLOCK,
    Key_Pause = SDL_SCANCODE_PAUSE,
    Key_Insert = SDL_SCANCODE_INSERT, // insert on PC, help on some Mac keyboards (but does send code Key_Insert, not Key_Help)
    Key_Home = SDL_SCANCODE_HOME,
    Key_Pageup = SDL_SCANCODE_PAGEUP,
    Key_Delete = SDL_SCANCODE_DELETE,
    Key_End = SDL_SCANCODE_END,
    Key_Pagedown = SDL_SCANCODE_PAGEDOWN,
    Key_Right = SDL_SCANCODE_RIGHT,
    Key_Left = SDL_SCANCODE_LEFT,
    Key_Down = SDL_SCANCODE_DOWN,
    Key_Up = SDL_SCANCODE_UP,

    Key_Numlockclear = SDL_SCANCODE_NUMLOCKCLEAR,
    Key_KpDivide = SDL_SCANCODE_KP_DIVIDE,
    Key_KpMultiply = SDL_SCANCODE_KP_MULTIPLY,
    Key_KpMinus = SDL_SCANCODE_KP_MINUS,
    Key_KpPlus = SDL_SCANCODE_KP_PLUS,
    Key_KpEnter = SDL_SCANCODE_KP_ENTER,
    Key_Kp1 = SDL_SCANCODE_KP_1,
    Key_Kp2 = SDL_SCANCODE_KP_2,
    Key_Kp3 = SDL_SCANCODE_KP_3,
    Key_Kp4 = SDL_SCANCODE_KP_4,
    Key_Kp5 = SDL_SCANCODE_KP_5,
    Key_Kp6 = SDL_SCANCODE_KP_6,
    Key_Kp7 = SDL_SCANCODE_KP_7,
    Key_Kp8 = SDL_SCANCODE_KP_8,
    Key_Kp9 = SDL_SCANCODE_KP_9,
    Key_Kp0 = SDL_SCANCODE_KP_0,
    Key_KpPeriod = SDL_SCANCODE_KP_PERIOD,

    Key_Nonusbackslash = SDL_SCANCODE_NONUSBACKSLASH,
    /*
     *   This is the additional key that ISO
     *   keyboards have over ANSI ones,
     *   located between left shift and Y.
     *   Produces GRAVE ACCENT and TILDE in a
     *   US or UK Mac layout, REVERSE SOLIDUS
     *   (backslash) and VERTICAL LINE in a
     *   US or UK Windows layout, and
     *   LESS-THAN SIGN and GREATER-THAN SIGN
     *   in a Swiss German, German, or French
     *   layout. */
    Key_Application = SDL_SCANCODE_APPLICATION, // windows contextual menu, compose

    Key_Power = SDL_SCANCODE_POWER,
    /*  The USB document says this is a status flag,
     *   not a physical key - but some Mac keyboards
     *   do have a power key. */

    Key_KpEquals = SDL_SCANCODE_KP_EQUALS,
    Key_F13 = SDL_SCANCODE_F13,
    Key_F14 = SDL_SCANCODE_F14,
    Key_F15 = SDL_SCANCODE_F15,
    Key_F16 = SDL_SCANCODE_F16,
    Key_F17 = SDL_SCANCODE_F17,
    Key_F18 = SDL_SCANCODE_F18,
    Key_F19 = SDL_SCANCODE_F19,
    Key_F20 = SDL_SCANCODE_F20,
    Key_F21 = SDL_SCANCODE_F21,
    Key_F22 = SDL_SCANCODE_F22,
    Key_F23 = SDL_SCANCODE_F23,
    Key_F24 = SDL_SCANCODE_F24,
    Key_Execute = SDL_SCANCODE_EXECUTE,
    Key_Help = SDL_SCANCODE_HELP,
    Key_Menu = SDL_SCANCODE_MENU,
    Key_Select = SDL_SCANCODE_SELECT,
    Key_Stop = SDL_SCANCODE_STOP,
    Key_Again = SDL_SCANCODE_AGAIN,
    Key_Undo = SDL_SCANCODE_UNDO,
    Key_Cut = SDL_SCANCODE_CUT,
    Key_Copy = SDL_SCANCODE_COPY,
    Key_Paste = SDL_SCANCODE_PASTE,
    Key_Find = SDL_SCANCODE_FIND,
    Key_Mute = SDL_SCANCODE_MUTE,
    Key_Volumeup = SDL_SCANCODE_VOLUMEUP,
    Key_Volumedown = SDL_SCANCODE_VOLUMEDOWN,
    Key_KpComma = SDL_SCANCODE_KP_COMMA,
    Key_KpEqualsas400 = SDL_SCANCODE_KP_EQUALSAS400,

    Key_International1 = SDL_SCANCODE_INTERNATIONAL1,
    Key_International2 = SDL_SCANCODE_INTERNATIONAL2,
    Key_International3 = SDL_SCANCODE_INTERNATIONAL3,
    Key_International4 = SDL_SCANCODE_INTERNATIONAL4,
    Key_International5 = SDL_SCANCODE_INTERNATIONAL5,
    Key_International6 = SDL_SCANCODE_INTERNATIONAL6,
    Key_International7 = SDL_SCANCODE_INTERNATIONAL7,
    Key_International8 = SDL_SCANCODE_INTERNATIONAL8,
    Key_International9 = SDL_SCANCODE_INTERNATIONAL9,
    Key_Lang1 = SDL_SCANCODE_LANG1,
    Key_Lang2 = SDL_SCANCODE_LANG2,
    Key_Lang3 = SDL_SCANCODE_LANG3,
    Key_Lang4 = SDL_SCANCODE_LANG4,
    Key_Lang5 = SDL_SCANCODE_LANG5,
    Key_Lang6 = SDL_SCANCODE_LANG6,
    Key_Lang7 = SDL_SCANCODE_LANG7,
    Key_Lang8 = SDL_SCANCODE_LANG8,
    Key_Lang9 = SDL_SCANCODE_LANG9,

    Key_Alterase = SDL_SCANCODE_ALTERASE,
    Key_Sysreq = SDL_SCANCODE_SYSREQ,
    Key_Cancel = SDL_SCANCODE_CANCEL,
    Key_Clear = SDL_SCANCODE_CLEAR,
    Key_Prior = SDL_SCANCODE_PRIOR,
    Key_Return2 = SDL_SCANCODE_RETURN2,
    Key_Separator = SDL_SCANCODE_SEPARATOR,
    Key_Out = SDL_SCANCODE_OUT,
    Key_Oper = SDL_SCANCODE_OPER,
    Key_Clearagain = SDL_SCANCODE_CLEARAGAIN,
    Key_Crsel = SDL_SCANCODE_CRSEL,
    Key_Exsel = SDL_SCANCODE_EXSEL,

    Key_Kp00 = SDL_SCANCODE_KP_00,
    Key_Kp000 = SDL_SCANCODE_KP_000,
    Key_Thousandsseparator = SDL_SCANCODE_THOUSANDSSEPARATOR,
    Key_Decimalseparator = SDL_SCANCODE_DECIMALSEPARATOR,
    Key_Currencyunit = SDL_SCANCODE_CURRENCYUNIT,
    Key_Currencysubunit = SDL_SCANCODE_CURRENCYSUBUNIT,
    Key_KpLeftparen = SDL_SCANCODE_KP_LEFTPAREN,
    Key_KpRightparen = SDL_SCANCODE_KP_RIGHTPAREN,
    Key_KpLeftbrace = SDL_SCANCODE_KP_LEFTBRACE,
    Key_KpRightbrace = SDL_SCANCODE_KP_RIGHTBRACE,
    Key_KpTab = SDL_SCANCODE_KP_TAB,
    Key_KpBackspace = SDL_SCANCODE_KP_BACKSPACE,
    Key_KpA = SDL_SCANCODE_KP_A,
    Key_KpB = SDL_SCANCODE_KP_B,
    Key_KpC = SDL_SCANCODE_KP_C,
    Key_KpD = SDL_SCANCODE_KP_D,
    Key_KpE = SDL_SCANCODE_KP_E,
    Key_KpF = SDL_SCANCODE_KP_F,
    Key_KpXor = SDL_SCANCODE_KP_XOR,
    Key_KpPower = SDL_SCANCODE_KP_POWER,
    Key_KpPercent = SDL_SCANCODE_KP_PERCENT,
    Key_KpLess = SDL_SCANCODE_KP_LESS,
    Key_KpGreater = SDL_SCANCODE_KP_GREATER,
    Key_KpAmpersand = SDL_SCANCODE_KP_AMPERSAND,
    Key_KpDblampersand = SDL_SCANCODE_KP_DBLAMPERSAND,
    Key_KpVerticalbar = SDL_SCANCODE_KP_VERTICALBAR,
    Key_KpDblverticalbar = SDL_SCANCODE_KP_DBLVERTICALBAR,
    Key_KpColon = SDL_SCANCODE_KP_COLON,
    Key_KpHash = SDL_SCANCODE_KP_HASH,
    Key_KpSpace = SDL_SCANCODE_KP_SPACE,
    Key_KpAt = SDL_SCANCODE_KP_AT,
    Key_KpExclam = SDL_SCANCODE_KP_EXCLAM,
    Key_KpMemstore = SDL_SCANCODE_KP_MEMSTORE,
    Key_KpMemrecall = SDL_SCANCODE_KP_MEMRECALL,
    Key_KpMemclear = SDL_SCANCODE_KP_MEMCLEAR,
    Key_KpMemadd = SDL_SCANCODE_KP_MEMADD,
    Key_KpMemsubtract = SDL_SCANCODE_KP_MEMSUBTRACT,
    Key_KpMemmultiply = SDL_SCANCODE_KP_MEMMULTIPLY,
    Key_KpMemdivide = SDL_SCANCODE_KP_MEMDIVIDE,
    Key_KpPlusminus = SDL_SCANCODE_KP_PLUSMINUS,
    Key_KpClear = SDL_SCANCODE_KP_CLEAR,
    Key_KpClearentry = SDL_SCANCODE_KP_CLEARENTRY,
    Key_KpBinary = SDL_SCANCODE_KP_BINARY,
    Key_KpOctal = SDL_SCANCODE_KP_OCTAL,
    Key_KpDecimal = SDL_SCANCODE_KP_DECIMAL,
    Key_KpHexadecimal = SDL_SCANCODE_KP_HEXADECIMAL,

    Key_Lctrl = SDL_SCANCODE_LCTRL,
    Key_Lshift = SDL_SCANCODE_LSHIFT,
    Key_Lalt = SDL_SCANCODE_LALT,
    Key_Lgui = SDL_SCANCODE_LGUI,
    Key_Rctrl = SDL_SCANCODE_RCTRL,
    Key_Rshift = SDL_SCANCODE_RSHIFT,
    Key_Ralt = SDL_SCANCODE_RALT,
    Key_Rgui = SDL_SCANCODE_RGUI,

    Key_Mode = SDL_SCANCODE_MODE,

    Key_Audionext = SDL_SCANCODE_AUDIONEXT,
    Key_Audioprev = SDL_SCANCODE_AUDIOPREV,
    Key_Audiostop = SDL_SCANCODE_AUDIOSTOP,
    Key_Audioplay = SDL_SCANCODE_AUDIOPLAY,
    Key_Audiomute = SDL_SCANCODE_AUDIOMUTE,
    Key_Mediaselect = SDL_SCANCODE_MEDIASELECT,
    Key_Www = SDL_SCANCODE_WWW,
    Key_Mail = SDL_SCANCODE_MAIL,
    Key_Calculator = SDL_SCANCODE_CALCULATOR,
    Key_Computer = SDL_SCANCODE_COMPUTER,
    Key_AcSearch = SDL_SCANCODE_AC_SEARCH,
    Key_AcHome = SDL_SCANCODE_AC_HOME,
    Key_AcBack = SDL_SCANCODE_AC_BACK,
    Key_AcForward = SDL_SCANCODE_AC_FORWARD,
    Key_AcStop = SDL_SCANCODE_AC_STOP,
    Key_AcRefresh = SDL_SCANCODE_AC_REFRESH,
    Key_AcBookmarks = SDL_SCANCODE_AC_BOOKMARKS,

    Key_Brightnessdown = SDL_SCANCODE_BRIGHTNESSDOWN,
    Key_Brightnessup = SDL_SCANCODE_BRIGHTNESSUP,
    Key_Displayswitch = SDL_SCANCODE_DISPLAYSWITCH,
    Key_Kbdillumtoggle = SDL_SCANCODE_KBDILLUMTOGGLE,
    Key_Kbdillumdown = SDL_SCANCODE_KBDILLUMDOWN,
    Key_Kbdillumup = SDL_SCANCODE_KBDILLUMUP,
    Key_Eject = SDL_SCANCODE_EJECT,
    Key_Sleep = SDL_SCANCODE_SLEEP,

    Key_App1 = SDL_SCANCODE_APP1,
    Key_App2 = SDL_SCANCODE_APP2,

    Key_MAX = SDL_NUM_SCANCODES,

    Key_Ctrl, // merged keys
    Key_Shift,
    Key_Alt
};


int getWebKeyFromKey(Key key);


class CustomJSMethodHandler : public Awesomium::JSMethodHandler
{
public:
    void OnMethodCall(Awesomium::WebView *caller, unsigned int remote_object_id, const Awesomium::WebString &method_name, const Awesomium::JSArray &args);
    Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView *caller, unsigned int remote_object_id, const Awesomium::WebString &method_name, const Awesomium::JSArray &args);

    Awesomium::JSObject *myObject;
};


class AwesomiumInterface {
public:
    AwesomiumInterface();
    ~AwesomiumInterface();

    bool init(const char *inputDir, int width, int height);

    void update();
    void draw();

    void setDrawCoords(int x, int y, int width, int height);
    void setColor(i32v4 color);

private:
    bool _isInitialized;

    int _width, _height;

    GLuint _renderedTexture;

    GLuint _vboID;
    GLuint _elementBufferID;

    ui8 _color[4];

    unsigned short _numFiles;

    Awesomium::DataSource* _data_source;
    Awesomium::WebSession* _webSession;
    Awesomium::WebCore* _webCore;
    Awesomium::WebView* _webView;

    Awesomium::JSValue _jsValue;

    CustomJSMethodHandler _methodHandler;

    struct Vertex2D {
        f32v2 pos;
        ui8 uv[2];
        ui8 pad[2];
        ui8 color[4];
    };
};

#endif  // _AWESOMIUM_INTERFACE_H_