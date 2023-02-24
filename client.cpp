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

using namespace std;

std::string def_color = "\033[0m";
std::string alert_color = "\033[31m";
std::string colors[] = {"\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"}; // 根據client id來選擇color -> id % NUM_COLORS

bool exit_flag = false;
thread t_send, t_recv; // lvalues
int client_socket;
int client_id;
int select_room_id = 0;

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
		 << def_color;

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
		cout << color(client_id) << "You : " << def_color;
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
			else if (string(str).rfind(ENTER_ROOM) == 0)
			{
				cout << colors[NUM_COLORS - 1] << "Enter the room id you want to enter: " << endl;
				char roomID[MAX_LEN];
				cin.getline(roomID, MAX_LEN);

				strcat(str, ":");
				strcat(str, roomID);
			}
		} 

		send(client_socket, str, sizeof(str), 0);
		memset(str, 0, MAX_LEN);
	}
}

// Receive message
void recv_message(int client_socket)
{
	while (1)
	{
		if (exit_flag)
			return;

		char str[MAX_LEN];
		memset(str, 0, MAX_LEN);

		int bytes_received = recv(client_socket, str, sizeof(str), 0);

		if (bytes_received <= 0)
			continue;

		// command with msg and arguments
		if (str[0] == '#')
		{
			vector<string> splits = split(str, ":");
			string command = splits[0], msg = splits[1];

			if (string(str).rfind(SET_CIENT_ID) == 0) {
				client_id = atoi(msg.c_str());
				continue;
			}

			eraseText(6); // erase "You : "
			cout << colors[NUM_COLORS - 1] << msg << endl
				 << def_color;

			// enter the room that this client has created
			if (strncmp(command.c_str(), CREATE_ROOM, sizeof(CREATE_ROOM)) == 0)
			{
				int roomId = atoi(splits[2].c_str());
				select_room_id = roomId;
				
				cout << colors[NUM_COLORS - 1] << "current roomID: " << select_room_id << endl
					 << def_color;

				// Format -> #ER:roomID
				char enterRoomMsg[MAX_LEN];
				memset(enterRoomMsg, 0, MAX_LEN);
				strcat(enterRoomMsg, ENTER_ROOM);
				strcat(enterRoomMsg, ":");
				strcat(enterRoomMsg, to_string(select_room_id).c_str());

				send(client_socket, enterRoomMsg, sizeof(enterRoomMsg), 0);
			}
			
			cout << color(client_id) << "You : " << def_color;
			fflush(stdout);
		}
		// only msg
		else if (str[0] == '*')
		{
			eraseText(6); // erase "You : "

			cout << alert_color << str << endl << def_color;
			cout << color(client_id) << "You : " << def_color;

			// When printing (e.g. printf), the output is put into a
			// buffer and may not be written to the console until a
			// newline character is displayed. To ensure that everything
			// in the buffer is written to the console, fflush(stdout) may be used.
			fflush(stdout);
		}
		else
		{
			// Format -> name:colorId:roomId:msg
			vector<string> splits = split(str, ":");
			string name = splits[0], msg = splits[3];
			int color_code = atoi(splits[1].c_str());

			eraseText(6); // erase "You : "

			cout << color(color_code) << name << " : " << def_color << msg << endl;
			cout << color(client_id) << "You : " << def_color;

			fflush(stdout);
		}
		memset(str, 0, MAX_LEN);
	}
}
