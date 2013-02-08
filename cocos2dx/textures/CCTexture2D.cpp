/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008      Apple Inc. All Rights Reserved.

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



/*
* Support for RGBA_4_4_4_4 and RGBA_5_5_5_1 was copied from:
* https://devforums.apple.com/message/37855#37855 by a1studmuffin
*/

#include "CCTexture2D.h"
#include "ccConfig.h"
#include "ccMacros.h"
#include "CCConfiguration.h"
#include "platform/platform.h"
#include "platform/CCImage.h"
#include "CCGL.h"
#include "support/ccUtils.h"
#include "platform/CCPlatformMacros.h"
#include "textures/CCTexturePVR.h"
#include "CCDirector.h"
#include "shaders/CCGLProgram.h"
#include "shaders/ccGLStateCache.h"
#include "shaders/CCShaderCache.h"
#include "CCEGLView.h"
#if CC_ENABLE_CACHE_TEXTURE_DATA
    #include "CCTextureCache.h"
#endif

NS_CC_BEGIN

//CLASS IMPLEMENTATIONS:

// If the image has alpha, you can create RGBA8 (32-bit) or RGBA4 (16-bit) or RGB5A1 (16-bit)
// Default is: RGBA8888 (32-bit textures)
static CCTexture2DPixelFormat g_defaultAlphaPixelFormat = kCCTexture2DPixelFormat_Default;

// By default PVR images are treated as if they don't have the alpha channel premultiplied
static bool PVRHaveAlphaPremultiplied_ = false;

CCTexture2D::CCTexture2D()
: m_uPixelsWide(0)
, m_uPixelsHigh(0)
, m_uName(0)
, m_fMaxS(0.0)
, m_fMaxT(0.0)
, m_bHasPremultipliedAlpha(false)
, m_bHasMipmaps(false)
, m_bPVRHaveAlphaPremultiplied(true)
, m_pShaderProgram(NULL)
{
}

CCTexture2D::~CCTexture2D()
{
#if CC_ENABLE_CACHE_TEXTURE_DATA
    VolatileTexture::removeTexture(this);
#endif

    CCLOGINFO("cocos2d: deallocing CCTexture2D %u.", m_uName);
    CC_SAFE_RELEASE(m_pShaderProgram);

    if(m_uName)
    {
        ccGLDeleteTexture(m_uName);
    }
}

CCTexture2DPixelFormat CCTexture2D::getPixelFormat()
{
    return m_ePixelFormat;
}

unsigned int CCTexture2D::getPixelsWide()
{
    return m_uPixelsWide;
}

unsigned int CCTexture2D::getPixelsHigh()
{
    return m_uPixelsHigh;
}

GLuint CCTexture2D::getName()
{
    return m_uName;
}

CCSize CCTexture2D::getContentSize()
{

    CCSize ret;
    ret.width = m_tContentSize.width / CC_CONTENT_SCALE_FACTOR();
    ret.height = m_tContentSize.height / CC_CONTENT_SCALE_FACTOR();
    
    return ret;
}

const CCSize& CCTexture2D::getContentSizeInPixels()
{
    return m_tContentSize;
}

GLfloat CCTexture2D::getMaxS()
{
    return m_fMaxS;
}

void CCTexture2D::setMaxS(GLfloat maxS)
{
    m_fMaxS = maxS;
}

GLfloat CCTexture2D::getMaxT()
{
    return m_fMaxT;
}

void CCTexture2D::setMaxT(GLfloat maxT)
{
    m_fMaxT = maxT;
}

CCGLProgram* CCTexture2D::getShaderProgram(void)
{
    return m_pShaderProgram;
}

void CCTexture2D::setShaderProgram(CCGLProgram* pShaderProgram)
{
    CC_SAFE_RETAIN(pShaderProgram);
    CC_SAFE_RELEASE(m_pShaderProgram);
    m_pShaderProgram = pShaderProgram;
}

void CCTexture2D::releaseData(void *data)
{
    free(data);
}

void* CCTexture2D::keepData(void *data, unsigned int length)
{
    CC_UNUSED_PARAM(length);
    //The texture data mustn't be saved because it isn't a mutable texture.
    return data;
}

bool CCTexture2D::hasPremultipliedAlpha()
{
    return m_bHasPremultipliedAlpha;
}

bool CCTexture2D::initWithData(const void *data, CCTexture2DPixelFormat pixelFormat, unsigned int pixelsWide, unsigned int pixelsHigh, const CCSize& contentSize)
{
    int formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
	int dataSizeByte = 4;
	/*==
	glPixelStorei(CC_UNPACK_ALIGNMENT,1);
	glGenTextures(1, &m_uName);
	glBindTexture(CC_TEXTURE_2D, m_uName);
	==*/
	this->setAntiAliasTexParameters();
	
	// Specify OpenGL texture image
	switch(pixelFormat)
	{
	case kCCTexture2DPixelFormat_RGBA8888:
		formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
		dataSizeByte = 4;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGBA, CC_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_RGB888:
		dataSizeByte = 4;
		formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGB, CC_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_RGBA4444:
		dataSizeByte = 4;
		formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGBA, CC_UNSIGNED_SHORT_4_4_4_4, data);
		break;
	case kCCTexture2DPixelFormat_RGB5A1:
		dataSizeByte = 2;
		formatTmp = DXGI_FORMAT_B5G5R5A1_UNORM;
		//info.Format = DXGI_FORMAT_B5G5R5A1_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGBA, CC_UNSIGNED_SHORT_5_5_5_1, data);
		break;
	case kCCTexture2DPixelFormat_RGB565:
		dataSizeByte = 2;
		formatTmp = DXGI_FORMAT_B5G6R5_UNORM;
		//info.Format = DXGI_FORMAT_B5G6R5_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGB, CC_UNSIGNED_SHORT_5_6_5, data);
		break;
	case kCCTexture2DPixelFormat_AI88:
		dataSizeByte = 2;
		formatTmp = DXGI_FORMAT_R8G8_UNORM;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_LUMINANCE_ALPHA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_LUMINANCE_ALPHA, CC_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_A8:
		dataSizeByte = 1;
		formatTmp = DXGI_FORMAT_A8_UNORM;
		//info.Format = DXGI_FORMAT_A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_ALPHA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_ALPHA, CC_UNSIGNED_BYTE, data);
		break;
	default:
		CCAssert(0, "NSInternalInconsistencyException");

	}
	ID3D11Device *pdevice = CCDirector::sharedDirector()->getOpenGLView()->GetDevice();
	ID3D11Texture2D *tex;
	D3D11_TEXTURE2D_DESC tdesc;
	D3D11_SUBRESOURCE_DATA tbsd;
	tbsd.pSysMem = data;
	tbsd.SysMemPitch = pixelsWide*dataSizeByte;
	tbsd.SysMemSlicePitch = pixelsWide*pixelsHigh*dataSizeByte; // Not needed since this is a 2d texture

	tdesc.Width = pixelsWide;
	tdesc.Height = pixelsHigh;
	tdesc.MipLevels = 1;
	tdesc.ArraySize = 1;

	tdesc.SampleDesc.Count = 1;
	tdesc.SampleDesc.Quality = 0;
	tdesc.Usage = D3D11_USAGE_DEFAULT;
	tdesc.Format = (DXGI_FORMAT)formatTmp;
	tdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	tdesc.CPUAccessFlags = 0;
	tdesc.MiscFlags = 0;
	
	if(FAILED(pdevice->CreateTexture2D(&tdesc,&tbsd,&tex)))
	{
		// e_fail
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D11_TEXTURE2D_DESC desc;
	// Get a texture description to determine the texture
	// format of the loaded texture.
	tex->GetDesc( &desc );

	// Fill in the D3D11_SHADER_RESOURCE_VIEW_DESC structure.
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;

	// Create the shader resource view.
	pdevice->CreateShaderResourceView( tex, &srvDesc, &m_pTextureResource );
	m_uName = (CCuint)m_pTextureResource;
	if ( tex )
	{
		tex->Release();
		tex = 0;
	}
	
	m_tContentSize = contentSize;
	m_uPixelsWide = pixelsWide;
	m_uPixelsHigh = pixelsHigh;
	m_ePixelFormat = pixelFormat;
	m_fMaxS = contentSize.width / (float)(pixelsWide);
	m_fMaxT = contentSize.height / (float)(pixelsHigh);

	m_bHasPremultipliedAlpha = false;

	//m_eResolutionType = kCCResolutionUnknown;

	return true;
}


const char* CCTexture2D::description(void)
{
    return CCString::createWithFormat("<CCTexture2D | Name = %u | Dimensions = %u x %u | Coordinates = (%.2f, %.2f)>", m_uName, m_uPixelsWide, m_uPixelsHigh, m_fMaxS, m_fMaxT)->getCString();
}

// implementation CCTexture2D (Image)

bool CCTexture2D::initWithImage(CCImage *uiImage)
{
    if (uiImage == NULL)
    {
        CCLOG("cocos2d: CCTexture2D. Can't create Texture. UIImage is nil");
        this->release();
        return false;
    }
    
    unsigned int imageWidth = uiImage->getWidth();
    unsigned int imageHeight = uiImage->getHeight();
    
    CCConfiguration *conf = CCConfiguration::sharedConfiguration();
    
    unsigned maxTextureSize = conf->getMaxTextureSize();
    if (imageWidth > maxTextureSize || imageHeight > maxTextureSize) 
    {
        CCLOG("cocos2d: WARNING: Image (%u x %u) is bigger than the supported %u x %u", imageWidth, imageHeight, maxTextureSize, maxTextureSize);
        this->release();
        return NULL;
    }
    
    // always load premultiplied images
    return initPremultipliedATextureWithImage(uiImage, imageWidth, imageHeight);
}

bool CCTexture2D::initPremultipliedATextureWithImage(CCImage *image, unsigned int width, unsigned int height)
{
    unsigned char*            tempData = image->getData();
    unsigned int*             inPixel32 = NULL;
    unsigned char*            inPixel8 = NULL;
    unsigned short*           outPixel16 = NULL;
    bool                      hasAlpha = image->hasAlpha();
    CCSize                    imageSize = CCSizeMake((float)(image->getWidth()), (float)(image->getHeight()));
    CCTexture2DPixelFormat    pixelFormat;
    size_t                    bpp = image->getBitsPerComponent();

    // compute pixel format
    if(hasAlpha)
    {
        pixelFormat = g_defaultAlphaPixelFormat;
    }
    else
    {
        if (bpp >= 8)
        {
            pixelFormat = kCCTexture2DPixelFormat_RGB888;
        }
        else 
        {
            pixelFormat = kCCTexture2DPixelFormat_RGB565;
        }
        
    }
    
    // Repack the pixel data into the right format
    unsigned int length = width * height;
    
    if (pixelFormat == kCCTexture2DPixelFormat_RGB565)
    {
        if (hasAlpha)
        {
            // Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRGGGGGGBBBBB"
            
            tempData = new unsigned char[width * height * 2];
            outPixel16 = (unsigned short*)tempData;
            inPixel32 = (unsigned int*)image->getData();
            
            for(unsigned int i = 0; i < length; ++i, ++inPixel32)
            {
                *outPixel16++ = 
                ((((*inPixel32 >>  0) & 0xFF) >> 3) << 11) |  // R
                ((((*inPixel32 >>  8) & 0xFF) >> 2) << 5)  |  // G
                ((((*inPixel32 >> 16) & 0xFF) >> 3) << 0);    // B
            }
        }
        else 
        {
            // Convert "RRRRRRRRRGGGGGGGGBBBBBBBB" to "RRRRRGGGGGGBBBBB"
            
            tempData = new unsigned char[width * height * 2];
            outPixel16 = (unsigned short*)tempData;
            inPixel8 = (unsigned char*)image->getData();
            
            for(unsigned int i = 0; i < length; ++i)
            {
                *outPixel16++ = 
                (((*inPixel8++ & 0xFF) >> 3) << 11) |  // R
                (((*inPixel8++ & 0xFF) >> 2) << 5)  |  // G
                (((*inPixel8++ & 0xFF) >> 3) << 0);    // B
            }
        }    
    }
    else if (pixelFormat == kCCTexture2DPixelFormat_RGBA4444)
    {
        // Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRGGGGBBBBAAAA"
        
        inPixel32 = (unsigned int*)image->getData();  
        tempData = new unsigned char[width * height * 2];
        outPixel16 = (unsigned short*)tempData;
        
        for(unsigned int i = 0; i < length; ++i, ++inPixel32)
        {
            *outPixel16++ = 
            ((((*inPixel32 >> 0) & 0xFF) >> 4) << 12) | // R
            ((((*inPixel32 >> 8) & 0xFF) >> 4) <<  8) | // G
            ((((*inPixel32 >> 16) & 0xFF) >> 4) << 4) | // B
            ((((*inPixel32 >> 24) & 0xFF) >> 4) << 0);  // A
        }
    }
    else if (pixelFormat == kCCTexture2DPixelFormat_RGB5A1)
    {
        // Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRGGGGGBBBBBA"
        inPixel32 = (unsigned int*)image->getData();   
        tempData = new unsigned char[width * height * 2];
        outPixel16 = (unsigned short*)tempData;
        
        for(unsigned int i = 0; i < length; ++i, ++inPixel32)
        {
            *outPixel16++ = 
            ((((*inPixel32 >> 0) & 0xFF) >> 3) << 11) | // R
            ((((*inPixel32 >> 8) & 0xFF) >> 3) <<  6) | // G
            ((((*inPixel32 >> 16) & 0xFF) >> 3) << 1) | // B
            ((((*inPixel32 >> 24) & 0xFF) >> 7) << 0);  // A
        }
    }
    else if (pixelFormat == kCCTexture2DPixelFormat_A8)
    {
        // Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "AAAAAAAA"
        inPixel32 = (unsigned int*)image->getData();
        tempData = new unsigned char[width * height];
        unsigned char *outPixel8 = tempData;
        
        for(unsigned int i = 0; i < length; ++i, ++inPixel32)
        {
            *outPixel8++ = (*inPixel32 >> 24) & 0xFF;  // A
        }
    }
    
    if (hasAlpha && pixelFormat == kCCTexture2DPixelFormat_RGB888)
    {
        // Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRRRRGGGGGGGGBBBBBBBB"
        inPixel32 = (unsigned int*)image->getData();
        tempData = new unsigned char[width * height * 3];
        unsigned char *outPixel8 = tempData;
        
        for(unsigned int i = 0; i < length; ++i, ++inPixel32)
        {
            *outPixel8++ = (*inPixel32 >> 0) & 0xFF; // R
            *outPixel8++ = (*inPixel32 >> 8) & 0xFF; // G
            *outPixel8++ = (*inPixel32 >> 16) & 0xFF; // B
        }
    }
    
    initWithData(tempData, pixelFormat, width, height, imageSize);
    
    if (tempData != image->getData())
    {
        delete [] tempData;
    }

    m_bHasPremultipliedAlpha = image->isPremultipliedAlpha();
    return true;
}

// implementation CCTexture2D (Text)
bool CCTexture2D::initWithString(const char *text, const char *fontName, float fontSize)
{
    return initWithString(text,  fontName, fontSize, CCSizeMake(0,0), kCCTextAlignmentCenter, kCCVerticalTextAlignmentTop);
}

bool CCTexture2D::initWithString(const char *text, const char *fontName, float fontSize, const CCSize& dimensions, CCTextAlignment hAlignment, CCVerticalTextAlignment vAlignment)
{
#if CC_ENABLE_CACHE_TEXTURE_DATA
    // cache the texture data
    VolatileTexture::addStringTexture(this, text, dimensions, hAlignment, vAlignment, fontName, fontSize);
#endif

    bool bRet = false;
    CCImage::ETextAlign eAlign;

    if (kCCVerticalTextAlignmentTop == vAlignment)
    {
        eAlign = (kCCTextAlignmentCenter == hAlignment) ? CCImage::kAlignTop
            : (kCCTextAlignmentLeft == hAlignment) ? CCImage::kAlignTopLeft : CCImage::kAlignTopRight;
    }
    else if (kCCVerticalTextAlignmentCenter == vAlignment)
    {
        eAlign = (kCCTextAlignmentCenter == hAlignment) ? CCImage::kAlignCenter
            : (kCCTextAlignmentLeft == hAlignment) ? CCImage::kAlignLeft : CCImage::kAlignRight;
    }
    else if (kCCVerticalTextAlignmentBottom == vAlignment)
    {
        eAlign = (kCCTextAlignmentCenter == hAlignment) ? CCImage::kAlignBottom
            : (kCCTextAlignmentLeft == hAlignment) ? CCImage::kAlignBottomLeft : CCImage::kAlignBottomRight;
    }
    else
    {
        CCAssert(false, "Not supported alignment format!");
    }
    
    do 
    {
        CCImage* pImage = new CCImage();
        CC_BREAK_IF(NULL == pImage);
        bRet = pImage->initWithString(text, (int)dimensions.width, (int)dimensions.height, eAlign, fontName, (int)fontSize);
        CC_BREAK_IF(!bRet);
        bRet = initWithImage(pImage);
        CC_SAFE_RELEASE(pImage);
    } while (0);
    
    return bRet;
}


// implementation CCTexture2D (Drawing)

void CCTexture2D::drawAtPoint(const CCPoint& point)
{
    //GLfloat    coordinates[] = {    
    //    0.0f,    m_fMaxT,
    //    m_fMaxS,m_fMaxT,
    //    0.0f,    0.0f,
    //    m_fMaxS,0.0f };

    //GLfloat    width = (GLfloat)m_uPixelsWide * m_fMaxS,
    //    height = (GLfloat)m_uPixelsHigh * m_fMaxT;

    //GLfloat        vertices[] = {    
    //    point.x,            point.y,
    //    width + point.x,    point.y,
    //    point.x,            height  + point.y,
    //    width + point.x,    height  + point.y };

    //ccGLEnableVertexAttribs( kCCVertexAttribFlag_Position | kCCVertexAttribFlag_TexCoords );
    //m_pShaderProgram->use();
    //m_pShaderProgram->setUniformsForBuiltins();

    //ccGLBindTexture2D( m_uName );


    //glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    //glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, coordinates);

    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CCTexture2D::drawInRect(const CCRect& rect)
{
    //GLfloat    coordinates[] = {    
    //    0.0f,    m_fMaxT,
    //    m_fMaxS,m_fMaxT,
    //    0.0f,    0.0f,
    //    m_fMaxS,0.0f };

    //GLfloat    vertices[] = {    rect.origin.x,        rect.origin.y,                            /*0.0f,*/
    //    rect.origin.x + rect.size.width,        rect.origin.y,                            /*0.0f,*/
    //    rect.origin.x,                            rect.origin.y + rect.size.height,        /*0.0f,*/
    //    rect.origin.x + rect.size.width,        rect.origin.y + rect.size.height,        /*0.0f*/ };

    //ccGLEnableVertexAttribs( kCCVertexAttribFlag_Position | kCCVertexAttribFlag_TexCoords );
    //m_pShaderProgram->use();
    //m_pShaderProgram->setUniformsForBuiltins();

    //ccGLBindTexture2D( m_uName );

    //glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    //glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, coordinates);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef CC_SUPPORT_PVRTC
// implementation CCTexture2D (PVRTC);    
bool CCTexture2D::initWithPVRTCData(const void *data, int level, int bpp, bool hasAlpha, int length, CCTexture2DPixelFormat pixelFormat)
{
    if( !(CCConfiguration::sharedConfiguration()->supportsPVRTC()) )
    {
        CCLOG("cocos2d: WARNING: PVRTC images is not supported.");
        this->release();
        return false;
    }

    glGenTextures(1, &m_uName);
    glBindTexture(GL_TEXTURE_2D, m_uName);

    this->setAntiAliasTexParameters();

    GLenum format;
    GLsizei size = length * length * bpp / 8;
    if(hasAlpha) {
        format = (bpp == 4) ? GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
    } else {
        format = (bpp == 4) ? GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
    }
    if(size < 32) {
        size = 32;
    }
    glCompressedTexImage2D(GL_TEXTURE_2D, level, format, length, length, 0, size, data);

    m_tContentSize = CCSizeMake((float)(length), (float)(length));
    m_uPixelsWide = length;
    m_uPixelsHigh = length;
    m_fMaxS = 1.0f;
    m_fMaxT = 1.0f;
    m_bHasPremultipliedAlpha = PVRHaveAlphaPremultiplied_;
    m_ePixelFormat = pixelFormat;

    return true;
}
#endif // CC_SUPPORT_PVRTC

bool CCTexture2D::initWithPVRFile(const char* file)
{
    bool bRet = false;
    // nothing to do with CCObject::init
    
    CCTexturePVR *pvr = new CCTexturePVR;
    bRet = pvr->initWithContentsOfFile(file);
        
    if (bRet)
    {
        pvr->setRetainName(true); // don't dealloc texture on release
        
        m_uName = pvr->getName();
        m_fMaxS = 1.0f;
        m_fMaxT = 1.0f;
        m_uPixelsWide = pvr->getWidth();
        m_uPixelsHigh = pvr->getHeight();
        m_tContentSize = CCSizeMake((float)m_uPixelsWide, (float)m_uPixelsHigh);
        m_bHasPremultipliedAlpha = PVRHaveAlphaPremultiplied_;
        m_ePixelFormat = pvr->getFormat();
        m_bHasMipmaps = pvr->getNumberOfMipmaps() > 1;       

        pvr->release();
    }
    else
    {
        CCLOG("cocos2d: Couldn't load PVR image %s", file);
    }

    return bRet;
}

void CCTexture2D::PVRImagesHavePremultipliedAlpha(bool haveAlphaPremultiplied)
{
    PVRHaveAlphaPremultiplied_ = haveAlphaPremultiplied;
}

    
//
// Use to apply MIN/MAG filter
//
// implementation CCTexture2D (GLFilter)

void CCTexture2D::generateMipmap()
{
    CCAssert( m_uPixelsWide == ccNextPOT(m_uPixelsWide) && m_uPixelsHigh == ccNextPOT(m_uPixelsHigh), "Mipmap texture only works in POT textures");
    //ccGLBindTexture2D( m_uName );
    //glGenerateMipmap(GL_TEXTURE_2D);
    //m_bHasMipmaps = true;
}

bool CCTexture2D::hasMipmaps()
{
    return m_bHasMipmaps;
}

void CCTexture2D::setTexParameters(ccTexParams *texParams)
{
    CCAssert( (m_uPixelsWide == ccNextPOT(m_uPixelsWide) && m_uPixelsHigh == ccNextPOT(m_uPixelsHigh)) ||
		(texParams->wrapS == CC_CLAMP_TO_EDGE && texParams->wrapT == CC_CLAMP_TO_EDGE),
		"CC_CLAMP_TO_EDGE should be used in NPOT textures");

	D3D11_SAMPLER_DESC samplerDesc;
	int filter = -1;
	int u = -1;
	int v = -1;
	bool bcreat = true;
	ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
	if ( m_sampleState )
	{
		m_sampleState->GetDesc(&samplerDesc);
		filter = samplerDesc.Filter;
		u = samplerDesc.AddressU;
		v = samplerDesc.AddressV;
	}

	if ( texParams->magFilter == CC_NEAREST )
	{
		switch(texParams->minFilter)
		{
		case CC_NEAREST:
			filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;
		case CC_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case CC_NEAREST_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case CC_LINEAR_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case CC_NEAREST_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case CC_LINEAR_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		}
	}

	if ( texParams->magFilter == CC_LINEAR )
	{
		switch(texParams->minFilter)
		{
		case CC_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case CC_LINEAR:
			filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case CC_NEAREST_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case CC_LINEAR_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case CC_NEAREST_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case CC_LINEAR_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		}
	}
	
	if ( texParams->wrapS == CC_REPEAT )
	{
		u = D3D11_TEXTURE_ADDRESS_WRAP;
	}
	else if ( texParams->wrapS == CC_CLAMP_TO_EDGE )
	{
		u = D3D11_TEXTURE_ADDRESS_CLAMP;
	}
	
	if ( texParams->wrapT == CC_REPEAT )
	{
		v = D3D11_TEXTURE_ADDRESS_WRAP;
	}
	else if ( texParams->wrapT == CC_CLAMP_TO_EDGE )
	{
		v = D3D11_TEXTURE_ADDRESS_CLAMP;
	}

	if ( (filter==samplerDesc.Filter) && (u==samplerDesc.AddressU) && (v==samplerDesc.AddressV) )
	{
		bcreat = false;
	}
	else
	{
		if ( m_sampleState )
		{
			m_sampleState->Release();
			m_sampleState = 0;
		}
	}

	samplerDesc.Filter = (D3D11_FILTER)filter;
	samplerDesc.AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(u);
	samplerDesc.AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(v);
	samplerDesc.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(u);

	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if ( bcreat )
	{
		CCID3D11Device->CreateSamplerState(&samplerDesc, &m_sampleState);
	}
}

void CCTexture2D::setAliasTexParameters()
{
    ccTexParams texParams = { CC_NEAREST, CC_NEAREST, CC_CLAMP_TO_EDGE, CC_CLAMP_TO_EDGE };
	this->setTexParameters(&texParams);
}

void CCTexture2D::setAntiAliasTexParameters()
{
    ccTexParams texParams = { CC_LINEAR, CC_LINEAR, CC_CLAMP_TO_EDGE, CC_CLAMP_TO_EDGE };
	this->setTexParameters(&texParams);
}

const char* CCTexture2D::stringForFormat()
{
	switch (m_ePixelFormat) 
	{
		case kCCTexture2DPixelFormat_RGBA8888:
			return  "RGBA8888";

		case kCCTexture2DPixelFormat_RGB888:
			return  "RGB888";

		case kCCTexture2DPixelFormat_RGB565:
			return  "RGB565";

		case kCCTexture2DPixelFormat_RGBA4444:
			return  "RGBA4444";

		case kCCTexture2DPixelFormat_RGB5A1:
			return  "RGB5A1";

		case kCCTexture2DPixelFormat_AI88:
			return  "AI88";

		case kCCTexture2DPixelFormat_A8:
			return  "A8";

		case kCCTexture2DPixelFormat_I8:
			return  "I8";

		case kCCTexture2DPixelFormat_PVRTC4:
			return  "PVRTC4";

		case kCCTexture2DPixelFormat_PVRTC2:
			return  "PVRTC2";

		default:
			CCAssert(false , "unrecognized pixel format");
			CCLOG("stringForFormat: %ld, cannot give useful result", (long)m_ePixelFormat);
			break;
	}

	return  NULL;
}


//
// Texture options for images that contains alpha
//
// implementation CCTexture2D (PixelFormat)

void CCTexture2D::setDefaultAlphaPixelFormat(CCTexture2DPixelFormat format)
{
    g_defaultAlphaPixelFormat = format;
}


CCTexture2DPixelFormat CCTexture2D::defaultAlphaPixelFormat()
{
    return g_defaultAlphaPixelFormat;
}

unsigned int CCTexture2D::bitsPerPixelForFormat(CCTexture2DPixelFormat format)
{
	unsigned int ret=0;

	switch (format) {
		case kCCTexture2DPixelFormat_RGBA8888:
			ret = 32;
			break;
		case kCCTexture2DPixelFormat_RGB888:
			// It is 32 and not 24, since its internal representation uses 32 bits.
			ret = 32;
			break;
		case kCCTexture2DPixelFormat_RGB565:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_RGBA4444:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_RGB5A1:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_AI88:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_A8:
			ret = 8;
			break;
		case kCCTexture2DPixelFormat_I8:
			ret = 8;
			break;
		case kCCTexture2DPixelFormat_PVRTC4:
			ret = 4;
			break;
		case kCCTexture2DPixelFormat_PVRTC2:
			ret = 2;
			break;
		default:
			ret = -1;
			CCAssert(false , "unrecognized pixel format");
			CCLOG("bitsPerPixelForFormat: %ld, cannot give useful result", (long)format);
			break;
	}
	return ret;
}

unsigned int CCTexture2D::bitsPerPixelForFormat()
{
	return this->bitsPerPixelForFormat(m_ePixelFormat);
}


NS_CC_END
