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

	CMySql* m_mySql;
	CErrorLog* m_errorLog;
	CHeapArray* m_serverInfos;
	CNetwork* m_network;
	CLobbyServerInfo* m_serverInfo;

	CSocket* m_listenSocket;
	SOCKET m_socket;

	bool m_listenThreadRunning;
	
	WSADATA	m_wsaData;

	typedef void (CLobbyServer::* TMethod)();

	TMethod m_method[CNetwork::ClientEvent::E_CE_MAX_EVENTS];

	TMethod m_frame[CLobbyServer::E_MAX_STATE];

	int m_state;

	CLobbyServer();
	~CLobbyServer();

	// Base
	void CreateClient(SOCKET tempSocket);
	void DestroyClient(CLobbyServerInfo* serverInfo);
	void EstablishClient();
	void Frame();
	void InitializeWinsock();
	void ProcessEvent(CNetwork* network);
	void ReceiveSocket(CLobbyServerInfo* serverInfo);
	void Shutdown();
	void ShutdownClients();
	void ShutdownListen();
	void StartServer(const char* port);

	// Network
	void AccountInfo();
	void Disconnect();

	// Server State
	void Stopped();

	static unsigned int __stdcall ListenThread(void* obj);
	static unsigned int __stdcall AccountThread(void* obj);

private:

	HANDLE m_listenThreadHandle;
	
	UINT m_listenThreadId;

	ADDRINFO* m_addrResult = {};
	ADDRINFO* m_addrPtr = {};
	ADDRINFO  m_addrHints = {};
};