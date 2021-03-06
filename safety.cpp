/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: WindowsProject1.cpp
-- An application that can act as either a server or a client. The program receives data as a server by TCP.
-- The client can choose to upload and send a file via TCP or UDP. The client can specify packet size and the
-- number of times to send the file. The server saves the data to a text file named "OutputData.txt".
--
-- PROGRAM: Send/Receive
--
-- FUNCTIONS:
-- ATOM                MyRegisterClass(HINSTANCE hInstance);
-- BOOL                InitInstance(HINSTANCE, int);
-- LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
-- INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
-- INT_PTR CALLBACK		IPConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
-- INT_PTR CALLBACK		PacketConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
-- INT_PTR CALLBACK		clientConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
-- BOOL					uploadFile();
-- bool					receiverConnect();
-- bool					createPacket(std::string s);
-- void CALLBACK		handleRoutine(DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD inFlags);
-- DWORD WINAPI			ServerThread(LPVOID lpParam);
-- bool					clientConnect();
--
--
-- DATE: January 31, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai 
--
-- PROGRAMMER: Calvin Lai
--
-- NOTES:
--
----------------------------------------------------------------------------------------------------------------------*/

#pragma comment(lib, "WS3_32.lib")

#include "stdafx.h"
#include "WindowsProject1.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <commdlg.h>
#include <iostream>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>

// -- DEFINE SECTION --
#define SENDER true
#define RECEIVER false
#define BUFFERSIZE 102400

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
bool mode = NULL;
int sendCount = 10;
int bytesToSend = 1024;
char packetSize[BUFFERSIZE];
char times[BUFFERSIZE];
char portNumber[BUFFERSIZE];
char *protocol;
char IPAddress[BUFFERSIZE];
OPENFILENAME ofn;
std::queue <std::string> packetBuffer;
SOCKET acceptSocket;
std::ofstream outputFile;
int packetCount = 0;
int byteCount = 0;
// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	IPConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	PacketConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	clientConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL				uploadFile();
bool				receiverConnect(HWND, UINT);
bool				clientConnect(HWND, UINT);
bool				createPacket(std::string s);
void CALLBACK		handleRoutine(DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD inFlags);
DWORD WINAPI		ServerThread(LPVOID lpParam);

typedef struct _SOCKET_INFORMATION {
	OVERLAPPED Overlapped;
	SOCKET Socket;
	CHAR buffer[BUFFERSIZE];
	WSABUF dataBuff;
	DWORD bytesSEND;
	DWORD bytesRECV;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_USER + 1) {

	}
    switch (message)
    {

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case ID_FUNCTION_CONNECT:
				if (mode == RECEIVER) {
					receiverConnect(hWnd, message);
				}
				else {
					clientConnect(hWnd, message);
				}
				break;
			case ID_CONFIGURE_IPADDRESS:
				if (mode == SENDER) {
					DialogBox(hInst, MAKEINTRESOURCE(ID_IPCONFIG), hWnd, IPConfig);
				}
				else {
					DialogBox(hInst, MAKEINTRESOURCE(IDD_WINDOWSPROJECT1_DIALOG), hWnd, clientConfig);
				}
				break;
			case ID_CONFIGURE_PACKETSIZE:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_PACKETCONFIG), hWnd, About);
				break; 
			case ID_FUNCTION_SENDER:
				mode = SENDER;
				MessageBox(NULL, "Send mode selected", "Mode", MB_OK);
				break;
			case ID_FUNCTION_RECEIVER:
				mode = RECEIVER;
				MessageBox(NULL, "Receive mode selected", "Mode", MB_OK);
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
	outputFile.close();
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: clientConfig
--
-- DATE: February 9, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: INT_PTR CALLBACK clientConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
--
-- RETURNS: void.
--
-- NOTES:
-- Called when the config button is pressed when in client mode. Prompts user for IP Address, port, and protocol to
-- connect with.
--
----------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK clientConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (!GetDlgItemText(hDlg, IDC_IPADDRESS, IPAddress, BUFFERSIZE)) {
				*IPAddress = NULL;
			}
			if (!GetDlgItemText(hDlg, IDC_PORT, portNumber, BUFFERSIZE)) {
				*portNumber = NULL;
			}
		case IDCANCEL:
			EndDialog(hDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: IPConfig
--
-- DATE: January 31, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE:  INT_PTR CALLBACK IPConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
--
-- RETURNS: INT_PTR
--
-- NOTES:
-- Prompts the user for IP address and port to follow when in server mode.
----------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK IPConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (!GetDlgItemText(hDlg, IDC_IPADDRESS, IPAddress, BUFFERSIZE)) {
				*IPAddress = NULL;
			}
			if (!GetDlgItemText(hDlg, IDC_PORT, portNumber, BUFFERSIZE)) {
				*portNumber = NULL;
			}
			if (IsDlgButtonChecked(hDlg, IDC_UDP) == BST_CHECKED) {
				protocol = const_cast<char*> ("UDP");
			}
			else if (IsDlgButtonChecked(hDlg, IDC_TCP) == BST_CHECKED) {
				protocol = const_cast<char*> ("TCP");
			}
			else {
				MessageBox(NULL, "Select a protocol!", "Error", MB_OK);
				// Fall through
			}
		case IDCANCEL:
			EndDialog(hDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: PacketConfig
--
-- DATE: January 31, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: INT_PTR CALLBACK PacketConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
--
-- RETURNS: INT_PTR
--
-- NOTES:
-- Prompts the user to select the send count and the size of the packets.
----------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK PacketConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (!GetDlgItemText(hDlg, IDC_PACKETSIZE, packetSize, BUFFERSIZE)) {
				*packetSize = NULL;
			}
			if (!GetDlgItemText(hDlg, IDC_TIMES, times, BUFFERSIZE)) {
				*times = NULL;
			}
			sendCount = atoi(times);
			bytesToSend = atoi(packetSize);
		case IDCANCEL:
			EndDialog(hDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: receiverConnect
--
-- DATE: January 31, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: bool receiverConnect()
--
-- RETURNS: bool
--
-- NOTES:
-- Creates a socket to listen for connections. Creates a new socket to accept the connection once detected.
--
----------------------------------------------------------------------------------------------------------------------*/
bool receiverConnect(HWND hWnd, UINT msg) {

	outputFile.open("receivedData.txt");
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[BUFFERSIZE];
	int recvbuflen = BUFFERSIZE;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, "27015", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}
	MSG msg2;

	iResult = WSAAsyncSelect(ListenSocket, hWnd, WM_USER + 1, FD_READ | FD_WRITE);

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	freeaddrinfo(result);
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}
	/*
	// Translate and dispatch window messages for the application thread

	while (iResult = GetMessage(&msg2, NULL, 0, 0))
	{
		if (iResult == -1)
		{
			printf("GetMessage() failed with error %d\n", GetLastError());
			return;
		}

		TranslateMessage(&msg2);
		DispatchMessage(&msg2);
	}
	*/
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}

	// No longer need server socket
	closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			outputFile << recvbuf;
			byteCount += iResult;
			packetCount++;
			// Echo the buffer back to the sender
			/*
			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				exit(1);
			}
			printf("Bytes sent: %d\n", iSendResult);
			*/
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			exit(1);
		}

	} while (iResult > 0);


	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		exit(1);
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	outputFile.close();
	std::stringstream stream;
	stream << byteCount;
	std::string r1("Bytes received: " + stream.str());
	std::stringstream stream2;
	stream2 << packetCount;
	std::string r2("\nPackets received: " + stream2.str());
	std::string r3(r1 + r2);
	MessageBox(NULL, r3.c_str(), "Results", MB_OK);
	exit(0);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: clientConnect
--
-- DATE: February 9, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: bool clientConnect()
--
-- RETURNS: bool
--
-- NOTES:
-- Creates a socket and connects to the server when in client mode.
----------------------------------------------------------------------------------------------------------------------*/
bool clientConnect(HWND hWnd, UINT msg) {
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	const char *sendbuf = "this is a test";
	char recvbuf[BUFFERSIZE];
	int iResult;
	int recvbuflen = BUFFERSIZE;


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(IPAddress, "27015", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		if (protocol == "TCP") {
			ConnectSocket = socket(ptr->ai_family, SOCK_STREAM,
				ptr->ai_protocol);
		}
		else if (protocol == "UDP") {
			ConnectSocket = socket(PF_INET, SOCK_DGRAM,
				0);
		}
		if (ConnectSocket == INVALID_SOCKET) {
			int i = WSAGetLastError();
			printf("socket failed with error: %ld\n", WSAGetLastError());
			
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	uploadFile();
	for (int i = 0; i < sendCount; i++) {
		std::queue<std::string> temp = packetBuffer;
		while (!packetBuffer.empty()) {
			iResult = send(ConnectSocket, packetBuffer.front().c_str(), (int)strlen(packetBuffer.front().c_str()), 0);
			packetCount++;
			byteCount += strlen(packetBuffer.front().c_str());
			if (iResult == SOCKET_ERROR) {
				int i = WSAGetLastError();
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}
			packetBuffer.pop();
		}
		packetBuffer = temp;
	}

	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	std::stringstream stream;
	stream << byteCount;
	std::string r1("Bytes sent: " + stream.str());
	std::stringstream stream2;
	stream2 << packetCount;
	std::string r2("\nPackets sent: " + stream2.str());
	std::string r3(r1 + r2);
	MessageBox(NULL, r3.c_str(), "Result", MB_OK);

	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: uploadFile
--
-- DATE: February 9, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: BOOL uploadFile()
--
-- RETURNS: void.
--
-- NOTES:
-- This function prompts the user for a file to select for transferring.
----------------------------------------------------------------------------------------------------------------------*/
BOOL uploadFile() {
	char filename[MAX_PATH];

	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "Text Files\0*.txt\0Any File\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Open a file";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		std::ifstream fin;
		fin.open(ofn.lpstrFile);
		std::string s;
		std::string ans;
		while (getline(fin, s)) {
			ans.append(s);
		}
		// create packets
		if (createPacket(ans)) {
			return true;
		}
		else {
			return false;
		}

	}
	else {
		MessageBox(NULL, "Unable to open file.", "Error", MB_OK);
		return false;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ServerThread
--
-- DATE: February 9, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: DWORD WINAPI ServerThread(LPVOID lpParam);
--
-- RETURNS: DWORD
--
-- NOTES:
-- The thread function that waits for WSA events.
--
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI ServerThread(LPVOID lpParam) {
	DWORD flags;
	LPSOCKET_INFORMATION socketInfo;
	WSAEVENT eventArray[1];
	DWORD index;
	DWORD recvBytes;

	eventArray[0] = (WSAEVENT)lpParam;

	while (TRUE) {
		while (TRUE) {
			index = WSAWaitForMultipleEvents(1, eventArray, FALSE, WSA_INFINITE, TRUE);

			if (index == WSA_WAIT_FAILED) {
				MessageBox(NULL, "WSAWaitForMultipleEvents failed.", "Error!", MB_OK);
				return FALSE;
			}

			if (index != WAIT_IO_COMPLETION) {
				// accept ready
				break;
			}
		}
	}

	WSAResetEvent(eventArray[index - WSA_WAIT_EVENT_0]);

	if ((socketInfo = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
		MessageBox(NULL, "GlobalAlloc failed.", "Error!", MB_OK);
		return FALSE;
	}

	socketInfo->Socket = acceptSocket;
	ZeroMemory(&(socketInfo->Overlapped), sizeof(WSAOVERLAPPED));
	socketInfo->bytesSEND = 0;
	socketInfo->bytesRECV = 0;
	socketInfo->dataBuff.len = BUFFERSIZE;
	socketInfo->dataBuff.buf = socketInfo->buffer;

	flags = 0;
	if (WSARecv(socketInfo->Socket, &(socketInfo->dataBuff), 1, &recvBytes, &flags, &(socketInfo->Overlapped), handleRoutine) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			MessageBox(NULL, "WSARecv() failed.", "Error", MB_OK);
			exit(10);
		}
	}
	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: handleRoutine
--
-- DATE: February 9, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: void CALLBACK handleRoutine(DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD inFlags)
--
-- RETURNS: void
--
-- NOTES:
-- The routine that handles the events received by the server thread.
--
----------------------------------------------------------------------------------------------------------------------*/
void CALLBACK handleRoutine(DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD inFlags) {
	DWORD sendBytes;
	DWORD flags;

	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)overlapped;

	if (error != 0) {
		MessageBox(NULL, "I/O operation failed.", "Error", MB_OK);
	}

	if (bytesTransferred == 0) {
		MessageBox(NULL, "Closing socket.", "Closing", MB_OK);
	}

	if (error != 0 || bytesTransferred == 0) {
		closesocket(SI->Socket);
		GlobalFree(SI);
		return;
	}
	/*
	if (SI->bytesRECV == 0) {
		SI->bytesRECV = bytesTransferred;
		SI->bytesSEND = 0;
	} else {
		SI->bytesSEND += bytesTransferred;
	}

	if (SI->bytesRECV > SI->bytesSEND) {
		ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

		SI->dataBuff.buf = SI->buffer + SI->bytesSEND;
		SI->dataBuff.len = SI->bytesRECV - SI->bytesSEND;

		if (WSASend(SI->Socket, &(SI->dataBuff), 1, &sendBytes, 0, &(SI->Overlapped), handleRoutine) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				return;
			}
		}
	}
	*/

	outputFile << SI->buffer;

	flags = 0;
	ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

	SI->dataBuff.buf = SI->buffer;
	SI->dataBuff.len = BUFFERSIZE;

	if (WSARecv(SI->Socket, &(SI->dataBuff), 1, &sendBytes, 0, &(SI->Overlapped), handleRoutine) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			return;
		}
	}
	
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: createPacket
--
-- DATE: February 9, 2018
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Calvin Lai
--
-- PROGRAMMER: Calvin Lai
--
-- INTERFACE: bool createPacket(std::string s)
--
-- RETURNS: bool
--
-- NOTES:
-- Converts text data into packets to be sent. Packet size is defaulted to 1024 bytes unless specified by user.
--
----------------------------------------------------------------------------------------------------------------------*/
bool createPacket(std::string s) {
	WSABUF packet;
	char buffer[BUFFERSIZE];
	int i = 0;
	int n = 0;
	int length = 0;
	int max = s.size();
	while (i < max - 1) {
		for (n = 0; n < bytesToSend - 1 || i == s.size(); n++, i++) {
			buffer[n] = s[i];
			length++;
			if (i == s.size() - 1) {
				break;
			}
		}
		buffer[n] = '\0';
		packet.buf = buffer;
		packet.len = strlen(buffer);
		packetBuffer.push(packet.buf);
	}

	return true;
}