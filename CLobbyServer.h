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
	CLobbyServerInfo* m_clientServerInfo;
	CLobbyServerInfo* m_serverInfo;
	CMySql* m_mySql;
	CNetwork* m_networkReceive;
	CNetwork* m_networkSend;
	CSocket* m_listenSocket;

	HANDLE m_listenThreadHandle;
	HANDLE m_accountThreadHandle;

	SOCKET m_socket;

	int32_t m_bytesReceived;
	int32_t m_result;
	int32_t m_state;

	uint16_t m_winsockVersion;

	uint32_t m_listenThreadId;

	WSADATA	m_wsaData;

	typedef void (CLobbyServer::* TMethod)();

	TMethod m_event[CNetwork::LobbyEvent::E_LE_MAX];
	TMethod m_frame[CLobbyServer::ServerState::E_MAX_STATE];

	CLobbyServer();
	~CLobbyServer();

	void AccountInfo();
	void CreateClient(SOCKET tempSocket);
	void ConsoleMessage();
	void DestroyClient(CLobbyServerInfo* serverInfo);
	void Disconnect();
	void Frame();
	void InitializeWinsock();
	void ProcessEvent();
	void SendNetwork(CNetwork* network);
	void Stop();
	void ShutdownClients();
	void ShutdownListen();
	void StartServer(const char* port);
	void Stopped();

	static unsigned int __stdcall AccountThread(void* obj);
	static unsigned int __stdcall ListenThread(void* obj);
};