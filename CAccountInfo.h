#pragma once

#include "framework.h"

#include "../GameCommon/CHttpRequest.h"
#include "../GameCommon/CLobbyServerInfo.h"
#include "../GameCommon/CString.h"

class CAccountInfo
{
public:

	CHttpRequest* m_httpRequest;
	CLobbyServerInfo* m_serverInfo;
	CString* m_url;
	
	void* m_server;
	
	CAccountInfo();
	CAccountInfo(void* server, CLobbyServerInfo* serverInfo, const char* url);
	~CAccountInfo();

	void RequestAccountInfo();
};