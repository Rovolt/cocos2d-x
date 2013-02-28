#include "pch.h"

#include "CCSAXParser.h"
#include "CCPlatformConfig.h"
#include "CCFileUtils.h"
#include "tinyxml\tinyxml.h"
#include <vector>
NS_CC_BEGIN;

class CC_DLL XmlSaxHander : public TiXmlVisitor
{
public:
	XmlSaxHander():m_ccsaxParserImp(0){};

	virtual bool VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute );
	virtual bool VisitExit( const TiXmlElement& element );
	virtual bool Visit( const TiXmlText& text );
	virtual bool Visit( const TiXmlUnknown&){ return true; }

	void setCCSAXParserImp(CCSAXParser* parser)
	{
		m_ccsaxParserImp = parser;
	}

private:
	CCSAXParser *m_ccsaxParserImp;
};


bool XmlSaxHander::VisitEnter( const TiXmlElement& element, const TiXmlAttribute* firstAttribute )
{
	//CCLog(" VisitEnter %s",element.Value());

	std::vector<const char*> attsVector;
	for( const TiXmlAttribute* attrib = firstAttribute; attrib; attrib = attrib->Next() )
	{
		//CCLog("%s", attrib->Name());
		attsVector.push_back(attrib->Name());
		//CCLog("%s",attrib->Value());
		attsVector.push_back(attrib->Value());
	}
	attsVector.push_back(nullptr);

	CCSAXParser::startElement(m_ccsaxParserImp, (const CC_XML_CHAR *)element.Value(), (const CC_XML_CHAR **)(&attsVector[0]));
	return true;
}
bool XmlSaxHander::VisitExit( const TiXmlElement& element )
{
	//CCLog("VisitExit %s",element.Value());

	CCSAXParser::endElement(m_ccsaxParserImp, (const CC_XML_CHAR *)element.Value());
	return true;
}

bool XmlSaxHander::Visit( const TiXmlText& text )
{
	//CCLog("Visit %s",text.Value());
	CCSAXParser::textHandler(m_ccsaxParserImp, (const CC_XML_CHAR *)text.Value(), text.ValueTStr().size());
	return true;
}


CCSAXParser::CCSAXParser()
{
	m_pDelegator = NULL;

}

CCSAXParser::~CCSAXParser(void)
{
}

bool CCSAXParser::init(const char *pszEncoding)
{
    //CC_UNUSED_PARAM(pszEncoding);
	// nothing to do
	return true;
}

bool CCSAXParser::parse(const char *pszFile)
{
	bool bRet = false;
    unsigned long size = 0;
    char* pBuffer = (char*)CCFileUtils::sharedFileUtils()->getFileData(pszFile, "rt", &size);
    if (pBuffer == NULL || size == 0)
    {
        return false;
    }

		
	TiXmlDocument tinyDoc;
	tinyDoc.Parse(pBuffer,0,TIXML_ENCODING_UTF8);
	XmlSaxHander printer;
	printer.setCCSAXParserImp(this);
	return tinyDoc.Accept( &printer );	
}

void CCSAXParser::startElement(void *ctx, const CC_XML_CHAR *name, const CC_XML_CHAR **atts)
{
	((CCSAXParser*)(ctx))->m_pDelegator->startElement(ctx, (char*)name, (const char**)atts);
}

void CCSAXParser::endElement(void *ctx, const CC_XML_CHAR *name)
{
	((CCSAXParser*)(ctx))->m_pDelegator->endElement(ctx, (char*)name);
}
void CCSAXParser::textHandler(void *ctx, const CC_XML_CHAR *name, int len)
{
	((CCSAXParser*)(ctx))->m_pDelegator->textHandler(ctx, (char*)name, len);
}
void CCSAXParser::setDelegator(CCSAXDelegator* pDelegator)
{
	m_pDelegator = pDelegator;
}

NS_CC_END;