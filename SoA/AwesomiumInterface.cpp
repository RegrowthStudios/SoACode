#include "stdafx.h"

#include <Awesomium/BitmapSurface.h>
#include <Awesomium/DataPak.h>
#include <Awesomium/DataSource.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/WebCore.h>

#include "AwesomiumInterface.h"

#include "SDL/SDL.h"
#include "Errors.h"
#include "shader.h"
#include "Options.h"

AwesomiumInterface::AwesomiumInterface() : _isInitialized(0), _width(0), _height(0), _vboID(0), _elementBufferID(0){}

AwesomiumInterface::~AwesomiumInterface(void) {}

//Initializes the interface. Returns false on failure
bool AwesomiumInterface::init(const char *inputDir, int width, int height)
{
    _width = width;
    _height = height;

    _webCore = Awesomium::WebCore::instance();
    if (_webCore == nullptr){
        _webCore = Awesomium::WebCore::Initialize(Awesomium::WebConfig());
    }

    _webSession = _webCore->CreateWebSession(Awesomium::WSLit("WebSession"), Awesomium::WebPreferences());
    _webView = _webCore->CreateWebView(_width, _height, _webSession, Awesomium::kWebViewType_Offscreen);

    if (!_webView){
        pError("Awesomium WebView could not be created!\n");
        return false;
    }

    if (!Awesomium::WriteDataPak(Awesomium::WSLit("UI_Resources.pak"), Awesomium::WSLit(inputDir), Awesomium::WSLit(""), _numFiles)){
        pError("UI Initialization Error: Failed to write UI_Resources.pak\n");
        return false;
    }

    _data_source = new Awesomium::DataPakSource(Awesomium::WSLit("UI_Resources.pak"));

    _webSession->AddDataSource(Awesomium::WSLit("UI"), _data_source);

    // Load a certain URL into our WebView instance
    Awesomium::WebURL url(Awesomium::WSLit("asset://UI/Index.html"));

    if (!url.IsValid()){
        pError("UI Initialization Error: URL was unable to be parsed.");
        return false;
    }

    _webView->LoadURL(url);

    // Wait for our WebView to finish loading
    while (_webView->IsLoading()) _webCore->Update();

    // Sleep a bit and update once more to give scripts and plugins
    // on the page a chance to finish loading.
    SLEEP(30);
    _webCore->Update();
    _webView->SetTransparent(1);
    _webView->set_js_method_handler(&_methodHandler);


    glGenTextures(1, &_renderedTexture);

    glBindTexture(GL_TEXTURE_2D, _renderedTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    Awesomium::Error error = _webView->last_error();
    if (error) {
        pError("Awesomium error " + std::to_string(error));
    }

    Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*)_webView->surface();
    if (!surface){
        showMessage("webView->surface() returned null!");
        exit(131521);
    }
    //surface->SaveToJPEG(WSLit("./UI/storedui.jpg")); //debug

    _jsValue = _webView->ExecuteJavascriptWithResult(Awesomium::WSLit("AppInterface"), Awesomium::WSLit(""));
    if (_jsValue.IsObject()){

    }
    _isInitialized = true;
    return true;
}

void AwesomiumInterface::update()
{
    _webCore->Update();

    Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*)_webView->surface();

    if (surface && surface->is_dirty())
    {
        // renders the surface buffer to the opengl texture!
        glBindTexture(GL_TEXTURE_2D, _renderedTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->width(), surface->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->buffer());
        surface->set_is_dirty(0);
    }
    if (!surface){
        pError("User Interface Error: webView->surface() returned null! Most likely due to erroneous code in the javascript or HTML5.\n");
    }
}

void AwesomiumInterface::draw()
{
    //Check if draw coords were set
    if (_vboID == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _elementBufferID);

    // Bind shader
    texture2Dshader.Bind((GLfloat)graphicsOptions.screenWidth, (GLfloat)graphicsOptions.screenHeight);
    glUniform1f(texture2Dshader.Text2DUseRoundMaskID, 0.0f);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _renderedTexture);

    glUniform1i(texture2Dshader.Text2DUniformID, 0);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)12);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)16);

    // Draw call
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texture2Dshader.Text2DElementBufferID);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    texture2Dshader.UnBind();
}

const GLubyte uiBoxUVs[8] = { 0, 0, 0, 255, 255, 255, 255, 0 };

void AwesomiumInterface::setDrawCoords(int x, int y, int width, int height) {
    if (_vboID == 0) {
        glGenBuffers(1, &_vboID);
        glGenBuffers(1, &_elementBufferID);

        GLubyte elements[6] = { 0, 1, 2, 2, 3, 0 };
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _elementBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    Vertex2D vertices[4];


    vertices[0].pos.x = x;
    vertices[0].pos.y = y + height;

    vertices[1].pos.x = x;
    vertices[1].pos.y = y;

    vertices[2].pos.x = x + width;
    vertices[2].pos.y = y;

    vertices[3].pos.x = x + width;
    vertices[3].pos.y = y + height;

    for (int i = 0; i < 4; i++) {
        vertices[i].uv[0] = uiBoxUVs[i * 2];
        vertices[i].uv[1] = uiBoxUVs[i * 2 + 1];

        vertices[i].color[0] = _color[0];
        vertices[i].color[1] = _color[1];
        vertices[i].color[2] = _color[2];
        vertices[i].color[3] = _color[3];
    }

    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void AwesomiumInterface::setColor(i32v4 color) {
    _color[0] = color[0];
    _color[1] = color[1];
    _color[2] = color[2];
    _color[3] = color[3];
}

int getWebKeyFromKey(Key key)
{
    switch (key)
    {
        mapWebKey(0, 0)
        mapWebKey(1, 1)
        mapWebKey(2, 2)
        mapWebKey(3, 3)
        mapWebKey(4, 4)
        mapWebKey(5, 5)
        mapWebKey(6, 6)
        mapWebKey(7, 7)
        mapWebKey(8, 8)
        mapWebKey(9, 9)
        mapWebKey(A, A)
        mapWebKey(B, B)
        mapWebKey(C, C)
        mapWebKey(D, D)
        mapWebKey(E, E)
        mapWebKey(F, F)
        mapWebKey(G, G)
        mapWebKey(H, H)
        mapWebKey(I, I)
        mapWebKey(J, J)
        mapWebKey(K, K)
        mapWebKey(L, L)
        mapWebKey(M, M)
        mapWebKey(N, N)
        mapWebKey(O, O)
        mapWebKey(P, P)
        mapWebKey(Q, Q)
        mapWebKey(R, R)
        mapWebKey(S, S)
        mapWebKey(T, T)
        mapWebKey(U, U)
        mapWebKey(V, V)
        mapWebKey(W, W)
        mapWebKey(X, X)
        mapWebKey(Y, Y)
        mapWebKey(Z, Z)
        mapWebKey(Up, UP)
        mapWebKey(Down, DOWN)
        mapWebKey(Right, RIGHT)
        mapWebKey(Left, LEFT)
        mapWebKey(Return, RETURN)
        mapWebKey(Escape, ESCAPE)
        mapWebKey(Delete, DELETE)
        mapWebKey(Space, SPACE)
        mapWebKey(Backspace, BACK)
        mapWebKey(Tab, TAB)
        mapWebKey(Kp0, NUMPAD0)
        mapWebKey(Kp1, NUMPAD1)
        mapWebKey(Kp2, NUMPAD2)
        mapWebKey(Kp3, NUMPAD3)
        mapWebKey(Kp4, NUMPAD4)
        mapWebKey(Kp5, NUMPAD5)
        mapWebKey(Kp6, NUMPAD6)
        mapWebKey(Kp7, NUMPAD7)
        mapWebKey(Kp8, NUMPAD8)
        mapWebKey(Kp9, NUMPAD9)
        mapWebKey(Rshift, RSHIFT)
        mapWebKey(Lshift, LSHIFT)
        mapWebKey(Rctrl, RCONTROL)
        mapWebKey(Lctrl, LCONTROL)
        mapWebKey(Ralt, RMENU)
        mapWebKey(Lalt, LMENU)
        mapWebKey(F1, F1)
        mapWebKey(F2, F2)
        mapWebKey(F3, F3)
        mapWebKey(F4, F4)
        mapWebKey(F5, F5)
        mapWebKey(F6, F6)
        mapWebKey(F7, F7)
        mapWebKey(F8, F8)
        mapWebKey(F9, F9)
        mapWebKey(F10, F10)
        mapWebKey(F11, F11)
        mapWebKey(F12, F12)
    default:
        return Awesomium::KeyCodes::AK_UNKNOWN;
    }
}
