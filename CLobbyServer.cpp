#include "CLobbyServer.h"

/*
*/
CLobbyServer::CLobbyServer()
{
	memset(this, 0x00, sizeof(CLobbyServer));

	m_errorLog = new CErrorLog("C:/Users/junk_/source/repos/Game/GameLobbyServerLog.txt");

	m_network = new CNetwork();

	m_serverInfos = new CHeapArray(sizeof(CLobbyServerInfo), true, true, 1, CLobbyServerInfo::E_MAX_CLIENTS);

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
	CLobbyServer::Shutdown();

	delete m_listenSocket;
	delete m_serverInfos;
	delete m_network;
	delete m_errorLog;
}

/*
*/
void CLobbyServer::AccountInfo()
{
	CAccountInfo* accountInfo = new CAccountInfo(this, m_serverInfo, "https://aviationweather.gov/api/data/metar?ids=KPKB&format=xml");

	HANDLE handle = (HANDLE)_beginthreadex(
		NULL,
		sizeof(CAccountInfo),
		&CLobbyServer::AccountThread,
		(void*)accountInfo,
		0,
		nullptr);

	if (handle == 0)
	{
		m_errorLog->WriteError(true, "CLobbyServer::AccountInfo::_beginthreadex\n");

		delete accountInfo;

		return;
	}

	WaitForSingleObject(handle, INFINITE);

	delete accountInfo;
}

/*
*/
void CLobbyServer::CreateClient(SOCKET tempSocket)
{
	CLobbyServerInfo* serverInfo = new CLobbyServerInfo();

	serverInfo->Constructor();

	serverInfo->m_socket->m_socket = tempSocket;

	m_socket = tempSocket;

	CNetwork* n = new CNetwork(CNetwork::ServerEvent::E_SE_TO_CLIENT, CNetwork::ClientEvent::E_CE_ACCEPTED_ACCOUNT,
		(void*)serverInfo, sizeof(CLobbyServerInfo),
		nullptr, 0);

	serverInfo->m_socket->Send((char*)n, sizeof(CNetwork));

	delete n;

	serverInfo->m_socket->SetReceiveTimeout(100);

	int totalBytes = serverInfo->m_socket->Receive((char*)m_network, sizeof(CNetwork));

	if (totalBytes <= 0)
	{
		delete serverInfo;

		return;
	}

	m_serverInfo = (CLobbyServerInfo*)m_network->m_serverInfo;

	CLobbyServer::EstablishClient();

	delete serverInfo;
}

/*
*/
void CLobbyServer::EstablishClient()
{
	CLobbyServerInfo* serverInfo = {};

	for (int i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isAvailable)
		{
			serverInfo->m_clientNumber = i;
			
			serverInfo->m_isAvailable = false;
			serverInfo->m_isRunning = true;

			serverInfo->m_socket->m_socket = m_socket;
			
			serverInfo->SetName(m_serverInfo->m_name);

			break;
		}
	}
}

/*
*/
void CLobbyServer::Frame()
{
	for (int i = 0; i < CLobbyServerInfo::E_MAX_CLIENTS; i++)
	{
		CLobbyServerInfo* serverInfo = (CLobbyServerInfo*)m_serverInfos->GetElement(1, i);

		if (serverInfo->m_isRunning)
		{
			CLobbyServer::ReceiveSocket(serverInfo);
		}
	}
}

/*
*/
void CLobbyServer::InitializeWinsock()
{
	WORD wVersionRequested = MAKEWORD(2, 2);

	int result = WSAStartup(wVersionRequested, &m_wsaData);

	if (result != 0)
	{
		m_errorLog->WriteError(true, "CLobbyServer::InitializeWinsock::WSAStartup:%i\n", result);
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
void CLobbyServer::DestroyClient(CLobbyServerInfo* serverInfo)
{
	m_errorLog->WriteError(true, "CLobbyServer::DestroyClient::Client Closing Connection:%s\n", serverInfo->m_name);

	serverInfo->m_socket->Shutdown();

	serverInfo->Reset();
}

/*
*/
void CLobbyServer::Disconnect()
{
	m_errorLog->WriteError(true, "CLobbyServer::Disconnect::Client Closing Connection:%s\n", m_serverInfo->m_name);

	m_serverInfo->m_isRunning = false;

	m_serverInfo->m_socket->Shutdown();
}

/*
*/
void CLobbyServer::ReceiveSocket(CLobbyServerInfo* serverInfo)
{
	int totalBytes = serverInfo->m_socket->Receive((char*)m_network, sizeof(CNetwork));

	if (totalBytes > 0)
	{
		CLobbyServer::ProcessEvent(m_network);
	}
	else if (totalBytes == 0)
	{
		CLobbyServer::DestroyClient(serverInfo);
	}
	else if (WSAGetLastError() == WSAETIMEDOUT)
	{
	}
	else if (totalBytes == INVALID_SOCKET)
	{
		CLobbyServer::DestroyClient(serverInfo);
	}
	else if (totalBytes == SOCKET_ERROR)
	{
		CLobbyServer::DestroyClient(serverInfo);
	}
}

/*
*/
void CLobbyServer::Shutdown()
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

		if (serverInfo->m_isRunning)
		{
			serverInfo->m_socket->Shutdown();
		}
	}
}

/*
*/
void CLobbyServer::ShutdownListen()
{
	m_listenThreadRunning = false;

	m_listenSocket->Shutdown();


	WaitForSingleObject(m_listenThreadHandle, 500);

	CloseHandle(m_listenThreadHandle);

	m_listenThreadHandle = 0;
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

	CLobbyServer* server = (CLobbyServer*)accountInfo->m_server;

	CLobbyServerInfo* serverInfo = accountInfo->m_serverInfo;


	server->m_errorLog->WriteError(true, "CLobbyServer::AccountThread Starting\n");

	accountInfo->RequestAccountInfo();

	
	CString* buffer = new CString("");

	
	CLinkListNode<CString>* bufferNode = accountInfo->m_httpRequest->m_buffers->m_list;

	while (bufferNode->m_object)
	{
		buffer->Append(bufferNode->m_object->m_text);

		bufferNode = bufferNode->m_next;
	}

#ifdef _DEBUG
	server->m_errorLog->WriteBytes(buffer->m_text);
#endif

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


	server->m_errorLog->WriteError(true, "CLobbyServer::AccountThread Ending\n");

	_endthreadex(0);


	return 1;
}

/*
*/
unsigned __stdcall CLobbyServer::ListenThread(void* obj)
{
	CLobbyServer* server = (CLobbyServer*)obj;

	server->m_errorLog->WriteError(true, "CLobbyServer::ListenThread Starting\n");

	while (server->m_listenThreadRunning)
	{
		SOCKET socket = server->m_listenSocket->Accept();

		if (socket)
		{
			server->CreateClient(socket);
		}
	}


	server->m_errorLog->WriteError(true, "CLobbyServer::ListenThread Ending\n");

	_endthreadex(0);

	return 1;
}