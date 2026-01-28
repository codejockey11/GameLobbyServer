#include "CAccountInfo.h"

/*
*/
CAccountInfo::CAccountInfo()
{
	memset(this, 0x00, sizeof(CAccountInfo));
}

/*
*/
CAccountInfo::CAccountInfo(CLobbyServerInfo* serverInfo, const char* url)
{
	memset(this, 0x00, sizeof(CAccountInfo));

	m_serverInfo = serverInfo;

	m_httpRequest = new CHttpRequest();

	m_url = new CString(url);
}

/*
*/
CAccountInfo::~CAccountInfo()
{
	SAFE_DELETE(m_url);
	SAFE_DELETE(m_httpRequest);
}

/*
*/
void CAccountInfo::RequestAccountInfo()
{
	m_httpRequest->UrlRequest(m_url->m_text);
}