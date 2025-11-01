#include "CLobbyServer.h"

/*
*/
CLobbyServer::CLobbyServer()
{
	memset(this, 0x00, sizeof(CLobbyServer));

	m_errorLog = new CErrorLog("C:/Users/junk_/source/repos/Game/GameLobbyServerLog.txt");

	m_network = new CNetwork();

	m_serverInfos = new CHeapArray(true, sizeof(CLobbyServerInfo), 1, CLobbyServerInfo::E_MAX_CLIENTS);

	for (int i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		serverInfo->Constructor();

		serverInfo->m_socket->SetErrorLog(m_errorLog);

		serverInfo->m_isAvailable = true;
	}

	m_state = CLobbyServer::ServerState::E_STOPPED;


	m_method[CNetwork::AccountServerEvent::E_ASE_ACCOUNT_INFO] = &CLobbyServer::AccountInfo;
	m_method[CNetwork::AccountServerEvent::E_ASE_DISCONNECT] = &CLobbyServer::Disconnect;

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

	delete m_listenSocket;
	delete m_serverInfos;
	delete m_network;
	delete m_errorLog;
}

/*
*/
void CLobbyServer::AccountInfo()
{
	//CAccountInfo* accountInfo = new CAccountInfo(this, m_serverInfo, "https://aviationweather.gov/api/data/metar?ids=KPKB&format=xml");
	CAccountInfo* accountInfo = new CAccountInfo(this, m_serverInfo, "https://www.google.com");

	m_errorLog->WriteError(true, "CLobbyServer::AccountThread Starting\n");

	m_accountThreadHandle = (HANDLE)_beginthreadex(
		NULL,
		sizeof(CAccountInfo),
		&CLobbyServer::AccountThread,
		(void*)accountInfo,
		0,
		nullptr);

	if (m_accountThreadHandle == 0)
	{
		m_errorLog->WriteError(true, "CLobbyServer::AccountInfo::_beginthreadex\n");

		delete accountInfo;

		return;
	}

	WaitForSingleObject(m_accountThreadHandle, INFINITE);

	delete accountInfo;
}

/*
*/
void CLobbyServer::CreateClient(SOCKET tempSocket)
{
	CLobbyServerInfo* acceptServerInfo = new CLobbyServerInfo();

	acceptServerInfo->Constructor();

	acceptServerInfo->m_socket->m_socket = tempSocket;

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCEPTED_ACCOUNT,
		(void*)acceptServerInfo, sizeof(CLobbyServerInfo),
		nullptr, 0);

	acceptServerInfo->m_socket->Send((char*)n, sizeof(CNetwork));

	delete n;

	acceptServerInfo->m_socket->SetReceiveTimeout(5000);

	CNetwork network = {};

	int totalBytes = acceptServerInfo->m_socket->Receive((char*)&network, sizeof(CNetwork));

	delete acceptServerInfo;

	if (totalBytes <= 0)
	{
		return;
	}

	acceptServerInfo = (CLobbyServerInfo*)&network.m_serverInfo;

	CLobbyServerInfo* serverInfo = {};

	for (int i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isAvailable)
		{
			serverInfo->m_clientNumber = i;

			serverInfo->m_isAvailable = false;
			serverInfo->m_isConnected = true;

			serverInfo->m_socket->m_socket = tempSocket;

			serverInfo->m_socket->SetReceiveTimeout(50);

			serverInfo->SetName(acceptServerInfo->m_name);

			break;
		}
	}
}

/*
*/
void CLobbyServer::DestroyClient(CLobbyServerInfo* serverInfo)
{
	m_errorLog->WriteError(true, "CLobbyServer::DestroyClient:%s\n", serverInfo->m_name);

	serverInfo->m_socket->Shutdown();

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
	for (int i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isConnected)
		{
			m_bytesReceived = serverInfo->m_socket->Receive((char*)m_network, sizeof(CNetwork));

			if (m_bytesReceived > 0)
			{
				CLobbyServer::ProcessEvent(m_network);
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
void CLobbyServer::ProcessEvent(CNetwork* network)
{
	m_network = (CNetwork*)network;

	CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)m_network->m_serverInfo;

	m_serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, serverInfo->m_clientNumber);

	m_serverInfo->SetServer(serverInfo);

	(this->*m_method[m_network->m_type])();
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
	for (int i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		serverInfo->Clear();
	}
}

/*
*/
void CLobbyServer::ShutdownListen()
{
	m_listenThreadRunning = false;

	m_listenSocket->ShutdownListen();

	m_errorLog->WriteError(true, "CLobbyServer::ListenThread Ending\n");

	WaitForSingleObject(m_listenThreadHandle, INFINITE);

	CloseHandle(m_listenThreadHandle);

	m_listenThreadHandle = 0;

	delete m_listenSocket;
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

	m_listenThreadHandle = (HANDLE)_beginthreadex(NULL, sizeof(CLobbyServer),
		&CLobbyServer::ListenThread,
		(void*)this,
		0,
		&m_listenThreadId);
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
	CAccountInfo* accountInfo = (CAccountInfo*)obj;

	CLobbyServerInfo* serverInfo = accountInfo->m_serverInfo;

	accountInfo->RequestAccountInfo();
	
	CString* buffer = new CString("");
	
	CLinkListNode<CString>* bufferNode = accountInfo->m_httpRequest->m_buffers->m_list;

	while (bufferNode->m_object)
	{
		buffer->Append(bufferNode->m_object->m_text);

		bufferNode = bufferNode->m_next;
	}

	if (buffer->m_length > CNetwork::E_DATA_SIZE)
	{
		int length = buffer->m_length;
		int count = length / CNetwork::E_DATA_SIZE;
		int position = 0;

		for (int i = 0; i < count; i++)
		{
			position = i * CNetwork::E_DATA_SIZE;
			length -= CNetwork::E_DATA_SIZE;

			CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCOUNT_INFO,
				(void*)serverInfo, sizeof(CLobbyServerInfo),
				(void*)&buffer->m_text[position], CNetwork::E_DATA_SIZE);

			serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

			delete n;
		}

		position += CNetwork::E_DATA_SIZE;

		CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCOUNT_INFO,
			(void*)serverInfo, sizeof(CLobbyServerInfo),
			(void*)&buffer->m_text[position], length);

		serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

		delete n;

		n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCOUNT_INFO_END,
			(void*)serverInfo, sizeof(CLobbyServerInfo),
			nullptr, 0);

		serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

		delete n;
	}

	delete buffer;

	_endthreadex(0);

	return 1;
}

/*
*/
unsigned __stdcall CLobbyServer::ListenThread(void* obj)
{
	CLobbyServer* server = (CLobbyServer*)obj;

	while (server->m_listenThreadRunning)
	{
		SOCKET socket = server->m_listenSocket->Accept();

		if (socket)
		{
			server->CreateClient(socket);
		}
		else
		{
			closesocket(socket);
		}
	}

	_endthreadex(0);

	return 1;
}