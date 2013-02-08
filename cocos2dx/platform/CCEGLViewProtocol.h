#ifndef __CCEGLVIEWPROTOCOL_H__
#define __CCEGLVIEWPROTOCOL_H__

#include "ccTypes.h"

enum ResolutionPolicy
{
    // The entire application is visible in the specified area without trying to preserve the original aspect ratio.
    // Distortion can occur, and the application may appear stretched or compressed.
    kResolutionExactFit,
    // The entire application fills the specified area, without distortion but possibly with some cropping,
    // while maintaining the original aspect ratio of the application.
    kResolutionNoBorder,
    // The entire application is visible in the specified area without distortion while maintaining the original
    // aspect ratio of the application. Borders can appear on two sides of the application.
    kResolutionShowAll,

    kResolutionUnKnown,
};
#include <stack>
#include <deque>
NS_CC_BEGIN

#define CC_MAX_TOUCHES  5

class EGLTouchDelegate;
class CCSet;

/**
 * @addtogroup platform
 * @{
 */
template<typename T, size_t TALIGN=16, size_t TBLOCK=8>
class aligned_allocator
{
public:
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	T * address(T& r) const 
	{
		return &r;
	}
	const T * address(const T& s) const 
	{
		return &s;
	}
	size_t max_size() const 
	{
		return (static_cast<size_t>(0) - static_cast<size_t>(1)) / sizeof(T);
	}
	template <typename U> struct rebind 
	{
		typedef aligned_allocator<U> other;
	};
	bool operator!=(const aligned_allocator& other) const 
	{
		return !(*this == other);
	}
	bool operator==(const aligned_allocator& other) const 
	{
		return true;
	}
	void construct(pointer p, const T &val) 
	{
		new (p) T(val);
	}
	void destroy(pointer p) 
	{
		p->~T();
	}
	aligned_allocator() 
	{
	}
	aligned_allocator(const aligned_allocator &) 
	{
	}
	template<typename U> aligned_allocator(const aligned_allocator<U>&) 
	{
	}
	~aligned_allocator() 
	{
	}
	pointer allocate(size_t n) 
	{
		return allocate(n, NULL);
	}
	pointer allocate(size_t n, const void *hint) 
	{
		pointer p = NULL;
		size_t count = sizeof(T) * n;
		size_t count_left = count % TBLOCK;
		if (0 != count_left) {
			count += TBLOCK - count_left;
		}
		if (!hint) {
			p = reinterpret_cast<pointer>(_aligned_malloc(count, TALIGN));
		}
		else {
			p = reinterpret_cast<pointer>(_aligned_realloc((void *)hint, count, TALIGN));
		}
		return p;
	}
	void deallocate(pointer p, size_t) 
	{
		_aligned_free(p);
	}
};

class CC_DLL CCEGLViewProtocol
{
public:
    CCEGLViewProtocol();
    virtual ~CCEGLViewProtocol();

	//d3d
	void D3DOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
	void D3DPerspective(FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf);
	void D3DLookAt(float fEyeX, float fEyeY, float fEyeZ, float fLookAtX, float fLookAtY, float fLookAtZ, float fUpX, float fUpY, float fUpZ);
	void D3DTranslate(float x, float y, float z);
	void D3DRotate(float angle, float x, float y, float z);
	void D3DScale(float x, float y, float z);
	void D3DMultMatrix(const float *m);
	void D3DBlendFunc(int sfactor, int dfactor);
	void D3DViewport(int x, int y, int width, int height);
	void D3DScissor(int x,int y,int w,int h);
	void D3DMatrixMode(int matrixMode);
	void D3DLoadIdentity();
	void D3DPushMatrix();
	void D3DPopMatrix();
	void D3DDepthFunc(int func);
	void D3DClearColor(float r, float b, float g, float a);


    /** Force destroying EGL view, subclass must implement this method. */
    virtual void    end() = 0;

    /** Get whether opengl render system is ready, subclass must implement this method. */
    virtual bool    isOpenGLReady() = 0;

    /** Exchanges the front and back buffers, subclass must implement this method. */
    virtual void    swapBuffers() = 0;

    /** Open or close IME keyboard , subclass must implement this method. */
    virtual void    setIMEKeyboardState(bool bOpen) = 0;

    /**
     * Get the frame size of EGL view.
     * In general, it returns the screen size since the EGL view is a fullscreen view.
     */
    virtual const CCSize& getFrameSize() const;

    /**
     * Set the frame size of EGL view.
     */
    virtual void setFrameSize(float width, float height);

    /**
     * Get the visible area size of opengl viewport.
     */
    virtual CCSize getVisibleSize() const;

    /**
     * Get the visible origin point of opengl viewport.
     */
    virtual CCPoint getVisibleOrigin() const;

    /**
     * Set the design resolution size.
     * @param width Design resolution width.
     * @param height Design resolution height.
     * @param resolutionPolicy The resolution policy desired, you may choose:
     *                         [1] kResolutionExactFit Fill screen by stretch-to-fit: if the design resolution ratio of width to height is different from the screen resolution ratio, your game view will be stretched.
     *                         [2] kResolutionNoBorder Full screen without black border: if the design resolution ratio of width to height is different from the screen resolution ratio, two areas of your game view will be cut.
     *                         [3] kResolutionShowAll  Full screen with black border: if the design resolution ratio of width to height is different from the screen resolution ratio, two black borders will be shown.
     */
    virtual void setDesignResolutionSize(float width, float height, ResolutionPolicy resolutionPolicy);

    /** Get design resolution size.
     *  Default resolution size is the same as 'getFrameSize'.
     */
    virtual const CCSize&  getDesignResolutionSize() const;

    /** Set touch delegate */
    virtual void setTouchDelegate(EGLTouchDelegate * pDelegate);

    /**
     * Set opengl view port rectangle with points.
     */
    virtual void setViewPortInPoints(float x , float y , float w , float h);

    /**
     * Set Scissor rectangle with points.
     */
    virtual void setScissorInPoints(float x , float y , float w , float h);

    virtual void setViewName(const char* pszViewName);

    const char* getViewName();

    /** Touch events are handled by default; if you want to customize your handlers, please override these functions: */
    virtual void handleTouchesBegin(int num, int ids[], float xs[], float ys[]);
    virtual void handleTouchesMove(int num, int ids[], float xs[], float ys[]);
    virtual void handleTouchesEnd(int num, int ids[], float xs[], float ys[]);
    virtual void handleTouchesCancel(int num, int ids[], float xs[], float ys[]);

    /**
     * Get the opengl view port rectangle.
     */
    const CCRect& getViewPortRect() const;

    /**
     * Get scale factor of the horizontal direction.
     */
    float getScaleX() const;

    /**
     * Get scale factor of the vertical direction.
     */
    float getScaleY() const;
private:
    void getSetOfTouchesEndOrCancel(CCSet& set, int num, int ids[], float xs[], float ys[]);

protected:
    EGLTouchDelegate* m_pDelegate;

    // real screen size
    CCSize m_obScreenSize;
    // resolution size, it is the size appropriate for the app resources.
    CCSize m_obDesignResolutionSize;
    // the view port size
    CCRect m_obViewPortRect;
    // the view name
    char   m_szViewName[50];

    float  m_fScaleX;
    float  m_fScaleY;
    ResolutionPolicy m_eResolutionPolicy;

	ID3D11Device1*           m_d3dDevice;
    ID3D11DeviceContext1*    m_d3dContext;
    IDXGISwapChain1*         m_swapChain;
    ID3D11RenderTargetView*  m_renderTargetView;
    ID3D11DepthStencilView*  m_depthStencilView;

	float m_color[4];
    int mMatrixMode;

	DirectX::XMMATRIX m_projectionMatrix;
	DirectX::XMMATRIX m_viewMatrix;

	struct MatrixStruct
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};
#if defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
	std::stack<MatrixStruct, std::deque<MatrixStruct, aligned_allocator<MatrixStruct> > > m_MatrixStack;
#else 
	std::stack<MatrixStruct> m_MatrixStack;
#endif
    int m_oldViewState;
};

// end of platform group
/// @}

NS_CC_END

#endif /* __CCEGLVIEWPROTOCOL_H__ */
