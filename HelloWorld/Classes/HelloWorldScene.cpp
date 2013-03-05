#include "HelloWorldScene.h"
#include "../AppMacro.h"
USING_NS_CC;





static ccColor3B s_TouchColors[30] = {
    ccYELLOW,
    ccBLUE,
    ccGREEN,
    ccRED,
    ccMAGENTA
};

class TouchPoint : public CCNode
{
public:
    TouchPoint()
    {
        //setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
    }

    void setTouchPos(const CCPoint& pt)
    {
		_touch->setPosition(pt);
    }

    void setTouchColor(ccColor3B color)
    {
		_touch->setColor(color);
    }

    static TouchPoint* touchPointWithParent(CCNode* pParent)
    {
        TouchPoint* pRet = new TouchPoint();
        //pRet->setContentSize(pParent->getContentSize());
        pRet->setAnchorPoint(ccp(0.5f, 0.5f));
		pRet->_touch = CCSprite::create("touch.png");
		pRet->addChild(pRet->_touch);
        pRet->autorelease();
        return pRet;
    }

private:
    //CCPoint m_pTouchPoint;
    //ccColor3B m_TouchColor;
	CCSprite* _touch;
};



static CCDictionary s_dic;

void HelloWorld::registerWithTouchDispatcher(void)
{
    CCDirector::sharedDirector()->getTouchDispatcher()->addStandardDelegate(this, 0);
}

void HelloWorld::ccTouchesBegan(CCSet *pTouches, CCEvent *pEvent)
{
    CCSetIterator iter = pTouches->begin();
    for (; iter != pTouches->end(); iter++)
    {
        CCTouch* pTouch = (CCTouch*)(*iter);
        TouchPoint* pTouchPoint = TouchPoint::touchPointWithParent(this);
        CCPoint location = pTouch->getLocation();

        pTouchPoint->setTouchPos(location);
        pTouchPoint->setTouchColor(s_TouchColors[pTouch->getID()]);

        addChild(pTouchPoint);
        s_dic.setObject(pTouchPoint, pTouch->getID());
    }
    

}

void HelloWorld::ccTouchesMoved(CCSet *pTouches, CCEvent *pEvent)
{
    CCSetIterator iter = pTouches->begin();
    for (; iter != pTouches->end(); iter++)
    {
        CCTouch* pTouch = (CCTouch*)(*iter);
        TouchPoint* pTP = (TouchPoint*)s_dic.objectForKey(pTouch->getID());
        CCPoint location = pTouch->getLocation();
        pTP->setTouchPos(location);
    }
}

void HelloWorld::ccTouchesEnded(CCSet *pTouches, CCEvent *pEvent)
{
    CCSetIterator iter = pTouches->begin();
    for (; iter != pTouches->end(); iter++)
    {
        CCTouch* pTouch = (CCTouch*)(*iter);
        TouchPoint* pTP = (TouchPoint*)s_dic.objectForKey(pTouch->getID());
        removeChild(pTP, true);
        s_dic.removeObjectForKey(pTouch->getID());
    }
}

void HelloWorld::ccTouchesCancelled(CCSet *pTouches, CCEvent *pEvent)
{
    ccTouchesEnded(pTouches, pEvent);
}





CCScene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // 'layer' is an autorelease object
    HelloWorld *layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}
#include "CCEGLView.h"
#include "win8_metro/DirectXRender.h"
#include <sstream>
// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !CCLayer::init() )
    {
		
        return false;
    }
    setTouchEnabled(true);
    CCSize visibleSize = CCDirector::sharedDirector()->getVisibleSize();
    CCPoint origin = CCDirector::sharedDirector()->getVisibleOrigin();
	//origin.y = -origin.y;
    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    CCMenuItemImage *pCloseItem = CCMenuItemImage::create(
                                        "CloseNormal.png",
                                        "CloseSelected.png",
                                        this,
                                        menu_selector(HelloWorld::menuCloseCallback));
    
	/*pCloseItem->setPosition(ccp(origin.x + visibleSize.width - pCloseItem->getContentSize().width/2 ,
                                origin.y + pCloseItem->getContentSize().height/2));*/
	pCloseItem->setPosition(ccp(origin.x + visibleSize.width/2,
                                origin.y + visibleSize.height/2));

	
    // create menu, it's an autorelease object
    CCMenu* pMenu = CCMenu::create(pCloseItem, NULL);
    pMenu->setPosition(CCPointZero);
    this->addChild(pMenu, 1);

    /////////////////////////////
    // 3. add your codes below...
	CCRect view_port = CCEGLView::sharedOpenGLView()->getViewPortRect();
	DirectXRender^ render = DirectXRender::SharedDXRender();

	
    // add a label shows "Hello World"
    // create and initialize a label
	std::stringstream ss;
	ss << "Vis Width: " << visibleSize.width << std::endl;
	ss << "Vis Hegiht: " << visibleSize.height << std::endl;
	ss << "Origin X: " << origin.x << std::endl;
	ss << "Origin Y: " << origin.y << std::endl;

	ss << "VPR Width: " << view_port.size.width << std::endl;
	ss << "VPR Hegiht: " << view_port.size.height << std::endl;
	ss << "VPR Origin X: " << view_port.origin.x << std::endl;
	ss << "VPR Origin Y: " << view_port.origin.y << std::endl;
	ss << "DPI: " << render->GetDpi() << std::endl;
	std::string val = ss.str();
	CCLabelTTF* pLabel = CCLabelTTF::create(val.c_str(), "Arial", 16);

	pLabel->setColor(ccc3(0,0,0));
    
    // position the label on the center of the screen
    /*pLabel->setPosition(ccp(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - pLabel->getContentSize().height));*/
	pLabel->setPosition(ccp(origin.x, origin.y));
	pLabel->setAnchorPoint(ccp(0,0));
    // add the label as a child to this layer
    this->addChild(pLabel, 1);

    // add "HelloWorld" splash screen"
    CCSprite* pSprite = CCSprite::create("HelloWorld.png");

    // position the sprite on the center of the screen
    pSprite->setPosition(ccp(visibleSize.width/2 + origin.x, visibleSize.height/2 + origin.y));

    // add the sprite as a child to this layer
    this->addChild(pSprite, 0);
    
	
	CCSprite* left_bottom = CCSprite::create("touch.png");
    left_bottom->setColor(ccc3(255, 0, 0));
    left_bottom->setPosition(ccp(origin.x, origin.y));
    left_bottom->setAnchorPoint(ccp(0.5, 0.5));
    this->addChild(left_bottom);

    CCSprite* right_bottom = CCSprite::create("touch.png");
    right_bottom->setColor(ccc3(0, 255, 0));
    right_bottom->setPosition(ccp(origin.x+visibleSize.width, origin.y));
    right_bottom->setAnchorPoint(ccp(0.5, 0.5));
    this->addChild(right_bottom);

    CCSprite* right_top = CCSprite::create("touch.png");
    right_top->setColor(ccc3(0, 0, 255));
    right_top->setPosition(ccp(origin.x+visibleSize.width, origin.y+visibleSize.height));
    right_top->setAnchorPoint(ccp(0.5, 0.5));
    this->addChild(right_top);

    CCSprite* left_top = CCSprite::create("touch.png");
    left_top->setColor(ccc3(0, 255, 255));
    left_top->setPosition(ccp(origin.x, origin.y+visibleSize.height));
    left_top->setAnchorPoint(ccp(0.5, 0.5));
    this->addChild(left_top);

    return true;
}


void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    CCDirector::sharedDirector()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
