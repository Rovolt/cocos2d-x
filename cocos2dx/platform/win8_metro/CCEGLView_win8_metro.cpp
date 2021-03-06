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

#include "pch.h"
#include "DirectXRender.h"

#include "CCEGLView.h"
#include "CCEGLViewProtocol.h"
#include "cocoa/CCSet.h"
#include "ccMacros.h"
#include "CCDirector.h"
#include "touch_dispatcher/CCTouch.h"
#include "touch_dispatcher/CCTouchDispatcher.h"

#include "text_input_node/CCIMEDispatcher.h"
#include "keypad_dispatcher/CCKeypadDispatcher.h"
#include "CCApplication.h"
//#include <fstream>
using namespace DirectX;
NS_CC_BEGIN;

static CCEGLView * s_pMainWindow;

// helper function to get the proper resolution scale
// (even if invalid value is received)
static float GetResolutionScale()
{
	float ret = 1.0;
	int resolutionScale = (int)Windows::Graphics::Display::DisplayProperties::ResolutionScale;
	if (resolutionScale && resolutionScale != 100)
	{
		ret = (float)resolutionScale / 100;
	}
	return ret;
}

CCEGLView::CCEGLView()
	: m_pDelegate(NULL)
	, m_fScreenScaleFactor(1.0f)
	, m_fWinScaleX(1.0f)
	, m_fWinScaleY(1.0f)
{
	m_rcViewPort.bottom = 0;
	m_rcViewPort.left = 0;
	m_rcViewPort.right = 0;
	m_rcViewPort.top = 0;

	m_d3dDevice = DirectXRender::SharedDXRender()->m_d3dDevice.Get();
	m_d3dContext = DirectXRender::SharedDXRender()->m_d3dContext.Get();
	m_swapChain = DirectXRender::SharedDXRender()->m_swapChain.Get();
	m_renderTargetView = DirectXRender::SharedDXRender()->m_renderTargetView.Get();
	m_depthStencilView = DirectXRender::SharedDXRender()->m_depthStencilView.Get();

	m_projectionMatrix = XMMatrixIdentity();
	m_viewMatrix = XMMatrixIdentity();
	mMatrixMode = -1;
}

CCEGLView::~CCEGLView()
{
}

ID3D11Device* CCEGLView::GetDevice()
{
	return m_d3dDevice;
}

ID3D11DeviceContext* CCEGLView::GetDeviceContext()
{
	return m_d3dContext;
}

ID3D11DepthStencilView* CCEGLView::GetDepthStencilView()
{
	return m_depthStencilView;
}
//#ifdef CC_WIN8_PHONE
inline float DIP2Pixels(float DIP, float dpi)
{
	return DIP * dpi / 96;
}
//#endif
bool CCEGLView::Create()
{
	bool bRet = false;
	do 
	{
		DirectXRender^ render = DirectXRender::SharedDXRender();

		//#ifndef CC_WIN8_PHONE
		//		m_obScreenSize.width = (int)render->m_window->Bounds.Width;
		//		m_obScreenSize.height = (int)render->m_window->Bounds.Height;
		//#else
		float dpi = render->GetDpi();
		m_obScreenSize.width = DIP2Pixels(render->m_window->Bounds.Width, dpi);
		m_obScreenSize.height = DIP2Pixels(render->m_window->Bounds.Height, dpi);
		//#endif
		setDesignResolution(m_obScreenSize.width, m_obScreenSize.height);
		SetBackBufferRenderTarget();
#ifndef CC_WIN8_PHONE
		m_oldViewState = int(Windows::UI::ViewManagement::ApplicationView::Value);
#else
		m_oldViewState = 0;
#endif
		s_pMainWindow = this;
		bRet = true;
	} while (0);

	return bRet;
}

CCSize CCEGLView::getSize()
{
	return m_sizeInPoints;
}

CCSize CCEGLView::getSizeInPixel()
{
	return getSize();
}

bool CCEGLView::isOpenGLReady()
{
	return s_pMainWindow != NULL;
}

void CCEGLView::release()
{
	s_pMainWindow = NULL;

	SetMap::iterator setIter = m_pSets.begin();
	for ( ; setIter != m_pSets.end(); ++setIter)
	{
		CC_SAFE_DELETE(setIter->second);
	}
	m_pSets.clear();

	TouchMap::iterator touchIter = m_pTouches.begin();
	for ( ; touchIter != m_pTouches.end(); ++touchIter)
	{
		CC_SAFE_DELETE(touchIter->second);
	}
	m_pTouches.clear();

	CC_SAFE_DELETE(m_pDelegate);
}

void CCEGLView::setTouchDelegate(EGLTouchDelegate * pDelegate)
{
	m_pDelegate = pDelegate;
}

void CCEGLView::swapBuffers()
{
	DirectXRender::SharedDXRender()->Present();
}

void CCEGLView::setViewPortInPoints(float x, float y, float w, float h)
{
	/*float factor = m_fScreenScaleFactor * CC_CONTENT_SCALE_FACTOR();

	*/
//#ifndef CC_WIN8_PHONE
	D3DViewport(
		(int)(x),
		(int)(y),
		(int)(w),
		(int)(h));
//#else
//	D3DViewport(
//		(int)(x * m_fScaleX + m_obViewPortRect.origin.y),
//		(int)(y * m_fScaleY + m_obViewPortRect.origin.x),
//		(int)(w * m_fScaleX),
//		(int)(h * m_fScaleY));
//#endif
	/*glViewport((GLint)(x * m_fScaleX + m_obViewPortRect.origin.x),
	(GLint)(y * m_fScaleY + m_obViewPortRect.origin.y),
	(GLsizei)(w * m_fScaleX),
	(GLsizei)(h * m_fScaleY));*/
}

void CCEGLView::setScissorInPoints(float x, float y, float w, float h)
{
	float factor = m_fScreenScaleFactor * CC_CONTENT_SCALE_FACTOR();
	// Switch coordinate system's origin from bottomleft(OpenGL) to topleft(DirectX)
#ifndef CC_WIN8_PHONE
	y = m_sizeInPoints.height - ((y+ m_obViewPortOrigin.y) + h) * m_fViewPortScale * m_fScaleY; 
	D3DScissor(
		(int)((x + m_obViewPortOrigin.x)*m_fViewPortScale * m_fScaleX + m_obViewPortRect.origin.x),
		(int)((y + m_obViewPortRect.origin.y) ),
		(int)(w * m_fScaleX * m_fViewPortScale),
		(int)(h * m_fScaleY * m_fViewPortScale));
#else
	//y = m_sizeInPoints.width - (y + h) * m_fScaleY; 
	//x = m_sizeInPoints.height - (x + w) * m_fScaleX;
	D3DScissor(
		(int)(y + m_obViewPortRect.origin.y),
		(int)(x * m_fScaleX + m_obViewPortRect.origin.y),
		(int)(h * m_fScaleY),
		(int)(w * m_fScaleX));
#endif

	/*glScissor((GLint)(x * m_fScaleX + m_obViewPortRect.origin.x),
	(GLint)(y * m_fScaleY + m_obViewPortRect.origin.y),
	(GLsizei)(w * m_fScaleX),
	(GLsizei)(h * m_fScaleY));*/
}

void CCEGLView::setIMEKeyboardState(bool /*bOpen*/)
{
}

void CCEGLView::getScreenRectInView(CCRect& rect)
{
	DirectXRender^ render = DirectXRender::SharedDXRender();
	float winWidth = render->m_window->Bounds.Width;
	float winHeight = render->m_window->Bounds.Height;

	rect.origin.x = float(- m_rcViewPort.left) / m_fScreenScaleFactor;
	rect.origin.y = float((m_rcViewPort.bottom - m_rcViewPort.top) - winHeight) / (2.0f * m_fScreenScaleFactor);
	rect.size.width = float(winWidth) / m_fScreenScaleFactor;
	rect.size.height = float(winHeight) / m_fScreenScaleFactor;
}

void CCEGLView::setScreenScale(float factor)
{
	m_fScreenScaleFactor = factor;
}

bool CCEGLView::canSetContentScaleFactor()
{
	return false;
}

void CCEGLView::setContentScaleFactor(float contentScaleFactor)
{
	m_fScreenScaleFactor = contentScaleFactor;
}

void CCEGLView::setDesignResolution(int dx, int dy)
{
	// 重新计算 contentScale 和 m_rcViewPort 
	m_sizeInPoints.width = (float)dx;
	m_sizeInPoints.height = (float)dy;

	DirectXRender^ render = DirectXRender::SharedDXRender();
	//#ifndef CC_WIN8_PHONE
	//	float winWidth = render->m_window->Bounds.Width;
	//    float winHeight = render->m_window->Bounds.Height;
	//#else
	float dpi = render->GetDpi();
	float winWidth = DIP2Pixels(render->m_window->Bounds.Width, dpi);
	float winHeight = DIP2Pixels(render->m_window->Bounds.Height, dpi);

	//#endif


	// m_window size might be less than its real size due to ResolutionScale
	winWidth *= GetResolutionScale();
	winHeight *= GetResolutionScale();

	m_fScreenScaleFactor = min(winWidth / dx, winHeight / dy);
	m_fScreenScaleFactor *= CCDirector::sharedDirector()->getContentScaleFactor();

	int viewPortW = (int)(m_sizeInPoints.width * m_fScreenScaleFactor);
	int viewPortH = (int)(m_sizeInPoints.height * m_fScreenScaleFactor);

	// calculate client new width and height
	m_rcViewPort.left   = LONG((winWidth - viewPortW) / 2);
	m_rcViewPort.top    = LONG((winHeight - viewPortH) / 2);
	m_rcViewPort.right  = LONG(m_rcViewPort.left + viewPortW);
	m_rcViewPort.bottom = LONG(m_rcViewPort.top + viewPortH);
}

void CCEGLView::SetBackBufferRenderTarget()
{
	m_d3dContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
}

void CCEGLViewProtocol::D3DPerspective( FLOAT fovy, FLOAT aspect, FLOAT zNear, FLOAT zFar)
{
	CCfloat xmin, xmax, ymin, ymax;

	ymax = zNear * (CCfloat)tanf(fovy * (float)M_PI / 360);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	XMMATRIX tmpMatrix;
	tmpMatrix = XMMatrixPerspectiveOffCenterRH(xmin, xmax, ymin, ymax, zNear, zFar);
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
	}
}

void CCEGLViewProtocol::D3DOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
{
	XMMATRIX tmpMatrix;
	tmpMatrix = XMMatrixOrthographicOffCenterRH(left, right, bottom, top, zNear, zFar);
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
	}
}

void CCEGLViewProtocol::D3DLookAt(float fEyeX, float fEyeY, float fEyeZ, float fLookAtX, float fLookAtY, float fLookAtZ, float fUpX, float fUpY, float fUpZ)
{
	XMMATRIX tmpMatrix;
	tmpMatrix = XMMatrixLookAtRH(XMVectorSet(fEyeX,fEyeY,fEyeZ,0.f), XMVectorSet(fLookAtX,fLookAtY,fLookAtZ,0.f), XMVectorSet(fUpX,fUpY,fUpZ,0.f));
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
	}
}

void CCEGLViewProtocol::D3DLoadIdentity()
{
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixIdentity();
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixIdentity();
	}
}

void CCEGLViewProtocol::D3DViewport(int x, int y, int width, int height)
{
	D3D11_VIEWPORT viewport;
	//std::cout << "Viewport: x " << x << " y " << y << " widht " << width << " height " << height << std::endl;
	// Setup the viewport for rendering.
	//#ifndef CC_WIN8_PHONE
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	//#else
	//	//To make the view landscape
	//	viewport.Width = (float)height;
	//	viewport.Height = (float)width;
	//#endif
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = (float)x;
	viewport.TopLeftY = (float)y;

	// Create the viewport.
	m_d3dContext->RSSetViewports(1, &viewport);
}

void CCEGLViewProtocol::D3DScissor(int x,int y,int w,int h)
{
	D3D11_RECT scissorRects;
#ifndef CC_WIN8_PHONE
	scissorRects.top = y;
	scissorRects.left = x;
	scissorRects.right = x+w;
	scissorRects.bottom = y+h;
#else
	scissorRects.top = y;
	scissorRects.left = x;
	scissorRects.right = x+w;
	scissorRects.bottom = y+h;
#endif

	m_d3dContext->RSSetScissorRects(1,&scissorRects);
}

void CCEGLView::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
	return;
}

void CCEGLView::GetViewMatrix(XMMATRIX& viewMatrix)
{
	viewMatrix = m_viewMatrix;
	return;
}

void CCEGLView::SetProjectionMatrix(const XMMATRIX& projectionMatrix)
{
	m_projectionMatrix = projectionMatrix;
	return;
}

void CCEGLView::SetViewMatrix(const XMMATRIX& viewMatrix)
{
	m_viewMatrix = viewMatrix;
	return;
}

void CCEGLViewProtocol::D3DMatrixMode(int matrixMode)
{
	mMatrixMode = matrixMode;
}


void CCEGLViewProtocol::D3DPushMatrix()
{
	MatrixStruct matrixStruct;
	matrixStruct.projection = m_projectionMatrix;
	matrixStruct.view = m_viewMatrix;
	m_MatrixStack.push(matrixStruct);
}
void CCEGLViewProtocol::D3DPopMatrix()
{
	MatrixStruct matrixStruct;
	matrixStruct = m_MatrixStack.top();
	m_viewMatrix = matrixStruct.view;
	m_projectionMatrix = matrixStruct.projection;
	m_MatrixStack.pop();
}

//viewMatrix !!!m2*m2!!!
void CCEGLViewProtocol::D3DTranslate(float x, float y, float z)
{
	XMMATRIX tmpMatrix;
	tmpMatrix = XMMatrixTranslation(x,y,z);
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
	}
}
void CCEGLViewProtocol::D3DRotate(float angle, float x, float y, float z)
{
	XMMATRIX tmpMatrix;
	if ( x )
	{
		tmpMatrix = XMMatrixRotationX(angle);
		if ( mMatrixMode == CC_PROJECTION )
		{
			m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
		}
		else if ( mMatrixMode == CC_MODELVIEW )
		{
			m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
		}
	}
	if ( y )
	{
		tmpMatrix = XMMatrixRotationY(angle);
		if ( mMatrixMode == CC_PROJECTION )
		{
			m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
		}
		else if ( mMatrixMode == CC_MODELVIEW )
		{
			m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
		}
	}
	if ( z )
	{
		tmpMatrix = XMMatrixRotationZ(angle);
		if ( mMatrixMode == CC_PROJECTION )
		{
			m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
		}
		else if ( mMatrixMode == CC_MODELVIEW )
		{
			m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
		}
	}
}
void CCEGLViewProtocol::D3DScale(float x, float y, float z)
{
	XMMATRIX tmpMatrix;
	tmpMatrix = XMMatrixScaling(x,y,z);
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
	}
}

void CCEGLViewProtocol::D3DMultMatrix(const float *m)
{
	XMMATRIX tmpMatrix=XMMATRIX(m);
	if ( mMatrixMode == CC_PROJECTION )
	{
		m_projectionMatrix = XMMatrixMultiply(tmpMatrix,m_projectionMatrix);
	}
	else if ( mMatrixMode == CC_MODELVIEW )
	{
		m_viewMatrix = XMMatrixMultiply(tmpMatrix,m_viewMatrix);
	}
}

void CCEGLViewProtocol::D3DDepthFunc(int func)
{
	ID3D11DepthStencilState *dss0 = 0;
	ID3D11DepthStencilState *dss1 = 0;
	D3D11_DEPTH_STENCIL_DESC dsd;
	UINT sref;
	//m_d3dContext->ClearDepthStencilView(m_d3dContext->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	bool en = TRUE;
	int wm = D3D11_DEPTH_WRITE_MASK_ALL;

	switch(func)
	{
	case CC_NEVER:		func = D3D11_COMPARISON_NEVER; break;
	case CC_LESS:		func = D3D11_COMPARISON_LESS; break;
	case CC_EQUAL:		func = D3D11_COMPARISON_EQUAL; break;
	case CC_LEQUAL:		func = D3D11_COMPARISON_LESS_EQUAL; break;
	case CC_GREATER:	func = D3D11_COMPARISON_GREATER; break;
	case CC_NOTEQUAL:	func = D3D11_COMPARISON_NOT_EQUAL; break;
	case CC_GEQUAL:		func = D3D11_COMPARISON_GREATER_EQUAL; break;
	case CC_ALWAYS:		func = D3D11_COMPARISON_ALWAYS; break;
	default:en = FALSE; wm = D3D11_DEPTH_WRITE_MASK_ZERO;break;
	}

	m_d3dContext->OMGetDepthStencilState(&dss0,&sref);

	if(dss0)
		dss0->GetDesc(&dsd);
	else
	{
		ZeroMemory(&dsd,sizeof(D3D11_DEPTH_STENCIL_DESC));
		sref = 0;
	}

	dsd.DepthEnable = en;
	dsd.DepthWriteMask = static_cast<D3D11_DEPTH_WRITE_MASK>(wm);
	dsd.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(func);

	dsd.StencilEnable = en;
	dsd.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	if(FAILED(m_d3dDevice->CreateDepthStencilState(&dsd,&dss1)))
		exit(-1);

	m_d3dContext->OMSetDepthStencilState(dss1,sref);

	if(dss0)
		dss0->Release();
	dss1->Release();
}

void CCEGLViewProtocol::D3DBlendFunc(int sfactor, int dfactor)
{
	int sfactor2 = sfactor;
	int dfactor2 = dfactor;
	switch(sfactor)
	{
	case CC_ZERO:					sfactor=D3D11_BLEND_ZERO; sfactor2=D3D11_BLEND_ZERO;break;
	case CC_ONE:					sfactor=D3D11_BLEND_ONE; sfactor2=D3D11_BLEND_ONE;break;
	case CC_DST_COLOR:				sfactor=D3D11_BLEND_DEST_COLOR; sfactor2=D3D11_BLEND_DEST_ALPHA;break;
	case CC_ONE_MINUS_DST_COLOR:	sfactor=D3D11_BLEND_INV_DEST_COLOR; sfactor2=D3D11_BLEND_INV_DEST_ALPHA;break;
	case CC_SRC_ALPHA_SATURATE:		sfactor2=D3D11_BLEND_SRC_ALPHA_SAT; sfactor=D3D11_BLEND_SRC_ALPHA_SAT;break;
	case CC_SRC_ALPHA:				sfactor2=D3D11_BLEND_SRC_ALPHA; sfactor=D3D11_BLEND_SRC_ALPHA;break;
	case CC_ONE_MINUS_SRC_ALPHA:	sfactor2=D3D11_BLEND_INV_SRC_ALPHA; sfactor=D3D11_BLEND_INV_SRC_ALPHA;break;
	case CC_DST_ALPHA:				sfactor2=D3D11_BLEND_DEST_ALPHA; sfactor=D3D11_BLEND_DEST_ALPHA;break;
	case CC_ONE_MINUS_DST_ALPHA:	sfactor2=D3D11_BLEND_INV_DEST_ALPHA; sfactor=D3D11_BLEND_INV_DEST_ALPHA;break;
	}
	switch(dfactor)
	{
	case CC_ZERO:					dfactor=D3D11_BLEND_ZERO; dfactor2=D3D11_BLEND_ZERO;break;
	case CC_ONE:					dfactor=D3D11_BLEND_ONE; dfactor2=D3D11_BLEND_ONE;break;
	case CC_SRC_COLOR:				dfactor=D3D11_BLEND_SRC_COLOR; dfactor2=D3D11_BLEND_SRC_ALPHA;break;
	case CC_ONE_MINUS_SRC_COLOR:	dfactor=D3D11_BLEND_INV_SRC_COLOR; dfactor2=D3D11_BLEND_INV_SRC_ALPHA;break;
	case CC_SRC_ALPHA:				dfactor2=D3D11_BLEND_SRC_ALPHA; dfactor=D3D11_BLEND_SRC_ALPHA;break;
	case CC_ONE_MINUS_SRC_ALPHA:	dfactor2=D3D11_BLEND_INV_SRC_ALPHA; dfactor=D3D11_BLEND_INV_SRC_ALPHA;break;
	case CC_DST_ALPHA:				dfactor2=D3D11_BLEND_DEST_ALPHA; dfactor=D3D11_BLEND_DEST_ALPHA;break;
	case CC_ONE_MINUS_DST_ALPHA:	dfactor2=D3D11_BLEND_INV_DEST_ALPHA; dfactor=D3D11_BLEND_INV_DEST_ALPHA;break;
	}

	ID3D11BlendState* dbs0;
	ID3D11BlendState* dbs1;
	D3D11_BLEND_DESC dbd;
	float blendFactor[4]={0.0f,0.0f,0.0f,0.0f};
	UINT sref;
	m_d3dContext->OMGetBlendState(&dbs0,blendFactor,&sref);

	if(dbs0)
		dbs0->GetDesc(&dbd);
	else
	{
		ZeroMemory(&dbd,sizeof(D3D11_BLEND_DESC));
		sref = 0;
	}

	if ( (dbd.RenderTarget[0].SrcBlend != (D3D11_BLEND)sfactor)|| (dbd.RenderTarget[0].DestBlend != (D3D11_BLEND)dfactor) )
	{
		if ( (sfactor==-1) && (dfactor==-1) )
		{
			dbd.RenderTarget[0].BlendEnable = FALSE;
			sfactor = dfactor = D3D11_BLEND_ONE;
			sfactor2 = dfactor2 = D3D11_BLEND_ONE;
		}
		else
		{
			dbd.RenderTarget[0].BlendEnable = TRUE;
		}
		dbd.AlphaToCoverageEnable = FALSE;
		dbd.IndependentBlendEnable = FALSE;
		dbd.RenderTarget[0].SrcBlend = (D3D11_BLEND)sfactor;
		dbd.RenderTarget[0].DestBlend = (D3D11_BLEND)dfactor;
		dbd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		dbd.RenderTarget[0].SrcBlendAlpha = (D3D11_BLEND)sfactor2;
		dbd.RenderTarget[0].DestBlendAlpha = (D3D11_BLEND)dfactor2;
		dbd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		dbd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		memcpy( &dbd.RenderTarget[1], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
		memcpy( &dbd.RenderTarget[2], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
		memcpy( &dbd.RenderTarget[3], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
		memcpy( &dbd.RenderTarget[4], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
		memcpy( &dbd.RenderTarget[5], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
		memcpy( &dbd.RenderTarget[6], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
		memcpy( &dbd.RenderTarget[7], &dbd.RenderTarget[0], sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) );
	}

	if(FAILED(m_d3dDevice->CreateBlendState(&dbd,&dbs1)))
		exit(-1);
	m_d3dContext->OMSetBlendState(dbs1, blendFactor, 0xffffffff);
	if(dbs0)
		dbs0->Release();
	dbs1->Release();
}

void CCEGLView::clearRender(ID3D11RenderTargetView* renderTargetView)
{
	float color[4]={m_color[0],m_color[1],m_color[2],m_color[3]};
	if ( !renderTargetView )
	{
		m_d3dContext->ClearRenderTargetView(m_renderTargetView, color);
	}
	else
	{
		m_d3dContext->ClearRenderTargetView(renderTargetView, color);
	}
	m_d3dContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void CCEGLViewProtocol::D3DClearColor(float r, float b, float g, float a)
{
	m_color[0] = r;
	m_color[1] = b;
	m_color[2] = g;
	m_color[3] = a;
}

void CCEGLView::GetClearColor(float* color)
{
	color[0] = m_color[0];
	color[1] = m_color[1];
	color[2] = m_color[2];
	color[3] = m_color[3];
}

CCEGLView* CCEGLView::sharedOpenGLView()
{
	assert(s_pMainWindow);
	return s_pMainWindow;
}

void CCEGLView::OnWindowSizeChanged()
{
	m_renderTargetView = DirectXRender::SharedDXRender()->m_renderTargetView.Get();
	m_depthStencilView = DirectXRender::SharedDXRender()->m_depthStencilView.Get();

	//// 重新确定 viewPort
	DirectXRender^ render = DirectXRender::SharedDXRender();
	static float dpi = DirectXRender::SharedDXRender()->GetDpi();

	float winWidth = DIP2Pixels(render->m_window->Bounds.Width, dpi);
	float winHeight = DIP2Pixels(render->m_window->Bounds.Height, dpi);



	CCDirector::sharedDirector()->setProjection(kCCDirectorProjection3D);
	float design_width = m_obScreenSize.width / getScaleX();
	float design_height = m_obScreenSize.height / getScaleY();

	float scale = MIN(winWidth / design_width,
		winHeight / design_height);


	if(design_width*scale > winWidth)
	{
		design_width = winWidth / scale;
	}
	if(design_height*scale > winHeight)
	{
		design_height = winHeight / scale;
	}

	float new_width = design_width*scale;
	float new_height = design_height*scale;
	m_obViewPortOrigin.x = (winWidth - new_width) / 2;
	m_obViewPortOrigin.y = (winHeight - new_height) /2;
	setViewPortInPoints(m_obViewPortOrigin.x, m_obViewPortOrigin.y, new_width, new_height);

	m_fViewPortScale = scale / getScaleX();
}

void CCEGLView::OnCharacterReceived(unsigned int keyCode)
{
	switch (keyCode)
	{
	case VK_BACK:
		CCIMEDispatcher::sharedDispatcher()->dispatchDeleteBackward();
		break;

	case VK_RETURN:
		CCIMEDispatcher::sharedDispatcher()->dispatchInsertText("\n", 1);
		break;

		//    case VK_TAB:
		//
		//        break;
		//
	case VK_ESCAPE:
		// ESC input
		//CCDirector::sharedDirector()->end();
		//CCKeypadDispatcher::sharedDispatcher()->dispatchKeypadMSG(kTypeBackClicked);

		break;

	default:
		if (keyCode < 0x20)
		{
			break;
		}
		else if (keyCode < 128)
		{
			// ascii char
			CCIMEDispatcher::sharedDispatcher()->dispatchInsertText((const char *)&keyCode, 1);
		}
		else
		{
			char szUtf8[8] = {0};
			int nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&keyCode, 1, szUtf8, sizeof(szUtf8), NULL, NULL);

			CCIMEDispatcher::sharedDispatcher()->dispatchInsertText(szUtf8, nLen);
		}
		break;
	}
}

void CCEGLView::ConvertPointerCoords(float &x, float &y)
{
	//#ifdef CC_WIN8_PHONE
	static float dpi = DirectXRender::SharedDXRender()->GetDpi();
	x = DIP2Pixels(x, dpi);
	y = DIP2Pixels(y, dpi);
	//#endif
#ifndef CC_WIN8_PHONE
	y = m_obScreenSize.height-y;
#endif

	x=((x - m_obViewPortOrigin.x)/ m_fViewPortScale - m_obViewPortRect.origin.x)  / m_fScaleX ;
	y=((y - m_obViewPortOrigin.y)/ m_fViewPortScale - m_obViewPortRect.origin.y)  / m_fScaleY;


}

void CCEGLView::end()
{}

void CCEGLView::OnPointerPressed(int id, const CCPoint& point)
{
	// prepare CCTouch
	CCTouch* pTouch = m_pTouches[id];
	if (! pTouch)
	{
		pTouch = new CCTouch();
		m_pTouches[id] = pTouch;
	}

	// prepare CCSet
	CCSet* pSet = m_pSets[id];
	if (! pSet)
	{
		pSet = new CCSet();
		m_pSets[id] = pSet;
	}

	if (! pTouch || ! pSet)
		return;
#ifndef CC_WIN8_PHONE
	float x = point.x;
	float y = point.y;
#else
	float x = point.y;
	float y = point.x;
#endif
	ConvertPointerCoords(x, y);
	pTouch->SetTouchInfo(x, y);
	pSet->addObject(pTouch);

	m_pDelegate->touchesBegan(pSet, NULL);
}

void CCEGLView::OnPointerReleased(int id, const CCPoint& point)
{
	CCTouch* pTouch = m_pTouches[id];
	CCSet* pSet = m_pSets[id];

	if (! pTouch || ! pSet)
		return;

#ifndef CC_WIN8_PHONE
	float x = point.x;
	float y = point.y;
#else
	float x = point.y;
	float y = point.x;
#endif
	ConvertPointerCoords(x, y);
	pTouch->SetTouchInfo(x, y);

	m_pDelegate->touchesEnded(pSet, NULL);
	pSet->removeObject(pTouch);

	CC_SAFE_DELETE(m_pTouches[id]);
	CC_SAFE_DELETE(m_pSets[id]);
}

void CCEGLView::OnPointerMoved(int id, const CCPoint& point)
{
	CCTouch* pTouch = m_pTouches[id];
	CCSet* pSet = m_pSets[id];

	if (! pTouch || ! pSet)
		return;

#ifndef CC_WIN8_PHONE
	float x = point.x;
	float y = point.y;
#else
	float x = point.y;
	float y = point.x;
#endif
	ConvertPointerCoords(x, y);
	pTouch->SetTouchInfo(x, y);

	m_pDelegate->touchesMoved(pSet, NULL);
}

NS_CC_END;
