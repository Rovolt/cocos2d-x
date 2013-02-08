/****************************************************************************
Copyright (c) 2012 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#pragma once

#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11_1.h>
#include <d2d1.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include "CCCommon.h"
#include "cocoa/CCGeometry.h"
#include <stack>
#include <vector>
#include <map>
#include "platform/CCEGLViewProtocol.h"
NS_CC_BEGIN;

class CCSet;
class CCTouch;
class EGLTouchDelegate;


class CC_DLL CCEGLView : public CCEGLViewProtocol
{
public:

    CCEGLView();
    virtual ~CCEGLView();

    ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();
	ID3D11DepthStencilView* GetDepthStencilView();

    CCSize  getSize();
    CCSize  getSizeInPixel();
    bool    isOpenGLReady();
    void    release();
    void    setTouchDelegate(EGLTouchDelegate * pDelegate);
    void    swapBuffers();
    bool    canSetContentScaleFactor();
    void    setContentScaleFactor(float contentScaleFactor);
    void    setDesignResolution(int dx, int dy);

	virtual bool Create();
	void end();
    void setViewPortInPoints(float x, float y, float w, float h);
    void setScissorInPoints(float x, float y, float w, float h);

    void setIMEKeyboardState(bool bOpen);

    void getScreenRectInView(CCRect& rect);
    void setScreenScale(float factor);

    void SetBackBufferRenderTarget();
	void clearRender(ID3D11RenderTargetView* renderTargetView);	
	void GetProjectionMatrix(DirectX::XMMATRIX& projectionMatrix);
	void GetViewMatrix(DirectX::XMMATRIX& viewMatrix);
	void SetProjectionMatrix(const DirectX::XMMATRIX& projectionMatrix);
	void SetViewMatrix(const DirectX::XMMATRIX& viewMatrix);

	void GetClearColor(float* color);
	

    // static function
    /**
    @brief	get the shared main open gl window
    */
	static CCEGLView& sharedOpenGLView();

    // metro only
    void OnWindowSizeChanged();
    void OnCharacterReceived(unsigned int keyCode);
	void OnPointerPressed(int id, const CCPoint& point);
    void OnPointerReleased(int id, const CCPoint& point);
    void OnPointerMoved(int id, const CCPoint& point);
protected:
    void ConvertPointerCoords(float &x, float &y);

private:
    

    typedef std::map<int, CCSet*> SetMap;
    SetMap              m_pSets;
    typedef std::map<int, CCTouch*> TouchMap;
    TouchMap            m_pTouches;

    EGLTouchDelegate*   m_pDelegate;
	
    CCSize              m_sizeInPoints;
    float               m_fScreenScaleFactor;
    RECT                m_rcViewPort;

    float               m_fWinScaleX;
    float               m_fWinScaleY;
    int                 m_initWinWidth;
    int                 m_initWinHeight;

    
};

NS_CC_END;
