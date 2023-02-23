#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <mutex>

#include "command.h"
#include "stringUtil.h"

// macro
#define MAX_LEN 256
#define NUM_COLORS 6

using namespace std;

bool exit_flag = false;
thread t_send, t_recv; // lvalues
int client_socket;
int select_room_id = 0;
string def_col = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

void catch_ctrl_c(int signal);
string color(int code);
int eraseText(int cnt);
void send_message(int client_socket);
void recv_message(int client_socket);

int main()
{
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket: ");
		exit(-1);
	}

	int option = 1;
	setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_port = htons(10000);
	client.sin_addr.s_addr = INADDR_ANY;
	// client.sin_addr.s_addr = inet_addr("127.0.0.1"); // Provide IP address of server
	bzero(&client.sin_zero, 0);

	if ((connect(client_socket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == -1)
	{
		perror("connect: ");
		exit(-1);
	}

	// catch Ctrl + C event
	signal(SIGINT, catch_ctrl_c);

	// enter name and send to the server
	char name[MAX_LEN];
	cout << "Enter your name : ";
	cin.getline(name, MAX_LEN);
	send(client_socket, name, sizeof(name), 0);

	cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl
		 << def_col;

	thread t1(send_message, client_socket);
	thread t2(recv_message, client_socket);

	// bind rvalues to lvalues
	//
	// t_send  = move(t1);
	// lvalues   rvalues
	//
	// move(x)可以取得x的rvalues

	t_send = move(t1);
	t_recv = move(t2);

	if (t_send.joinable())
		t_send.join();
	if (t_recv.joinable())
		t_recv.join();

	return 0;
}

// Handler for "Ctrl + C"
void catch_ctrl_c(int signal)
{
	char str[MAX_LEN] = "#exit";
	send(client_socket, str, sizeof(str), 0);
	exit_flag = true;
	t_send.detach();
	t_recv.detach();
	close(client_socket);
	exit(signal);
}

string color(int code)
{
	return colors[code % NUM_COLORS];
}

// Erase text from terminal
int eraseText(int cnt)
{
	char back_space = 8;
	for (int i = 0; i < cnt; i++)
	{
		cout << back_space;
	}
}

// Send message to everyone
void send_message(int client_socket)
{
	while (1)
	{
		cout << colors[1] << "You : " << def_col;
		char str[MAX_LEN];
		cin.getline(str, MAX_LEN);

		if (str[0] == '#')
		{
			if (strcmp(str, EXIT) == 0)
			{
				send(client_socket, str, sizeof(str), 0);
				exit_flag = true;
				t_recv.detach();
				close(client_socket);
				return;
			}
			else if (string(str).rfind(CREATE_ROOM) == 0)
			{
				cout << colors[NUM_COLORS - 1] << "Enter room name: " << endl;
				char roomname[MAX_LEN];
				cin.getline(roomname, MAX_LEN);

				cout << colors[NUM_COLORS - 1] << "Enter room capacity: " << endl;
				char roomCapacity[MAX_LEN];
				cin.getline(roomCapacity, MAX_LEN);

				strcat(str, ":");
				strcat(str, roomname);
				strcat(str, ":");
				strcat(str, roomCapacity);
			}
		}

		send(client_socket, str, sizeof(str), 0);
	}
}

// Receive message
void recv_message(int client_socket)
{
	while (1)
	{
		if (exit_flag)
			return;
		char name[1024], str[1024];
		int color_code;

		int bytes_received = recv(client_socket, name, sizeof(name), 0);

		if (bytes_received <= 0)
			continue;

		// show server info
		if (name[0] == '#')
		{
			vector<string> splits = split(name, ":");
			string command = splits[0], msg = splits[1];

			eraseText(6); // erase "You : "
			cout << colors[NUM_COLORS - 1] << msg << endl
				 << def_col;

			// enter the room that this client has created
			
			if (strncmp(command.c_str(), CREATE_ROOM, sizeof(CREATE_ROOM)) == 0)
			{
				int roomId = atoi(splits[2].c_str());
				select_room_id = roomId;
				
				cout << colors[NUM_COLORS - 1] << "current roomID: " << select_room_id << endl
					 << def_col;
			}
			
			cout << colors[1] << "You : " << def_col;
			fflush(stdout);
		}
		else
		{
			recv(client_socket, &color_code, sizeof(color_code), 0);
			recv(client_socket, str, sizeof(str), 0);

			eraseText(6); // erase "You : "

			if (strcmp(name, "#NULL") != 0)
				cout << color(color_code) << name << " : " << def_col << str << endl;
			else
				cout << color(color_code) << str << endl;
			cout << colors[1] << "You : " << def_col;

			// When printing (e.g. printf), the output is put into a
			// buffer and may not be written to the console until a
			// newline character is displayed. To ensure that everything
			// in the buffer is written to the console, fflush(stdout) may be used.
			fflush(stdout);
		}
	}
}
