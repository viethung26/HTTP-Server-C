// HttpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include <iostream>
#include <WinSock2.h>
#define ROOTDIR "D:/"

DWORD WINAPI ClientThread(LPVOID arg);
char* response = NULL;
char* DataBase = NULL;
int port = 5000;

int main()
{
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	SOCKET server;
	SOCKADDR_IN serverAddr;
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	listen(server, 10);
	while (true) {
		SOCKADDR_IN clientAddr;
		int clen = sizeof(clientAddr);
		SOCKET client = accept(server, (SOCKADDR*)&clientAddr, &clen);
		DWORD id = 0;
		CreateThread(NULL, 0, ClientThread, (LPVOID)client, 0, &id);
	}
	WSACleanup();
}
VOID AddData(const char *str) {
	//Su dung de add data vao bien DataBase danh cho method post
	char *tmp = (char*)calloc(1024, 1);
	sprintf(tmp, "%s", str);
	int oldLen = DataBase == NULL ? 0 : strlen(DataBase);
	int newLen = oldLen + strlen(tmp) + 1;
	DataBase = (char*)realloc(DataBase, newLen);
	memset(DataBase + oldLen, 0, newLen - oldLen);
	strcpy(DataBase + oldLen, tmp);
	free(tmp);
	tmp = NULL;
}
VOID AppendText(const char *str) {
	//Them text vao response cho Get method
	char *tmp = (char*)calloc(1024, 1);
	sprintf(tmp, "%s", str);
	int oldLen = response == NULL ? 0 : strlen(response);
	int newLen = oldLen + strlen(tmp)+1;
	response = (char*)realloc(response, newLen);
	memset(response + oldLen, 0, newLen - oldLen);
	strcpy(response + oldLen, tmp);
	free(tmp);
	tmp = NULL;
}
VOID AppendLink(BOOL isFolder, const char* path, const char *str) {
	// Them link vao response cho get method
	char *tmp = (char*)calloc(1024, 1);
	const char * src = isFolder ? "http://localhost:5000/folder.png" : "http://localhost:5000/file.png";
	if(strlen(path)>0)
	sprintf(tmp, "<img src=\"%s\" height=\"20px\"/><a href=\"http://localhost:%d/%s/%s\">%s</a></br>",src, port, path, str, str);
	else sprintf(tmp, "<img src=\"%s\" height=\"20px\"/><a href=\"http://localhost:%d/%s\"/>%s</a></br>",src, port, str, str);
	int oldLen = response == NULL ? 0 : strlen(response);
	int newLen = oldLen + strlen(tmp) + 1;
	response = (char*)realloc(response, newLen);
	memset(response + oldLen, 0, newLen - oldLen);
	strcpy(response + oldLen, tmp);
	free(tmp);
	tmp = NULL;
}
DWORD WINAPI ClientThread(LPVOID arg) {
	SOCKET client = (SOCKET)arg;
	char buf[1024];
	char verb[1024];
	char path[1024];
	char http[1024];
	memset(buf, 0, sizeof(buf));
	memset(verb, 0, sizeof(verb));
	memset(path, 0, sizeof(path));
	memset(http, 0, sizeof(http));
	recv(client, buf, sizeof(buf), 0);
	sscanf(buf, "%s%s%s", verb, path, http);
	printf("%s\n", buf);
	char *index = NULL;
	//Check Method cua Request
	if (strcmp(verb, "get")==0|| strcmp(verb, "GET")==0) {
	// Xu li method GET
		if (strcmp(path, "/data") == 0) {
		// Get data
			const char* header = "HTTP/1.1 200\r\nContent-Type: text/plain\r\n\r\n";
			send(client, header, strlen(header), 0);
			if (DataBase) {
				AppendText("[");
				AppendText(DataBase);
				AppendText("]");
			}
			else AppendText("");
			send(client, response, strlen(response), 0);
		}
		else {
		// Get file and folder on server
			// thay %20 boi ki tu white space
			do {
				index = strstr(path, "%20");
				if (index != NULL) {
					strcpy(index, index + 2);
					index[0] = ' ';
				}
			} while (index);
			char dir[1024];
			memset(dir, 0, sizeof(dir));
			strcpy(dir, ROOTDIR);
			strcpy(path, path + 1); // bo '/' o truoc
			if (strcmp(path + (strlen(path) - 1), "/") == 0) path[strlen(path) - 1] = '\0'; // Bo '/' o sau
			strcat(dir, path);
			// kiem tra thuoc tinh la file hay folder
			DWORD fileAttr = GetFileAttributesA(dir);
			if (fileAttr & FILE_ATTRIBUTE_OFFLINE) {
				printf("loi");
				const char* header = "HTTP/1.1 404\r\nContent-Type: text/html\r\n\r\n";
				send(client, header, strlen(header), 0);
				AppendText("<html><body><h1>404 ERROR</h1><p>File not found</p></body></html>");
				send(client, response, strlen(response), 0);
			}
			else {
				if (fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
					//Xu li cho folder
						//add Header cho response
					const char* header = "HTTP/1.1 200\r\nContent-Type: text/html\r\n\r\n";
					send(client, header, strlen(header), 0);
					AppendText("<html>");
					AppendText("<head><style>*{margin-bottom: 8px}body{margin: 16px}\nimg{vertical-align:middle}\na{margin-left: 8px;text-decoration:none;color:blue;}\na:hover{font-size:24px;color:red;background-color:yellow}</style></head>");
					AppendText("<body>");
					char * heading = (char*)calloc(1024, 1);
					sprintf(heading, "<h2>Folder path: /%s</h2>", path);
					AppendText(heading);
					//them regex de tim kiem trong folder
					if (strlen(path) > 0) strcat(dir, "/*.*");
					else strcat(dir, "*.*");
					WIN32_FIND_DATAA fData;
					HANDLE hFind = FindFirstFileA(dir, &fData);
					BOOL next = false;
					// Liet ke cac file va folder
					do {
						if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) AppendLink(true, path, fData.cFileName);
						else AppendLink(false, path, fData.cFileName);
						next = FindNextFileA(hFind, &fData);
					} while (next);
					AppendText("</body></html>");
					send(client, response, strlen(response), 0);
				}
				else {
					// Xu li cho file
					if (strstr(dir, ".mp3") || strstr(dir, ".wav")) {
						const char* header = "HTTP/1.1 200\r\nContent-Type: audio/mp3\r\n\r\n";
						send(client, header, strlen(header), 0);
					}
					else if (strstr(dir, ".mp4")) {
						const char* header = "HTTP/1.1 200\r\nContent-Type: video/mp4\r\n\r\n";
						send(client, header, strlen(header), 0);
					}
					else if (strstr(dir, ".png") || strstr(dir, ".jpg") || strstr(dir, ".gif") || strstr(dir, ".jpeg") || strstr(dir, ".bmp")) {
						const char* header = "HTTP/1.1 200\r\nContent-Type: image/png\r\n\r\n";
						send(client, header, strlen(header), 0);
					}
					else if (strstr(dir, ".pdf")) {
						const char* header = "HTTP/1.1 200\r\nContent-Type: application/pdf\r\n\r\n";
						send(client, header, strlen(header), 0);
					}
					else if (strstr(dir, ".html")) {
						const char* header = "HTTP/1.1 200\r\nContent-Type: text/html\r\n\r\n";
						send(client, header, strlen(header), 0);
					}
					else {
						const char* header = "HTTP/1.1 200\r\nContent-Type: text/plain\r\n\r\n";
						send(client, header, strlen(header), 0);
					}
					// Doc va truyen file
					FILE* f = fopen(dir, "rb");
					char* buf = (char*)calloc(1024, 1);
					while (!feof(f)) {
						int n = fread(buf, 1, 1024, f);
						if (n > 0) send(client, buf, n, 0);
					}
					fclose(f);
				}
			}
			
		}
	}
	else if(strcmp(verb, "post") == 0 || strcmp(verb, "POST") == 0){
	// Xu li method POST
		char* requestBody = strstr(buf, "\r\n\r\n")+4;
		if(strcmp(path,"/chat")==0){
		// Neu post vs path la /chat thi add data post vao dataBase
			const char* header = "HTTP/1.1 200\r\nContent-Type: text/plain\r\n\r\n";
			send(client, header, strlen(header), 0);
			if(DataBase!=NULL) AddData(",");
			AddData(requestBody);
			
			AppendText(requestBody);
			send(client, response, strlen(response), 0);
		}
		else {
		// Neu post len nhung path khac thi chi tra ve data nhan duoc
			const char* header = "HTTP/1.1 200\r\nContent-Type: text/html\r\n\r\n";
			send(client, header, strlen(header), 0);
			AppendText("<html><body>");
			AppendText("<p> Your data: \"");
			AppendText(requestBody);
			AppendText("\"</p>");
			AppendText("</body></html>");

			send(client, response, strlen(response), 0);
		}
		
	}
	else {
	// Cac Method khac bao error khong ho tro
		const char* header = "HTTP/1.1 200\r\nContent-Type: text/html\r\n\r\n";
		send(client, header, strlen(header), 0);
		AppendText("<html><body>");
		AppendText("<h3>Error</h3>");
		AppendText("<p>Du lieu gui den loi hoac method khong phu hop</p>");
		AppendText("</body></html>");

		send(client, response, strlen(response), 0);
	}
	free(response);
	response = NULL;
	closesocket(client);
	return 0;
}
