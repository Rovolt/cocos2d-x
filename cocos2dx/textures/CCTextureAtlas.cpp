/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2011      Zynga Inc.

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

// cocos2d
#include "CCTextureAtlas.h"
#include "CCTextureCache.h"
#include "ccMacros.h"
#include "shaders/CCGLProgram.h"
#include "shaders/ccGLStateCache.h"
#include "support/CCNotificationCenter.h"
#include "CCEventType.h"
#include "CCGL.h"
// support
#include "CCTexture2D.h"
#include "cocoa/CCString.h"
#include <stdlib.h>
#include "DirectXHelper.h"

#include <fstream>
#include "BasicLoader.h"
#include "CCEGLView.h"
#include "CCDirector.h"

using namespace DirectX;
using namespace std;
//According to some tests GL_TRIANGLE_STRIP is slower, MUCH slower. Probably I'm doing something very wrong

// implementation CCTextureAtlas

NS_CC_BEGIN

CCTextureAtlas::CCTextureAtlas()
    :m_pIndices(NULL)
    ,m_bDirty(false)
    ,m_pTexture(NULL)
    ,m_pQuads(NULL)
{}
CCDXTextureAtlas CCTextureAtlas::mDXTextureAtlas;
CCTextureAtlas::~CCTextureAtlas()
{
    CCLOGINFO("cocos2d: CCTextureAtlas deallocing %p.", this);

    CC_SAFE_FREE(m_pQuads);
    CC_SAFE_FREE(m_pIndices);

//    glDeleteBuffers(2, m_pBuffersVBO);
//
//#if CC_TEXTURE_ATLAS_USE_VAO
//    glDeleteVertexArrays(1, &m_uVAOname);
//#endif
    CC_SAFE_RELEASE(m_pTexture);
    
    CCNotificationCenter::sharedNotificationCenter()->removeObserver(this, EVNET_COME_TO_FOREGROUND);
}

unsigned int CCTextureAtlas::getTotalQuads()
{
    return m_uTotalQuads;
}

unsigned int CCTextureAtlas::getCapacity()
{
    return m_uCapacity;
}

CCTexture2D* CCTextureAtlas::getTexture()
{
    return m_pTexture;
}

void CCTextureAtlas::setTexture(CCTexture2D * var)
{
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE(m_pTexture);
    m_pTexture = var;
}

ccV3F_C4B_T2F_Quad* CCTextureAtlas::getQuads()
{
    //if someone accesses the quads directly, presume that changes will be made
    m_bDirty = true;
    return m_pQuads;
}

void CCTextureAtlas::setQuads(ccV3F_C4B_T2F_Quad *var)
{
    m_pQuads = var;
}

// TextureAtlas - alloc & init

CCTextureAtlas * CCTextureAtlas::create(const char* file, unsigned int capacity)
{
    CCTextureAtlas * pTextureAtlas = new CCTextureAtlas();
    if(pTextureAtlas && pTextureAtlas->initWithFile(file, capacity))
    {
        pTextureAtlas->autorelease();
        return pTextureAtlas;
    }
    CC_SAFE_DELETE(pTextureAtlas);
    return NULL;
}

CCTextureAtlas * CCTextureAtlas::createWithTexture(CCTexture2D *texture, unsigned int capacity)
{
    CCTextureAtlas * pTextureAtlas = new CCTextureAtlas();
    if (pTextureAtlas && pTextureAtlas->initWithTexture(texture, capacity))
    {
        pTextureAtlas->autorelease();
        return pTextureAtlas;
    }
    CC_SAFE_DELETE(pTextureAtlas);
    return NULL;
}

bool CCTextureAtlas::initWithFile(const char * file, unsigned int capacity)
{
    // retained in property
    CCTexture2D *texture = CCTextureCache::sharedTextureCache()->addImage(file);

    if (texture)
    {
        return initWithTexture(texture, capacity);
    }
    else
    {
        CCLOG("cocos2d: Could not open file: %s", file);
        return false;
    }
}

bool CCTextureAtlas::initWithTexture(CCTexture2D *texture, unsigned int capacity)
{
//    CCAssert(texture != NULL, "texture should not be null");
    m_uCapacity = capacity;
    m_uTotalQuads = 0;

    // retained in property
    this->m_pTexture = texture;
    CC_SAFE_RETAIN(m_pTexture);

    // Re-initialization is not allowed
    CCAssert(m_pQuads == NULL && m_pIndices == NULL, "");

    m_pQuads = (ccV3F_C4B_T2F_Quad*)malloc( m_uCapacity * sizeof(ccV3F_C4B_T2F_Quad) );
    m_pIndices = (GLushort *)malloc( m_uCapacity * 6 * sizeof(GLushort) );
    
    if( ! ( m_pQuads && m_pIndices) && m_uCapacity > 0) 
    {
        //CCLOG("cocos2d: CCTextureAtlas: not enough memory");
        CC_SAFE_FREE(m_pQuads);
        CC_SAFE_FREE(m_pIndices);

        // release texture, should set it to null, because the destruction will
        // release it too. see cocos2d-x issue #484
        CC_SAFE_RELEASE_NULL(m_pTexture);
        return false;
    }

    memset( m_pQuads, 0, m_uCapacity * sizeof(ccV3F_C4B_T2F_Quad) );
    memset( m_pIndices, 0, m_uCapacity * 6 * sizeof(GLushort) );
    
    // listen the event when app go to background
    CCNotificationCenter::sharedNotificationCenter()->addObserver(this,
                                                           callfuncO_selector(CCTextureAtlas::listenBackToForeground),
                                                           EVNET_COME_TO_FOREGROUND,
                                                           NULL);

    this->setupIndices();

#if CC_TEXTURE_ATLAS_USE_VAO
    setupVBOandVAO();    
#else    
    setupVBO();
#endif

    m_bDirty = true;

    return true;
}

void CCTextureAtlas::listenBackToForeground(CCObject *obj)
{  
#if CC_TEXTURE_ATLAS_USE_VAO
    setupVBOandVAO();    
#else    
    setupVBO();
#endif
    
    // set m_bDirty to true to force it rebinding buffer
    m_bDirty = true;
}

const char* CCTextureAtlas::description()
{
    return CCString::createWithFormat("<CCTextureAtlas | totalQuads = %u>", m_uTotalQuads)->getCString();
}


void CCTextureAtlas::setupIndices()
{
if (m_uCapacity == 0)
		return;

	for( unsigned int i=0; i < m_uCapacity; i++)
	{
#if CC_TEXTURE_ATLAS_USE_TRIANGLE_STRIP
		m_pIndices[i*6+0] = i*4+0;
		m_pIndices[i*6+1] = i*4+0;
		m_pIndices[i*6+2] = i*4+2;		
		m_pIndices[i*6+3] = i*4+1;
		m_pIndices[i*6+4] = i*4+3;
		m_pIndices[i*6+5] = i*4+3;
#else
		m_pIndices[i*6+0] = (CCushort)(i*4+0);
		m_pIndices[i*6+1] = (CCushort)(i*4+1);
		m_pIndices[i*6+2] = (CCushort)(i*4+2);

		// inverted index. issue #179
		//m_pIndices[i*6+3] = (CCushort)(i*4+3);
		//m_pIndices[i*6+4] = (CCushort)(i*4+2);
		//m_pIndices[i*6+5] = (CCushort)(i*4+1);		
		//		m_pIndices[i*6+3] = i*4+2;
		//		m_pIndices[i*6+4] = i*4+3;
		//		m_pIndices[i*6+5] = i*4+1;

		// !!!!!!dx!!!!!!!!
		m_pIndices[i*6+3] = (CCushort)(i*4+0);
		m_pIndices[i*6+4] = (CCushort)(i*4+2);
		m_pIndices[i*6+5] = (CCushort)(i*4+3);
#endif	
	}

}

//TextureAtlas - VAO / VBO specific

#if CC_TEXTURE_ATLAS_USE_VAO
void CCTextureAtlas::setupVBOandVAO()
{
    glGenVertexArrays(1, &m_uVAOname);
    ccGLBindVAO(m_uVAOname);

#define kQuadSize sizeof(m_pQuads[0].bl)

    glGenBuffers(2, &m_pBuffersVBO[0]);

    glBindBuffer(GL_ARRAY_BUFFER, m_pBuffersVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_pQuads[0]) * m_uCapacity, m_pQuads, GL_DYNAMIC_DRAW);

    // vertices
    glEnableVertexAttribArray(kCCVertexAttrib_Position);
    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, kQuadSize, (GLvoid*) offsetof( ccV3F_C4B_T2F, vertices));

    // colors
    glEnableVertexAttribArray(kCCVertexAttrib_Color);
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (GLvoid*) offsetof( ccV3F_C4B_T2F, colors));

    // tex coords
    glEnableVertexAttribArray(kCCVertexAttrib_TexCoords);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, kQuadSize, (GLvoid*) offsetof( ccV3F_C4B_T2F, texCoords));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pBuffersVBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_pIndices[0]) * m_uCapacity * 6, m_pIndices, GL_STATIC_DRAW);

    // Must unbind the VAO before changing the element buffer.
    ccGLBindVAO(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_DEBUG();
}
#else // CC_TEXTURE_ATLAS_USE_VAO
void CCTextureAtlas::setupVBO()
{
    //glGenBuffers(2, &m_pBuffersVBO[0]);

    mapBuffers();
}
#endif // ! // CC_TEXTURE_ATLAS_USE_VAO

void CCTextureAtlas::mapBuffers()
{
    // Avoid changing the element buffer for whatever VAO might be bound.
	/*ccGLBindVAO(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_pBuffersVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_pQuads[0]) * m_uCapacity, m_pQuads, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pBuffersVBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_pIndices[0]) * m_uCapacity * 6, m_pIndices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_DEBUG();*/
}

// TextureAtlas - Update, Insert, Move & Remove

void CCTextureAtlas::updateQuad(ccV3F_C4B_T2F_Quad *quad, unsigned int index)
{
    CCAssert( index >= 0 && index < m_uCapacity, "updateQuadWithTexture: Invalid index");

    m_uTotalQuads = MAX( index+1, m_uTotalQuads);

    m_pQuads[index] = *quad;    


    m_bDirty = true;

}

void CCTextureAtlas::insertQuad(ccV3F_C4B_T2F_Quad *quad, unsigned int index)
{
    CCAssert( index < m_uCapacity, "insertQuadWithTexture: Invalid index");

    m_uTotalQuads++;
    CCAssert( m_uTotalQuads <= m_uCapacity, "invalid totalQuads");

    // issue #575. index can be > totalQuads
    unsigned int remaining = (m_uTotalQuads-1) - index;

    // last object doesn't need to be moved
    if( remaining > 0) 
    {
        // texture coordinates
        memmove( &m_pQuads[index+1],&m_pQuads[index], sizeof(m_pQuads[0]) * remaining );        
    }

    m_pQuads[index] = *quad;


    m_bDirty = true;

}

void CCTextureAtlas::insertQuads(ccV3F_C4B_T2F_Quad* quads, unsigned int index, unsigned int amount)
{
    CCAssert(index + amount <= m_uCapacity, "insertQuadWithTexture: Invalid index + amount");

    m_uTotalQuads += amount;

    CCAssert( m_uTotalQuads <= m_uCapacity, "invalid totalQuads");

    // issue #575. index can be > totalQuads
    int remaining = (m_uTotalQuads-1) - index - amount;

    // last object doesn't need to be moved
    if( remaining > 0)
    {
        // tex coordinates
        memmove( &m_pQuads[index+amount],&m_pQuads[index], sizeof(m_pQuads[0]) * remaining );
    }


    unsigned int max = index + amount;
    unsigned int j = 0;
    for (unsigned int i = index; i < max ; i++)
    {
        m_pQuads[index] = quads[j];
        index++;
        j++;
    }

    m_bDirty = true;
}

void CCTextureAtlas::insertQuadFromIndex(unsigned int oldIndex, unsigned int newIndex)
{
    CCAssert( newIndex >= 0 && newIndex < m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");
    CCAssert( oldIndex >= 0 && oldIndex < m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");

    if( oldIndex == newIndex )
    {
        return;
    }
    // because it is ambiguous in iphone, so we implement abs ourselves
    // unsigned int howMany = abs( oldIndex - newIndex);
    unsigned int howMany = (oldIndex - newIndex) > 0 ? (oldIndex - newIndex) :  (newIndex - oldIndex);
    unsigned int dst = oldIndex;
    unsigned int src = oldIndex + 1;
    if( oldIndex > newIndex)
    {
        dst = newIndex+1;
        src = newIndex;
    }

    // texture coordinates
    ccV3F_C4B_T2F_Quad quadsBackup = m_pQuads[oldIndex];
    memmove( &m_pQuads[dst],&m_pQuads[src], sizeof(m_pQuads[0]) * howMany );
    m_pQuads[newIndex] = quadsBackup;


    m_bDirty = true;

}

void CCTextureAtlas::removeQuadAtIndex(unsigned int index)
{
    CCAssert( index < m_uTotalQuads, "removeQuadAtIndex: Invalid index");

    unsigned int remaining = (m_uTotalQuads-1) - index;


    // last object doesn't need to be moved
    if( remaining ) 
    {
        // texture coordinates
        memmove( &m_pQuads[index],&m_pQuads[index+1], sizeof(m_pQuads[0]) * remaining );
    }

    m_uTotalQuads--;


    m_bDirty = true;

}

void CCTextureAtlas::removeQuadsAtIndex(unsigned int index, unsigned int amount)
{
    CCAssert(index + amount <= m_uTotalQuads, "removeQuadAtIndex: index + amount out of bounds");

    unsigned int remaining = (m_uTotalQuads) - (index + amount);

    m_uTotalQuads -= amount;

    if ( remaining )
    {
        memmove( &m_pQuads[index], &m_pQuads[index+amount], sizeof(m_pQuads[0]) * remaining );
    }

    m_bDirty = true;
}

void CCTextureAtlas::removeAllQuads()
{
    m_uTotalQuads = 0;
}

// TextureAtlas - Resize
bool CCTextureAtlas::resizeCapacity(unsigned int newCapacity)
{
    if( newCapacity == m_uCapacity )
    {
        return true;
    }
    unsigned int uOldCapactiy = m_uCapacity; 
    // update capacity and totolQuads
    m_uTotalQuads = MIN(m_uTotalQuads, newCapacity);
    m_uCapacity = newCapacity;

    ccV3F_C4B_T2F_Quad* tmpQuads = NULL;
    GLushort* tmpIndices = NULL;
    
    // when calling initWithTexture(fileName, 0) on bada device, calloc(0, 1) will fail and return NULL,
    // so here must judge whether m_pQuads and m_pIndices is NULL.
    if (m_pQuads == NULL)
    {
        tmpQuads = (ccV3F_C4B_T2F_Quad*)malloc( m_uCapacity * sizeof(m_pQuads[0]) );
        if (tmpQuads != NULL)
        {
            memset(tmpQuads, 0, m_uCapacity * sizeof(m_pQuads[0]) );
        }
    }
    else
    {
        tmpQuads = (ccV3F_C4B_T2F_Quad*)realloc( m_pQuads, sizeof(m_pQuads[0]) * m_uCapacity );
        if (tmpQuads != NULL && m_uCapacity > uOldCapactiy)
        {
            memset(tmpQuads+uOldCapactiy, 0, (m_uCapacity - uOldCapactiy)*sizeof(m_pQuads[0]) );
        }
    }

    if (m_pIndices == NULL)
    {    
        tmpIndices = (GLushort*)malloc( m_uCapacity * 6 * sizeof(m_pIndices[0]) );
        if (tmpIndices != NULL)
        {
            memset( tmpIndices, 0, m_uCapacity * 6 * sizeof(m_pIndices[0]) );
        }
        
    }
    else
    {
        tmpIndices = (GLushort*)realloc( m_pIndices, sizeof(m_pIndices[0]) * m_uCapacity * 6 );
        if (tmpIndices != NULL && m_uCapacity > uOldCapactiy)
        {
            memset( tmpIndices+uOldCapactiy, 0, (m_uCapacity-uOldCapactiy) * 6 * sizeof(m_pIndices[0]) );
        }
    }

    if( ! ( tmpQuads && tmpIndices) ) {
        CCLOG("cocos2d: CCTextureAtlas: not enough memory");
        CC_SAFE_FREE(tmpQuads);
        CC_SAFE_FREE(tmpIndices);
        CC_SAFE_FREE(m_pQuads);
        CC_SAFE_FREE(m_pIndices);
        m_uCapacity = m_uTotalQuads = 0;
        return false;
    }

    m_pQuads = tmpQuads;
    m_pIndices = tmpIndices;


    setupIndices();
    mapBuffers();

    m_bDirty = true;

    return true;
}

void CCTextureAtlas::increaseTotalQuadsWith(unsigned int amount)
{
    m_uTotalQuads += amount;
}

void CCTextureAtlas::moveQuadsFromIndex(unsigned int oldIndex, unsigned int amount, unsigned int newIndex)
{
    CCAssert(newIndex + amount <= m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");
    CCAssert(oldIndex < m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");

    if( oldIndex == newIndex )
    {
        return;
    }
    //create buffer
    size_t quadSize = sizeof(ccV3F_C4B_T2F_Quad);
    ccV3F_C4B_T2F_Quad* tempQuads = (ccV3F_C4B_T2F_Quad*)malloc( quadSize * amount);
    memcpy( tempQuads, &m_pQuads[oldIndex], quadSize * amount );

    if (newIndex < oldIndex)
    {
        // move quads from newIndex to newIndex + amount to make room for buffer
        memmove( &m_pQuads[newIndex], &m_pQuads[newIndex+amount], (oldIndex-newIndex)*quadSize);
    }
    else
    {
        // move quads above back
        memmove( &m_pQuads[oldIndex], &m_pQuads[oldIndex+amount], (newIndex-oldIndex)*quadSize);
    }
    memcpy( &m_pQuads[newIndex], tempQuads, amount*quadSize);

    free(tempQuads);

    m_bDirty = true;
}

void CCTextureAtlas::moveQuadsFromIndex(unsigned int index, unsigned int newIndex)
{
    CCAssert(newIndex + (m_uTotalQuads - index) <= m_uCapacity, "moveQuadsFromIndex move is out of bounds");

    memmove(m_pQuads + newIndex,m_pQuads + index, (m_uTotalQuads - index) * sizeof(m_pQuads[0]));
}

void CCTextureAtlas::fillWithEmptyQuadsFromIndex(unsigned int index, unsigned int amount)
{
    ccV3F_C4B_T2F_Quad quad;
    memset(&quad, 0, sizeof(quad));

    unsigned int to = index + amount;
    for (unsigned int i = index ; i < to ; i++)
    {
        m_pQuads[i] = quad;
    }
}

// TextureAtlas - Drawing

void CCTextureAtlas::drawQuads()
{
    this->drawNumberOfQuads(m_uTotalQuads, 0);
}

void CCTextureAtlas::drawNumberOfQuads(unsigned int n)
{
    this->drawNumberOfQuads(n, 0);
}

void CCTextureAtlas::drawNumberOfQuads(unsigned int n, unsigned int start)
{    
    if (0 == n) 
    {
        return;
    }
    mDXTextureAtlas.Render(m_pQuads,m_pIndices,m_uCapacity,m_pTexture,n,start);
    CC_INCREMENT_GL_DRAWS(1);
}
//void CCTextureAtlas::drawQuads()
//{
//	this->drawNumberOfQuads(m_uTotalQuads, 0);
//}

//void CCTextureAtlas::drawNumberOfQuads(unsigned int n)
//{
//	this->drawNumberOfQuads(n, 0);
//}
//
//void CCTextureAtlas::drawNumberOfQuads(unsigned int n, unsigned int start)
//{	
//	if (0 == n)
//		return;
//
//	mDXTextureAtlas.Render(m_pQuads,m_pIndices,m_uCapacity,m_pTexture,n,start);
//}


//void CCTextureAtlas::SetColor(UINT r,UINT g,UINT b,UINT a)
//{
//	for ( int i=0; i<m_uCapacity; i++ )
//	{
//		m_pQuads[i].tl.colors.r = r;
//		m_pQuads[i].tl.colors.g = g;
//		m_pQuads[i].tl.colors.b = b;
//		m_pQuads[i].tl.colors.a = a;
//
//		m_pQuads[i].tr.colors.r = r;
//		m_pQuads[i].tr.colors.g = g;
//		m_pQuads[i].tr.colors.b = b;
//		m_pQuads[i].tr.colors.a = a;
//
//		m_pQuads[i].br.colors.r = r;
//		m_pQuads[i].br.colors.g = g;
//		m_pQuads[i].br.colors.b = b;
//		m_pQuads[i].br.colors.a = a;
//
//		m_pQuads[i].bl.colors.r = r;
//		m_pQuads[i].bl.colors.g = g;
//		m_pQuads[i].bl.colors.b = b;
//		m_pQuads[i].bl.colors.a = a;
//	}
//}


CCDXTextureAtlas::CCDXTextureAtlas()
{
	m_vertexShader = 0;
	m_pixelShader = 0;
	m_layout = 0;
	m_matrixBuffer = 0;
	m_indexBuffer = 0;
	m_vertexBuffer = 0;

	mIsInit = FALSE;
}
CCDXTextureAtlas::~CCDXTextureAtlas()
{
	FreeBuffer();
}
void CCDXTextureAtlas::FreeBuffer()
{
	CC_SAFE_RELEASE_NULL_DX(m_vertexBuffer);
	CC_SAFE_RELEASE_NULL_DX(m_indexBuffer);
	CC_SAFE_RELEASE_NULL_DX(m_matrixBuffer);
	CC_SAFE_RELEASE_NULL_DX(m_layout);
	CC_SAFE_RELEASE_NULL_DX(m_pixelShader);
	CC_SAFE_RELEASE_NULL_DX(m_vertexShader);
}
void CCDXTextureAtlas::setIsInit(bool isInit)
{
	mIsInit = isInit;
}

void CCDXTextureAtlas::RenderVertexBuffer(ccV3F_C4B_T2F_Quad* quads,unsigned int capacity)
{
	VertexType* verticesTmp = new VertexType[4*capacity];
	if(!verticesTmp)
	{
		return ;
	}

	for ( int i=0; i<capacity; i++ )
	{
		verticesTmp[4*i+0].position = XMFLOAT3(quads[i].tl.vertices.x, quads[i].tl.vertices.y, quads[i].tl.vertices.z);
		verticesTmp[4*i+1].position = XMFLOAT3(quads[i].tr.vertices.x, quads[i].tr.vertices.y, quads[i].tr.vertices.z);
		verticesTmp[4*i+2].position = XMFLOAT3(quads[i].br.vertices.x, quads[i].br.vertices.y, quads[i].br.vertices.z);
		verticesTmp[4*i+3].position = XMFLOAT3(quads[i].bl.vertices.x, quads[i].bl.vertices.y, quads[i].bl.vertices.z);

		verticesTmp[4*i+0].texture = XMFLOAT2(quads[i].tl.texCoords.u, quads[i].tl.texCoords.v);
		verticesTmp[4*i+1].texture = XMFLOAT2(quads[i].tr.texCoords.u, quads[i].tr.texCoords.v);
		verticesTmp[4*i+2].texture = XMFLOAT2(quads[i].br.texCoords.u, quads[i].br.texCoords.v);
		verticesTmp[4*i+3].texture = XMFLOAT2(quads[i].bl.texCoords.u, quads[i].bl.texCoords.v);

		verticesTmp[4*i+0].color = XMFLOAT4(quads[i].tl.colors.r/255.f, quads[i].tl.colors.g/255.f, quads[i].tl.colors.b/255.f, quads[i].tl.colors.a/255.f);
		verticesTmp[4*i+1].color = XMFLOAT4(quads[i].tr.colors.r/255.f, quads[i].tr.colors.g/255.f, quads[i].tr.colors.b/255.f, quads[i].tr.colors.a/255.f);
		verticesTmp[4*i+2].color = XMFLOAT4(quads[i].br.colors.r/255.f, quads[i].br.colors.g/255.f, quads[i].br.colors.b/255.f, quads[i].br.colors.a/255.f);
		verticesTmp[4*i+3].color = XMFLOAT4(quads[i].bl.colors.r/255.f, quads[i].bl.colors.g/255.f, quads[i].bl.colors.b/255.f, quads[i].bl.colors.a/255.f);
	}

	D3D11_MAPPED_SUBRESOURCE mappedResourceVertex;
	VertexType* verticesPtr;
	if(FAILED(CCID3D11DeviceContext->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceVertex))){return ;}
	verticesPtr = (VertexType*)mappedResourceVertex.pData;
	memcpy(verticesPtr, (void*)verticesTmp, (sizeof(VertexType) * 4 * capacity));
	CCID3D11DeviceContext->Unmap(m_vertexBuffer, 0);

	if ( verticesTmp )
	{
		delete[] verticesTmp;
		verticesTmp = 0;
	}
	////////////////////////
	unsigned int stride;
	unsigned int offset;
	stride = sizeof(VertexType); 
	offset = 0;
	CCID3D11DeviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	CCID3D11DeviceContext->IASetIndexBuffer( m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	CCID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

void CCDXTextureAtlas::initVertexBuffer(unsigned short* indices,unsigned int capacity)
{
	CC_SAFE_RELEASE_NULL_DX(m_indexBuffer);
	CC_SAFE_RELEASE_NULL_DX(m_vertexBuffer);
	D3D11_BUFFER_DESC vertexBufferDesc;

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType)*4 * capacity;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Now create the vertex buffer.
	if(FAILED(CCID3D11Device->CreateBuffer(&vertexBufferDesc, NULL, &m_vertexBuffer)))
	{
		return ;
	}


	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory( &indexBufferDesc, sizeof(indexBufferDesc) );

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(CCushort) * 6 * capacity;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;
	CCID3D11Device->CreateBuffer(&indexBufferDesc, &iinitData, &m_indexBuffer);
}

bool CCDXTextureAtlas::InitializeShader()
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	//ID3D10Blob* vertexShaderBuffer;
	//ID3D10Blob* pixelShaderBuffer;
	D3D11_BUFFER_DESC matrixBufferDesc;

	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	//vertexShaderBuffer = 0;
	//pixelShaderBuffer = 0;

	BasicLoader^ loader = ref new BasicLoader(CCID3D11Device);
	D3D11_INPUT_ELEMENT_DESC layoutDesc[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	loader->LoadShader(
		L"CCTextureAtlasVertexShader.cso",
		layoutDesc,
		ARRAYSIZE(layoutDesc),
		&m_vertexShader,
		&m_layout
		);

	loader->LoadShader(
		L"CCTextureAtlasPixelShader.cso",
		&m_pixelShader
		);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = CCID3D11Device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	if(FAILED(result))
	{
		return false;
	}

	return true;
}

void CCDXTextureAtlas::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
	char* compileErrors;
	unsigned long bufferSize, i;
	ofstream fout;


	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	fout.open("shader-error.txt");

	// Write out the error message.
	for(i=0; i<bufferSize; i++)
	{
		fout << compileErrors[i];
	}

	// Close the file.
	fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	// MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

	return;
}


bool CCDXTextureAtlas::SetShaderParameters( XMMATRIX &viewMatrix, XMMATRIX &projectionMatrix, ID3D11ShaderResourceView* texture)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	unsigned int bufferNumber;

	viewMatrix = XMMatrixTranspose(viewMatrix);
	projectionMatrix = XMMatrixTranspose(projectionMatrix);

	if(FAILED(CCID3D11DeviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))){return false;}
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;
	CCID3D11DeviceContext->Unmap(m_matrixBuffer, 0);

	bufferNumber = 0;
	CCID3D11DeviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

	CCID3D11DeviceContext->PSSetShaderResources(0, 1, &texture);

	return true;
}

void CCDXTextureAtlas::RenderShader(CCTexture2D* texture,unsigned int n, unsigned int start)
{
	CCID3D11DeviceContext->IASetInputLayout(m_layout);
	CCID3D11DeviceContext->VSSetShader(m_vertexShader, NULL, 0);
	CCID3D11DeviceContext->PSSetShader(m_pixelShader, NULL, 0);
	CCID3D11DeviceContext->PSSetSamplers(0, 1, texture->GetSamplerState());
	CCID3D11DeviceContext->DrawIndexed(n*6, start*6, 0 );

	return;
}


void CCDXTextureAtlas::Render(ccV3F_C4B_T2F_Quad* quads,unsigned short* indices,unsigned int capacity,CCTexture2D* texture,unsigned int n, unsigned int start)
{
	if ( !mIsInit )
	{
		mIsInit = TRUE;
		FreeBuffer();
		InitializeShader();
	}
	initVertexBuffer(indices,capacity);

	XMMATRIX viewMatrix, projectionMatrix;
	// Get the world, view, and projection matrices from the camera and d3d objects.
	CCD3DCLASS->GetViewMatrix(viewMatrix);
	CCD3DCLASS->GetProjectionMatrix(projectionMatrix);

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderVertexBuffer(quads,capacity);

	// Set the shader parameters that it will use for rendering.
	SetShaderParameters(viewMatrix, projectionMatrix, texture->getTextureResource());

	// Now render the prepared buffers with the shader.
	RenderShader(texture,n,start);
}

NS_CC_END

