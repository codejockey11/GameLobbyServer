#pragma once

#include "framework.h"

#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CNetwork.h"
#include "../GameCommon/CLobbyServerInfo.h"
#include "../GameCommon/CMySql.h"
#include "../GameCommon/CSocket.h"

#include "CAccountInfo.h"

class CLobbyServer
{
public:

	enum ServerState
	{
		E_COUNTDOWN = 0,
		E_STOPPED,
		E_LOBBY_RUNNING,
		E_GAME_RUNNING,
		E_MAX_STATE
	};

	bool m_listenThreadRunning;

	CErrorLog* m_errorLog;
	CHeapArray* m_serverInfos;
	CLobbyServerInfo* m_serverInfo;
	CMySql* m_mySql;
	CNetwork* m_network;
	CSocket* m_listenSocket;

	HANDLE m_listenThreadHandle;
	HANDLE m_accountThreadHandle;

	int m_bytesReceived;
	int m_result;
	int m_state;

	SOCKET m_socket;

	UINT m_listenThreadId;

	WORD m_winsockVersion;

	WSADATA	m_wsaData;

	typedef void (CLobbyServer::* TMethod)();

	TMethod m_frame[CLobbyServer::E_MAX_STATE];
	TMethod m_method[CNetwork::ClientEvent::E_CE_MAX_EVENTS];

	CLobbyServer();
	~CLobbyServer();

	void AccountInfo();
	void CreateClient(SOCKET tempSocket);
	void DestroyClient(CLobbyServerInfo* serverInfo);
	void Disconnect();
	void Frame();
	void InitializeWinsock();
	void ProcessEvent(CNetwork* network);
	void Stop();
	void ShutdownClients();
	void ShutdownListen();
	void StartServer(const char* port);
	void Stopped();

	static unsigned int __stdcall AccountThread(void* obj);
	static unsigned int __stdcall ListenThread(void* obj);
};