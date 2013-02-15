/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

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

//#include "CCDirector.h"
#include <ppltasks.h>
#include "CCFileUtils.h"
#include "CCCommon.h"
#include "CCPlatformMacros.h"
#include "cocoa/CCString.h"
#include "CCFileUtilsCommon_cpp.h"
#include <wrl.h>
#include <wincodec.h>
#include "CCApplication.h"
#include <mmreg.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mfmediaengine.h>
#include <Windows.h>
using namespace Windows::Storage;
using namespace Windows::ApplicationModel;
using namespace std;

NS_CC_BEGIN;

// record the resource path
static char s_pszResourcePath[MAX_PATH] = {0};



void _CheckPath()
{
	if (! s_pszResourcePath[0])
	{
		//WCHAR  wszPath[MAX_PATH];
		//int nNum = WideCharToMultiByte(CP_ACP, 0, wszPath, 
		//	GetCurrentDirectoryW(sizeof(wszPath), wszPath), 
		//	s_pszResourcePath, MAX_PATH, NULL, NULL);
		//      s_pszResourcePath[nNum] = '\\';

		Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
		Windows::Storage::StorageFolder^ installedLocation = package->InstalledLocation;
		Platform::String^ resPath = installedLocation->Path + "\\Assets\\";
		std::string pathStr = CCUnicodeToUtf8(resPath->Data());
		strcpy_s(s_pszResourcePath, pathStr.c_str());
	}
}
static CCFileUtils* s_pFileUtils = NULL;
CCFileUtils* CCFileUtils::sharedFileUtils()
{
	if (s_pFileUtils == NULL)
	{
		s_pFileUtils = new CCFileUtils();
		_CheckPath();
	}
	return s_pFileUtils;
}
//void CCFileUtils::setResourceDirectory(const char *pszResourcePath)
//{
//    assert(pszResourcePath != NULL/*, "[FileUtils setResourcePath] -- wrong resource path"*/);
//    assert(strlen(pszResourcePath) <= MAX_PATH/*, "[FileUtils setResourcePath] -- resource path too long"*/);
//
//    strcpy_s(s_pszResourcePath, pszResourcePath);
//}

/*bool CCFileUtils::isFileExist(const char * resPath)
{
_CheckPath();
bool ret = false;
FILE * pf = 0;
if (resPath && strlen(resPath) && (pf = fopen(resPath, "rb")))
{
ret = true;
fclose(pf);
}
return ret;
}*/

void CCFileUtils::purgeFileUtils()
{
	if (s_pFileUtils != NULL)
	{
		s_pFileUtils->purgeCachedEntries();
	}

	CC_SAFE_DELETE(s_pFileUtils);
}
void CCFileUtils::purgeCachedEntries()
{

}
using namespace Windows::Data::Xml::Dom;
class CCDictMaker// : public CCSAXDelegator
{
public:
	CCSAXResult m_eResultType;
	CCArray* m_pRootArray;
	CCDictionary *m_pRootDict;
	CCDictionary *m_pCurDict;
	std::stack<CCDictionary*> m_tDictStack;
	std::string m_sCurKey;   ///< parsed key
	std::string m_sCurValue; // parsed value
	CCSAXState m_tState;
	CCArray* m_pArray;

	std::stack<CCArray*> m_tArrayStack;
	std::stack<CCSAXState>  m_tStateStack;
private:

	void emulateSAX_processNodes(XmlNodeList^ node_list)
	{
		for(auto it = node_list->First(); it->HasCurrent; it->MoveNext())
		{
			emulateSAX_processNode(it->Current);
		}
	}
	void emulateSAX_processNode(IXmlNode^ node)
	{
		if(node->NodeType == NodeType::TextNode)
		{
			std::string text = CCUnicodeToUtf8(node->InnerText->Begin());
			this->textHandler(0, text.c_str(), text.size());
		}
		else
		{
			Platform::String^ node_name = node->NodeName;
			startElement(0, node_name->Begin(), 0);

			if(node->ChildNodes->Size > 0)
			{
				emulateSAX_processNodes(node->ChildNodes);
			}

			endElement(0, node_name->Begin());
		}
	}
	void emulateSAX(const char *pFileName)
	{
		
		using namespace Windows::Storage;
		using namespace concurrency;
		XmlDocument^ doc= ref new XmlDocument();	
		unsigned long size = 0;
		const char* file_cont = reinterpret_cast<const char*>(CCFileUtils::sharedFileUtils()->getFileData(pFileName, "r", &size));
		assert(size > 0);
		wstring xml_str = CCUtf8ToUnicode(file_cont, size);

		//Stupid parser replace encoding
		//const std::wstring encoding = L"UTF-8";
		//size_t found = xml_str.find(encoding);
		//xml_str.replace(found, encoding.length(), L"UTF-16");
		Platform::String^ xml = ref new Platform::String(xml_str.c_str());
		assert(xml != "");

		XmlLoadSettings^ settings = ref new XmlLoadSettings();
		settings->ElementContentWhiteSpace = false;
		settings->ProhibitDtd = false;

		doc->LoadXml(xml, settings);

		emulateSAX_processNodes(doc->ChildNodes);
		/*Platform::String^ resPath = ref new Platform::String(CCUtf8ToUnicode(pFileName).c_str());
		auto obtain_xml_file_name = create_task(StorageFile::GetFileFromPathAsync(resPath));
		obtain_xml_file_name.wait();
		StorageFile^ xml_file = obtain_xml_file_name.get();
		auto load_xml_task = create_task(doc->LoadFromFileAsync( xml_file ));
		load_xml_task.wait();
		doc = load_xml_task.get();
		emulateSAX_processNodeList(doc->ChildNodes);*/
		
	}
public:
	CCDictMaker()        
		: m_eResultType(SAX_RESULT_NONE),
		m_pRootArray(NULL), 
		m_pRootDict(NULL),
		m_pCurDict(NULL),
		m_tState(SAX_NONE),
		m_pArray(NULL)
	{
	}

	~CCDictMaker()
	{
	}

	CCDictionary* dictionaryWithContentsOfFile(const char *pFileName)
	{
		m_eResultType = SAX_RESULT_DICT;
		//CCSAXParser parser;

		//if (false == parser.init("UTF-8"))
		//{
		//    return NULL;
		//}
		//parser.setDelegator(this);

		//parser.parse(pFileName);
		emulateSAX(pFileName);
		return m_pRootDict;
	}

	CCArray* arrayWithContentsOfFile(const char* pFileName)
	{
		m_eResultType = SAX_RESULT_ARRAY;
		//CCSAXParser parser;

		//if (false == parser.init("UTF-8"))
		//{
		//    return NULL;
		//}
		//parser.setDelegator(this);

		//parser.parse(pFileName);
		emulateSAX(pFileName);
		return m_pArray;
	}

	void startElement(void *ctx, const char16 *name, const char **atts)
	{
		CC_UNUSED_PARAM(ctx);
		CC_UNUSED_PARAM(atts);
		std::wstring sName(name);
		if( sName == L"dict" )
		{
			m_pCurDict = new CCDictionary();
			if(m_eResultType == SAX_RESULT_DICT && m_pRootDict == NULL)
			{
				// Because it will call m_pCurDict->release() later, so retain here.
				m_pRootDict = m_pCurDict;
				m_pRootDict->retain();
			}
			m_tState = SAX_DICT;

			CCSAXState preState = SAX_NONE;
			if (! m_tStateStack.empty())
			{
				preState = m_tStateStack.top();
			}

			if (SAX_ARRAY == preState)
			{
				// add the dictionary into the array
				m_pArray->addObject(m_pCurDict);
			}
			else if (SAX_DICT == preState)
			{
				// add the dictionary into the pre dictionary
				CCAssert(! m_tDictStack.empty(), "The state is wrong!");
				CCDictionary* pPreDict = m_tDictStack.top();
				pPreDict->setObject(m_pCurDict, m_sCurKey.c_str());
			}

			m_pCurDict->release();

			// record the dict state
			m_tStateStack.push(m_tState);
			m_tDictStack.push(m_pCurDict);
		}
		else if(sName == L"key")
		{
			m_tState = SAX_KEY;
		}
		else if(sName == L"integer")
		{
			m_tState = SAX_INT;
		}
		else if(sName == L"real")
		{
			m_tState = SAX_REAL;
		}
		else if(sName == L"string")
		{
			m_tState = SAX_STRING;
		}
		else if (sName == L"array")
		{
			m_tState = SAX_ARRAY;
			m_pArray = new CCArray();
			if (m_eResultType == SAX_RESULT_ARRAY && m_pRootArray == NULL)
			{
				m_pRootArray = m_pArray;
				m_pRootArray->retain();
			}
			CCSAXState preState = SAX_NONE;
			if (! m_tStateStack.empty())
			{
				preState = m_tStateStack.top();
			}

			if (preState == SAX_DICT)
			{
				m_pCurDict->setObject(m_pArray, m_sCurKey.c_str());
			}
			else if (preState == SAX_ARRAY)
			{
				CCAssert(! m_tArrayStack.empty(), "The state is wrong!");
				CCArray* pPreArray = m_tArrayStack.top();
				pPreArray->addObject(m_pArray);
			}
			m_pArray->release();
			// record the array state
			m_tStateStack.push(m_tState);
			m_tArrayStack.push(m_pArray);
		}
		else
		{
			m_tState = SAX_NONE;
		}
	}

	void endElement(void *ctx, const char16 *name)
	{
		CC_UNUSED_PARAM(ctx);
		CCSAXState curState = m_tStateStack.empty() ? SAX_DICT : m_tStateStack.top();
		std::wstring sName(name);
		if( sName == L"dict" )
		{
			m_tStateStack.pop();
			m_tDictStack.pop();
			if ( !m_tDictStack.empty())
			{
				m_pCurDict = m_tDictStack.top();
			}
		}
		else if (sName == L"array")
		{
			m_tStateStack.pop();
			m_tArrayStack.pop();
			if (! m_tArrayStack.empty())
			{
				m_pArray = m_tArrayStack.top();
			}
		}
		else if (sName == L"true")
		{
			CCString *str = new CCString("1");
			if (SAX_ARRAY == curState)
			{
				m_pArray->addObject(str);
			}
			else if (SAX_DICT == curState)
			{
				m_pCurDict->setObject(str, m_sCurKey.c_str());
			}
			str->release();
		}
		else if (sName == L"false")
		{
			CCString *str = new CCString("0");
			if (SAX_ARRAY == curState)
			{
				m_pArray->addObject(str);
			}
			else if (SAX_DICT == curState)
			{
				m_pCurDict->setObject(str, m_sCurKey.c_str());
			}
			str->release();
		}
		else if (sName == L"string" || sName == L"integer" || sName == L"real")
		{
			CCString* pStrValue = new CCString(m_sCurValue);

			if (SAX_ARRAY == curState)
			{
				m_pArray->addObject(pStrValue);
			}
			else if (SAX_DICT == curState)
			{
				m_pCurDict->setObject(pStrValue, m_sCurKey.c_str());
			}

			pStrValue->release();
			m_sCurValue.clear();
		}

		m_tState = SAX_NONE;
	}

	void textHandler(void *ctx, const char *ch, int len)
	{
		CC_UNUSED_PARAM(ctx);
		if (m_tState == SAX_NONE)
		{
			return;
		}

		CCSAXState curState = m_tStateStack.empty() ? SAX_DICT : m_tStateStack.top();
		CCString *pText = new CCString(std::string((char*)ch,0,len));

		switch(m_tState)
		{
		case SAX_KEY:
			m_sCurKey = pText->getCString();
			break;
		case SAX_INT:
		case SAX_REAL:
		case SAX_STRING:
			{
				if (curState == SAX_DICT)
				{
					CCAssert(!m_sCurKey.empty(), "key not found : <integer/real>");
				}

				m_sCurValue.append(pText->getCString());
			}
			break;
		default:
			break;
		}
		pText->release();
	}
};


CCDictionary* ccFileUtils_dictionaryWithContentsOfFileThreadSafe(const char *pFileName)
{
	CCDictMaker tMaker;
	return tMaker.dictionaryWithContentsOfFile(pFileName);
	return 0;
}

const char* CCFileUtils::fullPathFromRelativePath(const char *pszRelativePath)
{
	bool bFileExist = true;
	_CheckPath();
	const char* resDir = m_obDirectory.c_str();
	CCString * pRet = new CCString();
	pRet->autorelease();
	const std::string& resourceRootPath = CCApplication::sharedApplication()->getResourceRootPath();
	if ((strlen(pszRelativePath) > 1 && pszRelativePath[1] == ':'))
	{
		// path start with "x:", is absolute path
		pRet->m_sString = pszRelativePath;
	}
	else if (strlen(pszRelativePath) > 0 
		&& ('/' == pszRelativePath[0] || '\\' == pszRelativePath[0]))
	{
		// path start with '/' or '\', is absolute path without driver name
		char szDriver[3] = {s_pszResourcePath[0], s_pszResourcePath[1], 0};
		pRet->m_sString = szDriver;
		pRet->m_sString += pszRelativePath;
	}
	else if (resourceRootPath.length() > 0)
	{
		pRet->m_sString = resourceRootPath.c_str();
		pRet->m_sString += m_obDirectory.c_str();
		pRet->m_sString += pszRelativePath;
	}
	else
	{
		pRet->m_sString = s_pszResourcePath;
		pRet->m_sString += resDir;
		pRet->m_sString += pszRelativePath;
	}
	WIN32_FILE_ATTRIBUTE_DATA  fileInfo;
	// If file or directory doesn't exist, try to find it in the root path.
	std::wstring file_w = CCUtf8ToUnicode(pRet->m_sString.c_str(), pRet->m_sString.size());
	if (!GetFileAttributesEx(file_w.c_str(), GetFileExInfoStandard, &fileInfo))
	{
		pRet->m_sString = s_pszResourcePath;
		pRet->m_sString += pszRelativePath;

		file_w = CCUtf8ToUnicode(pRet->m_sString.c_str(), pRet->m_sString.size());
		if (!GetFileAttributesEx(file_w.c_str(), GetFileExInfoStandard, &fileInfo))
		{
			bFileExist = false;
		}
	}

	if (!bFileExist)
	{ // Can't find the file, return the relative path.
		pRet->m_sString = pszRelativePath;
	}

	//#if (CC_IS_RETINA_DISPLAY_SUPPORTED)
	//    if (CC_CONTENT_SCALE_FACTOR() != 1.0f)
	//    {
	//        std::string hiRes = pRet->m_sString.c_str();
	//        std::string::size_type pos = hiRes.find_last_of("/\\");
	//        std::string::size_type dotPos = hiRes.find_last_of(".");
	//        
	//        if (std::string::npos != dotPos && dotPos > pos)
	//        {
	//            hiRes.insert(dotPos, CC_RETINA_DISPLAY_FILENAME_SUFFIX);
	//        }
	//        else
	//        {
	//            hiRes.append(CC_RETINA_DISPLAY_FILENAME_SUFFIX);
	//        }
	//        DWORD attrib = GetFileAttributesA(hiRes.c_str());
	//        
	//        if (attrib != INVALID_FILE_ATTRIBUTES && ! (FILE_ATTRIBUTE_DIRECTORY & attrib))
	//        {
	//            pRet->m_sString.swap(hiRes);
	//        }
	//    }
	//#endif
	/*if (pResolutionType)
	{
	*pResolutionType = kCCResolutioniPhone;
	}*/
	return pRet->m_sString.c_str();
}

const char *CCFileUtils::fullPathFromRelativeFile(const char *pszFilename, const char *pszRelativeFile)
{
	_CheckPath();
	// std::string relativeFile = fullPathFromRelativePath(pszRelativeFile);
	std::string relativeFile = pszRelativeFile;
	CCString *pRet = new CCString();
	pRet->autorelease();
	pRet->m_sString = relativeFile.substr(0, relativeFile.find_last_of("/\\") + 1);
	pRet->m_sString += pszFilename;
	return pRet->m_sString.c_str();
}



unsigned char* CCFileUtils::getFileData(const char* pszFileName, const char* pszMode, unsigned long * pSize)
{
	const char *pszPath = fullPathFromRelativePath(pszFileName);

	FILE_STANDARD_INFO fileStandardInfo = { 0 };
	HANDLE hFile;
	DWORD bytesRead = 0;
	uint32 dwFileSize = 0;
	BYTE* pBuffer = 0;

	std::wstring path = CCUtf8ToUnicode(pszPath);


	do 
	{
		CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {0};
		extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
		extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
		extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
		extendedParams.lpSecurityAttributes = nullptr;
		extendedParams.hTemplateFile = nullptr;

		// read the file from hardware
		hFile = ::CreateFile2(path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			break;
		}


		BOOL result = ::GetFileInformationByHandleEx(
			hFile,
			FileStandardInfo,
			&fileStandardInfo,
			sizeof(fileStandardInfo)
			);

		//Read error
		if ((result == 0) || (fileStandardInfo.EndOfFile.HighPart != 0))
		{
			break;
		}

		dwFileSize = fileStandardInfo.EndOfFile.LowPart;
		//for read text
		pBuffer = new BYTE[dwFileSize+1];
		pBuffer[dwFileSize] = 0;
		if (!ReadFile(hFile, pBuffer, dwFileSize, &bytesRead, nullptr))
		{
			break;
		}
		*pSize = bytesRead;

	} while (0);

	if (! pBuffer && isPopupNotify())
	{
		std::string title = "Notification";
		std::string msg = "Get data from file(";
		msg.append(pszPath).append(") failed!");

		//CCMessageBox(msg.c_str(), title.c_str());
		OutputDebugString(L"CCFileUtils_win8_metro.cpp: Get data from file failed!\n");
	}

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return pBuffer;
}

//void CCFileUtils::setResource(const char* pszZipFileName)
//{
//    CC_UNUSED_PARAM(pszZipFileName);
//    CCAssert(0, "Have not implement!");
//}

string CCFileUtils::getWriteablePath()
{
	//return the path of Appliction LocalFolor
	std::string ret;
	StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
	Platform::String^ folderPath = localFolder->Path + "\\";
	ret = CCUnicodeToUtf8(folderPath->Data());
	return ret;
}

NS_CC_END;
