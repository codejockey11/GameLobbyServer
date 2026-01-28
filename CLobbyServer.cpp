#include "CLobbyServer.h"

/*
*/
CLobbyServer::CLobbyServer()
{
	memset(this, 0x00, sizeof(CLobbyServer));

	m_errorLog = new CErrorLog("C:/Users/junk_/source/repos/Game/GameLobbyServerLog.txt");

	m_networkReceive = new CNetwork();

	m_serverInfos = new CHeapArray(true, sizeof(CLobbyServerInfo), 1, CLobbyServerInfo::E_MAX_CLIENTS);

	for (int32_t i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		m_clientServerInfo->Constructor();

		m_clientServerInfo->m_socket->SetErrorLog(m_errorLog);
	}

	m_state = CLobbyServer::ServerState::E_STOPPED;

	m_event[CNetwork::LobbyEvent::E_LE_ACCOUNT_INFO] = &CLobbyServer::AccountInfo;
	m_event[CNetwork::LobbyEvent::E_LE_DISCONNECT] = &CLobbyServer::Disconnect;
	m_event[CNetwork::LobbyEvent::E_LE_MESSAGE] = &CLobbyServer::ConsoleMessage;

	m_frame[CLobbyServer::E_STOPPED] = &CLobbyServer::Stopped;
}

/*
*/
CLobbyServer::~CLobbyServer()
{
	if (m_listenThreadRunning)
	{
		CLobbyServer::Stop();
	}

	SAFE_DELETE(m_listenSocket);
	SAFE_DELETE(m_serverInfos);
	SAFE_DELETE(m_networkReceive);
	SAFE_DELETE(m_errorLog);
}

/*
*/
void CLobbyServer::AccountInfo()
{
	m_errorLog->WriteError(true, "CLobbyServer::AccountInfo:AccountThread Starting\n");

	m_accountThreadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CLobbyServer), &CLobbyServer::AccountThread, (void*)m_serverInfo, 0, nullptr);

	CloseHandle(m_accountThreadHandle);

	m_accountThreadHandle = 0;
}

/*
*/
void CLobbyServer::ConsoleMessage()
{
	m_errorLog->WriteError(true, "CLobbyServer::ConsoleMessage::%s\n", (char*)m_networkReceive->m_data);

	m_networkSend = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_CONSOLE_MESSAGE,
		(void*)m_serverInfo, sizeof(CLobbyServerInfo),
		(void*)m_networkReceive->m_data, (int32_t)strlen((char*)m_networkReceive->m_data));

	CLobbyServer::SendNetwork(m_networkSend);

	SAFE_DELETE(m_networkSend);
}

/*
*/
void CLobbyServer::CreateClient(SOCKET tempSocket)
{
	CLobbyServerInfo* acceptServerInfo = new CLobbyServerInfo();

	acceptServerInfo->Constructor();

	acceptServerInfo->m_socket->m_socket = tempSocket;

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCEPTED_LOBBY,
		(void*)acceptServerInfo, sizeof(CLobbyServerInfo),
		nullptr, 0);

	acceptServerInfo->m_socket->Send((char*)n, sizeof(CNetwork));

	SAFE_DELETE(n);

	acceptServerInfo->m_socket->SetReceiveTimeout(50);

	CNetwork network = {};

	int32_t totalBytes = acceptServerInfo->m_socket->Receive((char*)&network, sizeof(CNetwork));

	SAFE_DELETE(acceptServerInfo);

	if (totalBytes <= 0)
	{
		return;
	}

	acceptServerInfo = (CLobbyServerInfo*)&network.m_serverInfo;

	CLobbyServerInfo* serverInfo = {};

	for (int32_t i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isAvailable)
		{
			serverInfo->m_clientNumber = i;

			serverInfo->m_isAvailable = false;
			
			serverInfo->m_isConnected = true;

			serverInfo->m_socket->m_socket = tempSocket;

			serverInfo->m_socket->SetReceiveTimeout(1);

			serverInfo->SetName(acceptServerInfo->m_name);

			n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_COMPLETE_LOBBY_CONNECT,
				(void*)serverInfo, sizeof(CLobbyServerInfo),
				nullptr, 0);

			serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

			SAFE_DELETE(n);

			break;
		}
	}
}

/*
*/
void CLobbyServer::DestroyClient(CLobbyServerInfo* serverInfo)
{
	m_errorLog->WriteError(true, "CLobbyServer::DestroyClient:%s\n", serverInfo->m_name);

	serverInfo->Reset();
}

/*
*/
void CLobbyServer::Disconnect()
{
	m_errorLog->WriteError(true, "CLobbyServer::Disconnect::Client Closing Connection:%s\n", m_serverInfo->m_name);

	CLobbyServer::DestroyClient(m_serverInfo);
}

/*
*/
void CLobbyServer::Frame()
{
	for (int32_t i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isConnected)
		{
			m_bytesReceived = serverInfo->m_socket->Receive((char*)m_networkReceive, sizeof(CNetwork));

			if (m_bytesReceived > 0)
			{
				CLobbyServer::ProcessEvent();
			}
		}
	}
}

/*
*/
void CLobbyServer::InitializeWinsock()
{
	m_winsockVersion = MAKEWORD(2, 2);

	m_result = WSAStartup(m_winsockVersion, &m_wsaData);

	if (m_result != 0)
	{
		m_errorLog->WriteError(true, "CLobbyServer::InitializeWinsock::WSAStartup:%i\n", m_result);
	}

	m_errorLog->WriteError(true, "CLobbyServer::InitializeWinsock::WSAStartup:%s\n", m_wsaData.szDescription);
}

/*
*/
void CLobbyServer::ProcessEvent()
{
	m_clientServerInfo = (CLobbyServerInfo*)m_networkReceive->m_serverInfo;

	m_serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, m_clientServerInfo->m_clientNumber);

	m_serverInfo->SetServer(m_clientServerInfo);

	(this->*m_event[m_networkReceive->m_type])();
}

/*
*/
void CLobbyServer::SendNetwork(CNetwork* network)
{
	for (int32_t i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (m_clientServerInfo->m_isConnected)
		{
			m_clientServerInfo->m_socket->Send((char*)network, sizeof(CNetwork));
		}
	}
}

/*
*/
void CLobbyServer::Stop()
{
	m_state = CLobbyServer::ServerState::E_STOPPED;

	CLobbyServer::ShutdownListen();
	
	CLobbyServer::ShutdownClients();

	WSACleanup();
}

/*
*/
void CLobbyServer::ShutdownClients()
{
	for (int32_t i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		m_clientServerInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		m_clientServerInfo->Clear();
	}
}

/*
*/
void CLobbyServer::ShutdownListen()
{
	m_listenThreadRunning = false;

	m_listenSocket->ShutdownListen();

	SAFE_DELETE(m_listenSocket);
}

/*
*/
void CLobbyServer::StartServer(const char* port)
{
	m_errorLog->WriteError(true, "CLobbyServer::StartServer:%s\n", port);

	CLobbyServer::InitializeWinsock();

	m_listenSocket = new CSocket(m_errorLog);

	m_listenSocket->CreateListenSocket(port);

	m_listenSocket->Listen();

	m_listenThreadRunning = true;

	m_state = CLobbyServer::ServerState::E_LOBBY_RUNNING;

	m_errorLog->WriteError(true, "CLobbyServer::ListenThread Starting\n");

	m_listenThreadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CLobbyServer), &CLobbyServer::ListenThread, (void*)this, 0, &m_listenThreadId);

	CloseHandle(m_listenThreadHandle);

	m_listenThreadHandle = 0;
}

/*
*/
void CLobbyServer::Stopped()
{

}

/*
*/
unsigned int __stdcall CLobbyServer::AccountThread(void* obj)
{
	CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)obj;

	CAccountInfo* accountInfo = new CAccountInfo(serverInfo, "https://aviationweather.gov/api/data/metar?ids=KMCI,KJFK,KATL,KORD&format=xml");

	accountInfo->RequestAccountInfo();
	
	CListNode* node = accountInfo->m_httpRequest->m_buffers->m_list;

	int32_t totalBytes = 0;
	
	int32_t offset = 0;
	
	char* text = 0;

	while ((node) && (node->m_object))
	{
		CString* str = (CString*)node->m_object;

		while ((str->m_length - offset) > CNetwork::E_DATA_SIZE)
		{
			text = str->GetAtOffset(offset);

			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCOUNT_INFO,
				(void*)serverInfo, sizeof(CLobbyServerInfo),
				(void*)text, CNetwork::E_DATA_SIZE);

			serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

			SAFE_DELETE(n);

			totalBytes = offset;

			offset += CNetwork::E_DATA_SIZE;
		}

		if ((str->m_length - totalBytes) > 0)
		{
			text = str->GetAtOffset(offset);

			totalBytes = str->m_length - totalBytes;

			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCOUNT_INFO,
				(void*)serverInfo, sizeof(CLobbyServerInfo),
				(void*)text, totalBytes);

			serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

			SAFE_DELETE(n);
		}

		node = node->m_next;
	}

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCOUNT_INFO_END,
		(void*)serverInfo, sizeof(CLobbyServerInfo),
		nullptr, 0);

	serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

	SAFE_DELETE(n);

	SAFE_DELETE(accountInfo);

	_endthreadex(0);

	return 0;
}

/*
*/
unsigned int __stdcall CLobbyServer::ListenThread(void* obj)
{
	CLobbyServer* server = (CLobbyServer*)obj;

	while (server->m_listenThreadRunning)
	{
		SOCKET socket = server->m_listenSocket->Accept();

		if (socket)
		{
			server->CreateClient(socket);
		}
	}

	_endthreadex(0);

	return 0;
}