

#include "stdafx.h"

#include "AwesomiumInterface.h"

#include "Errors.h"

#ifndef AWESOMIUMINTERFACE_CPP_
#define AWESOMIUMINTERFACE_CPP_

namespace vorb {
namespace ui {

#define DEFAULT_WEB_URL "asset://UI/"

template <class C>
AwesomiumInterface<C>::AwesomiumInterface() :
    _isInitialized(false),
    _openglSurfaceFactory(nullptr),
    _renderedTexture(0),
    _width(0),
    _height(0),
    _vbo(0),
    _ibo(0),
    _color(255, 255, 255, 255) {
    // Empty
}

template <class C>
AwesomiumInterface<C>::~AwesomiumInterface(void) {
    destroy();
}

//Initializes the interface. Returns false on failure
template <class C>
bool AwesomiumInterface<C>::init(const char* inputDir, const char* sessionName, const char* indexName, ui32 width, ui32 height, C* api, IGameScreen* ownerScreen)
{
    if (_isInitialized) {
        pError("Awesomium Error: Tried to call AwesomiumInterface::init twice without destroying.");
        return false;
    }
    // Set dimensions
    _width = width;
    _height = height;

    // Set the API
    _awesomiumAPI = api;

    // Set default draw rectangle
    i32v4 destRect(0, 0, _width, _height);
    setDrawRect(destRect);

    // Sets up the webCore, which is the main process
    _webCore = Awesomium::WebCore::instance();
    if (_webCore == nullptr){
        _webCore = Awesomium::WebCore::Initialize(Awesomium::WebConfig());
    }

    //Set up our custom surface factory for direct opengl rendering
    _openglSurfaceFactory = new Awesomium::OpenglSurfaceFactory;
    _webCore->set_surface_factory(_openglSurfaceFactory);

    //Sets up the session
    _webSession = _webCore->CreateWebSession(Awesomium::WSLit(sessionName), Awesomium::WebPreferences());
    _webView = _webCore->CreateWebView((i32)_width, (i32)_height, _webSession, Awesomium::kWebViewType_Offscreen);

    if (!_webView){
        pError("Awesomium WebView could not be created!\n");
        return false;
    }

    nString pakName = nString(sessionName) + ".pak";

    if (!Awesomium::WriteDataPak(Awesomium::WSLit(pakName.c_str()), Awesomium::WSLit(inputDir), Awesomium::WSLit(""), _numFiles)){
        pError("UI Initialization Error: Failed to write " + pakName);
        return false;
    }
    _data_source = new Awesomium::DataPakSource(Awesomium::WSLit(pakName.c_str()));
    _webSession->AddDataSource(Awesomium::WSLit("UI"), _data_source);

    // Load a certain URL into our WebView instance.
    Awesomium::WebURL url(Awesomium::WSLit((DEFAULT_WEB_URL + nString(indexName)).c_str()));

    if (!url.IsValid()){
        pError("UI Initialization Error: URL was unable to be parsed.");
        return false;
    }

    // Set up the Game interface
    _gameInterface = _webView->CreateGlobalJavascriptObject(Awesomium::WSLit("App"));
    if (_gameInterface.IsObject()){
        _methodHandler.gameInterface = &_gameInterface.ToObject();

        //Initialize the callback API
        _awesomiumAPI->init(_methodHandler.gameInterface, ownerScreen);
    } else {
        pError("Awesomium Error: Failed to create app object.");
    }

    _webView->LoadURL(url);

    // Wait for our WebView to finish loading
    while (_webView->IsLoading()) _webCore->Update();

    // Sleep a bit and update once more to give scripts and plugins
    // on the page a chance to finish loading.
    SLEEP(30);
    _webCore->Update();
    _webView->SetTransparent(true);
    _webView->set_js_method_handler(&_methodHandler);

    // Set the callback API
    _methodHandler.setAPI(_awesomiumAPI);

    Awesomium::Error error = _webView->last_error();
    if (error) {
        pError("Awesomium Error: " + std::to_string(error));
    }

    // Retrieve the global 'window' object from the page
    _window = _webView->ExecuteJavascriptWithResult(Awesomium::WSLit("window"), Awesomium::WSLit(""));
    if (!_window.IsObject()) {
        pError("Awesomium Error: No window object.");
    }

    _isInitialized = true;
    return true;
}

template <class C>
void AwesomiumInterface<C>::destroy() {
    _webSession->Release();
    _webView->Destroy();
    delete _openglSurfaceFactory;
    delete _data_source;
    _isInitialized = false;
}

template <class C>
void AwesomiumInterface<C>::invokeFunction(const cString functionName, const Awesomium::JSArray& args) {
    _window.ToObject().Invoke(Awesomium::WSLit(functionName), args);
}

template <class C>
void AwesomiumInterface<C>::handleEvent(const SDL_Event& evnt) {
    float relX, relY;
    char* keyIdentifier;
    Awesomium::WebKeyboardEvent keyEvent;

    switch (evnt.type) {
        case SDL_MOUSEMOTION:
            relX = (evnt.motion.x - _drawRect.x) / (float)_drawRect.z;
            relY = (evnt.motion.y - _drawRect.y) / (float)_drawRect.w;

            _webView->Focus();
            _webView->InjectMouseMove(relX * _width, relY * _height);
            break;
        case SDL_MOUSEBUTTONDOWN:
            _webView->Focus();
            _webView->InjectMouseDown(getAwesomiumButtonFromSDL(evnt.button.button));
            break;
        case SDL_MOUSEBUTTONUP:
            _webView->Focus();
            _webView->InjectMouseUp(getAwesomiumButtonFromSDL(evnt.button.button));
            break;
        case SDL_MOUSEWHEEL:
            _webView->Focus();
            _webView->InjectMouseWheel(evnt.motion.y, evnt.motion.x);
            break;
        case SDL_TEXTINPUT:
        case SDL_KEYDOWN:
        case SDL_KEYUP:

            //Have to construct a webKeyboardEvent from the SDL Event
            keyIdentifier = keyEvent.key_identifier;
            keyEvent.virtual_key_code = getWebKeyFromSDLKey(evnt.key.keysym.scancode);
            Awesomium::GetKeyIdentifierFromVirtualKeyCode(keyEvent.virtual_key_code,
                                                          &keyIdentifier);

            // Apply modifiers
            keyEvent.modifiers = 0;
            if (evnt.key.keysym.mod & KMOD_LALT || evnt.key.keysym.mod & KMOD_RALT)
                keyEvent.modifiers |= Awesomium::WebKeyboardEvent::kModAltKey;
            if (evnt.key.keysym.mod & KMOD_LCTRL || evnt.key.keysym.mod & KMOD_RCTRL)
                keyEvent.modifiers |= Awesomium::WebKeyboardEvent::kModControlKey;
            if (evnt.key.keysym.mod & KMOD_LSHIFT || evnt.key.keysym.mod & KMOD_RSHIFT)
                keyEvent.modifiers |= Awesomium::WebKeyboardEvent::kModShiftKey;
            if (evnt.key.keysym.mod & KMOD_NUM)
                keyEvent.modifiers |= Awesomium::WebKeyboardEvent::kModIsKeypad;

            keyEvent.native_key_code = evnt.key.keysym.scancode;

            // Inject the event
            if (evnt.type == SDL_KEYDOWN) {
                keyEvent.type = Awesomium::WebKeyboardEvent::kTypeKeyDown;
                _webView->InjectKeyboardEvent(keyEvent);
            } else if (evnt.type == SDL_KEYUP) {
                keyEvent.type = Awesomium::WebKeyboardEvent::kTypeKeyUp;
                _webView->InjectKeyboardEvent(keyEvent);
            } else if (evnt.type == SDL_TEXTINPUT) {

                strcpy((char*)keyEvent.text, evnt.text.text);
                strcpy((char*)keyEvent.unmodified_text, evnt.text.text);

                keyEvent.type = Awesomium::WebKeyboardEvent::kTypeChar;
                _webView->InjectKeyboardEvent(keyEvent);
            }
            break;
        default:
            break;
    }
}

template <class C>
void AwesomiumInterface<C>::update()
{
    _webCore->Update();

    Awesomium::OpenglSurface* surface = (Awesomium::OpenglSurface*)_webView->surface();

    _renderedTexture = surface->getTextureID();

    if (!surface){
        pError("User Interface Error: webView->surface() returned null! Most likely due to erroneous code in the javascript or HTML5.\n");
    }
}

template <class C>
void AwesomiumInterface<C>::draw(vg::GLProgram* program) const
{
    //Check if draw coords were set
    if (_vbo == 0 || _renderedTexture == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

    // Bind shader
    program->use();
    program->enableVertexAttribArrays();

    glUniform1f(program->getUniform("xdim"), _width);
    glUniform1f(program->getUniform("ydim"), _height);

    glUniform1i(program->getUniform("roundMaskTexture"), 1);
    glUniform1f(program->getUniform("isRound"), 0.0f);

    glUniform1f(program->getUniform("xmod"), (GLfloat)0.0f);
    glUniform1f(program->getUniform("ymod"), (GLfloat)0.0f);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _renderedTexture);

    glUniform1i(program->getUniform("myTextureSampler"), 0);

    glVertexAttribPointer(program->getAttribute("vertexPosition_screenspace"), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)0);
    glVertexAttribPointer(program->getAttribute("vertexUV"), 2, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex2D), (void*)8);
    glVertexAttribPointer(program->getAttribute("vertexColor"), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex2D), (void*)12);

    // Draw call
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    program->disableVertexAttribArrays();
    program->use();
}

template <class C>
void AwesomiumInterface<C>::setDrawRect(const i32v4& rect) {

    const GLubyte uiBoxUVs[8] = { 0, 0, 0, 255, 255, 255, 255, 0 };

    _drawRect = rect;

    if (_vbo == 0) {
        glGenBuffers(1, &_vbo);
        glGenBuffers(1, &_ibo);

        GLushort elements[6] = { 0, 1, 2, 2, 3, 0 };
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    Vertex2D vertices[4];

    vertices[0].pos.x = _drawRect.x;
    vertices[0].pos.y = _drawRect.y + _drawRect.w;

    vertices[1].pos.x = _drawRect.x;
    vertices[1].pos.y = _drawRect.y;

    vertices[2].pos.x = _drawRect.x + _drawRect.z;
    vertices[2].pos.y = _drawRect.y;

    vertices[3].pos.x = _drawRect.x + _drawRect.z;
    vertices[3].pos.y = _drawRect.y + _drawRect.w;

    for (int i = 0; i < 4; i++) {
        vertices[i].uv[0] = uiBoxUVs[i * 2];
        vertices[i].uv[1] = uiBoxUVs[i * 2 + 1];

        vertices[i].color = _color;
    }

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template <class C>
void AwesomiumInterface<C>::setColor(const ColorRGBA8& color) {
    _color = color;
    // Update the vbo
    setDrawRect(_drawRect);
}

template <class C>
void CustomJSMethodHandler<C>::OnMethodCall(Awesomium::WebView *caller, unsigned int remote_object_id, const Awesomium::WebString &method_name, const Awesomium::JSArray &args) {

    std::cout << "Method call: " << method_name << std::endl;
    IAwesomiumAPI<C>::setptr funcptr = _api->getVoidFunction(Awesomium::ToString(method_name));
    if (funcptr) {
        ((*_api).*funcptr)(args);
    }
}

template <class C>
Awesomium::JSValue CustomJSMethodHandler<C>::OnMethodCallWithReturnValue(Awesomium::WebView *caller, unsigned int remote_object_id, const Awesomium::WebString &method_name, const Awesomium::JSArray &args) {

    std::cout << "Method call with return value: " << method_name << std::endl;
    IAwesomiumAPI<C>::getptr funcptr = _api->getFunctionWithReturnValue(Awesomium::ToString(method_name));
    if (funcptr) {
        return ((*_api).*funcptr)(args);
    }
    return Awesomium::JSValue(0);
}


/// Converts SDL button to awesomium button
template <class C>
Awesomium::MouseButton AwesomiumInterface<C>::getAwesomiumButtonFromSDL(Uint8 SDLButton) {
    switch (SDLButton) {
        case SDL_BUTTON_LEFT:
            return Awesomium::MouseButton::kMouseButton_Left;
        case SDL_BUTTON_RIGHT:
            return Awesomium::MouseButton::kMouseButton_Right;
        case SDL_BUTTON_MIDDLE:
            return Awesomium::MouseButton::kMouseButton_Middle;
    }
    return Awesomium::MouseButton::kMouseButton_Left;
}

/// Helper Macros
#define mapScanKey(a, b) case SDL_SCANCODE_##a: return Awesomium::KeyCodes::AK_##b;
#define mapKey(a, b) case SDLK_##a: return Awesomium::KeyCodes::AK_##b;

/// Get an Awesomium KeyCode from an SDLKey Code
template <class CC>
int AwesomiumInterface<CC>::getWebKeyFromSDLKey(SDL_Scancode key) {
    switch (key) {
        mapScanKey(BACKSPACE, BACK)
            mapScanKey(TAB, TAB)
            mapScanKey(CLEAR, CLEAR)
            mapScanKey(RETURN, RETURN)
            mapScanKey(PAUSE, PAUSE)
            mapScanKey(ESCAPE, ESCAPE)
            mapScanKey(SPACE, SPACE)
         //   mapKey(EXCLAIM, 1) // These are virtual keys so they don't belong here?
         //   mapKey(QUOTEDBL, 2)
         //   mapKey(HASH, 3)
        //    mapKey(DOLLAR, 4)
         //   mapKey(AMPERSAND, 7)
        //    mapKey(QUOTE, OEM_7)
        //    mapKey(LEFTPAREN, 9)
        //    mapKey(RIGHTPAREN, 0)
        //    mapKey(ASTERISK, 8)
        //    mapKey(PLUS, OEM_PLUS)
            mapScanKey(COMMA, OEM_COMMA)
            mapScanKey(MINUS, OEM_MINUS)
            mapScanKey(PERIOD, OEM_PERIOD)
            mapScanKey(SLASH, OEM_2)
            mapScanKey(0, 0)
            mapScanKey(1, 1)
            mapScanKey(2, 2)
            mapScanKey(3, 3)
            mapScanKey(4, 4)
            mapScanKey(5, 5)
            mapScanKey(6, 6)
            mapScanKey(7, 7)
            mapScanKey(8, 8)
            mapScanKey(9, 9)
       //     mapKey(COLON, OEM_1)
            mapScanKey(SEMICOLON, OEM_1)
        //    mapKey(LESS, OEM_COMMA)
            mapScanKey(EQUALS, OEM_PLUS)
       //     mapKey(GREATER, OEM_PERIOD)
        //    mapScanKey(QUESTION, OEM_2)
       //     mapScanKey(AT, 2)
            mapScanKey(LEFTBRACKET, OEM_4)
            mapScanKey(BACKSLASH, OEM_5)
            mapScanKey(RIGHTBRACKET, OEM_6)
      //      mapKey(CARET, 6)
      //      mapKey(UNDERSCORE, OEM_MINUS)
      //      mapKey(BACKQUOTE, OEM_3)
            mapScanKey(A, A)
            mapScanKey(B, B)
            mapScanKey(C, C)
            mapScanKey(D, D)
            mapScanKey(E, E)
            mapScanKey(F, F)
            mapScanKey(G, G)
            mapScanKey(H, H)
            mapScanKey(I, I)
            mapScanKey(J, J)
            mapScanKey(K, K)
            mapScanKey(L, L)
            mapScanKey(M, M)
            mapScanKey(N, N)
            mapScanKey(O, O)
            mapScanKey(P, P)
            mapScanKey(Q, Q)
            mapScanKey(R, R)
            mapScanKey(S, S)
            mapScanKey(T, T)
            mapScanKey(U, U)
            mapScanKey(V, V)
            mapScanKey(W, W)
            mapScanKey(X, X)
            mapScanKey(Y, Y)
            mapScanKey(Z, Z)
            //  mapScanKey(DELETE, DELETE)
            /*  mapScanKey(KP0, NUMPAD0)
            mapScanKey(KP1, NUMPAD1)
            mapScanKey(KP2, NUMPAD2)
            mapScanKey(KP3, NUMPAD3)
            mapScanKey(KP4, NUMPAD4)
            mapScanKey(KP5, NUMPAD5)
            mapScanKey(KP6, NUMPAD6)
            mapScanKey(KP7, NUMPAD7)
            mapScanKey(KP8, NUMPAD8)
            mapScanKey(KP9, NUMPAD9)*/
            mapScanKey(KP_PERIOD, DECIMAL)
            mapScanKey(KP_DIVIDE, DIVIDE)
            mapScanKey(KP_MULTIPLY, MULTIPLY)
            mapScanKey(KP_MINUS, SUBTRACT)
            mapScanKey(KP_PLUS, ADD)
            mapScanKey(KP_ENTER, SEPARATOR)
            mapScanKey(KP_EQUALS, UNKNOWN)
            mapScanKey(UP, UP)
            mapScanKey(DOWN, DOWN)
            mapScanKey(RIGHT, RIGHT)
            mapScanKey(LEFT, LEFT)
            mapScanKey(INSERT, INSERT)
            mapScanKey(HOME, HOME)
            mapScanKey(END, END)
            mapScanKey(PAGEUP, PRIOR)
            mapScanKey(PAGEDOWN, NEXT)
            mapScanKey(F1, F1)
            mapScanKey(F2, F2)
            mapScanKey(F3, F3)
            mapScanKey(F4, F4)
            mapScanKey(F5, F5)
            mapScanKey(F6, F6)
            mapScanKey(F7, F7)
            mapScanKey(F8, F8)
            mapScanKey(F9, F9)
            mapScanKey(F10, F10)
            mapScanKey(F11, F11)
            mapScanKey(F12, F12)
            mapScanKey(F13, F13)
            mapScanKey(F14, F14)
            mapScanKey(F15, F15)
            //mapScanKey(NUMLOCK, NUMLOCK)
            mapScanKey(CAPSLOCK, CAPITAL)
            //  mapScanKey(SCROLLOCK, SCROLL)
            mapScanKey(RSHIFT, RSHIFT)
            mapScanKey(LSHIFT, LSHIFT)
            mapScanKey(RCTRL, RCONTROL)
            mapScanKey(LCTRL, LCONTROL)
            mapScanKey(RALT, RMENU)
            mapScanKey(LALT, LMENU)
            //  mapScanKey(RMETA, LWIN)
            //  mapScanKey(LMETA, RWIN)
            //  mapScanKey(LSUPER, LWIN)
            //  mapScanKey(RSUPER, RWIN)
            mapScanKey(MODE, MODECHANGE)
            //  mapScanKey(COMPOSE, ACCEPT)
            mapScanKey(HELP, HELP)
            //  mapScanKey(PRINT, SNAPSHOT)
            mapScanKey(SYSREQ, EXECUTE)
        default:
            return Awesomium::KeyCodes::AK_UNKNOWN;
    }
}

}
}

#endif // AWESOMIUMINTERFACE_CPP_
