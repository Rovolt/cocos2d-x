#include "AppDelegate.h"

#include "cocos2d.h"
#include "Classes/HelloWorldScene.h"
#include "AppMacro.h"
#include "CCEGLView.h"
USING_NS_CC;

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate()
{
//    SimpleAudioEngine::end();
}

bool AppDelegate::initInstance()
{
    bool bRet = false;
    do 
    {

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN8_METRO)

	// fix bug: 16bit aligned
	void* buff=_aligned_malloc(sizeof(CCEGLView),16);
	CCEGLView* mainView = new (buff) CCEGLView();

	CCDirector *pDirector = CCDirector::sharedDirector();
	mainView->Create();
	pDirector->setOpenGLView(mainView);
	
	

	
	

#endif // CC_PLATFORM_WIN8_METRO

        bRet = true;
    } while (0);
    return bRet;
}


bool AppDelegate::applicationDidFinishLaunching()
{
	// initialize director
	CCDirector *pDirector = CCDirector::sharedDirector();
	CCEGLView* pEGLView = CCEGLView::sharedOpenGLView();

	
	//pDirector->setOpenGLView(&CCEGLView::sharedOpenGLView());
	CCSize frameSize = pEGLView->getFrameSize();
	// enable High Resource Mode(2x, such as iphone4) and maintains low resource on other devices.
	// pDirector->enableRetinaDisplay(true);

	if (frameSize.height > mediumResource.size.height)
	{ 
		CCFileUtils::sharedFileUtils()->setResourceDirectory(largeResource.directory);
        pDirector->setContentScaleFactor(MIN(largeResource.size.height/designResolutionSize.height, largeResource.size.width/designResolutionSize.width));
	}
    // if the frame's height is larger than the height of small resource size, select medium resource.
    else if (frameSize.height > smallResource.size.height)
    { 
        CCFileUtils::sharedFileUtils()->setResourceDirectory(mediumResource.directory);
        pDirector->setContentScaleFactor(MIN(mediumResource.size.height/designResolutionSize.height, mediumResource.size.width/designResolutionSize.width));
    }
    // if the frame's height is smaller than the height of medium resource size, select small resource.
	else
    { 
		CCFileUtils::sharedFileUtils()->setResourceDirectory(smallResource.directory);
        pDirector->setContentScaleFactor(MIN(smallResource.size.height/designResolutionSize.height, smallResource.size.width/designResolutionSize.width));
    }
	pEGLView->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height, kResolutionNoBorder); 
    // turn on display FPS
    pDirector->setDisplayStats(true);

    // set FPS. the default value is 1.0/60 if you don't call this
    pDirector->setAnimationInterval(1.0 / 60);

    // create a scene. it's an autorelease object
    CCScene *pScene = HelloWorld::scene();

	/*MutiTouchTestLayer* pLayer = MutiTouchTestLayer::create();
	CCScene* scene = CCScene::create();
    scene->addChild(pLayer, 0);
	scene->retain();*/

    //CCDirector::sharedDirector()->replaceScene(scene);

    // run
    pDirector->runWithScene(pScene);


	return true;

}

// This function will be called when the app is inactive. When comes a phone call,it's be invoked too
void AppDelegate::applicationDidEnterBackground()
{
    CCDirector::sharedDirector()->pause();
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    CCDirector::sharedDirector()->resume();
}
