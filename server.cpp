#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <map>
#include <stdlib.h>
#include <set>

#include "command.h"
#include "stringUtil.h"

// macro
#define MAX_ROOMs 3
#define MAX_CLIENTS 15

using namespace std;

std::string def_color = "\033[0m";
std::string alert_color = "\033[31m";
std::string colors[] = {"\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"}; // 根據client id來選擇color -> id % NUM_COLORS

struct client
{
	string name; // client name
	int socket;	 // a socket for this client
	thread th;	 // a thread for this client
	int select_room_id = 0;
};

struct room
{
	string name;  // room name
	int capacity; // root capacity
	int owner_id; // who own this room
	set<int> clients;
};

map<int, client> clients;
map<int, room> rooms; // this room is lobby if and only if roomid == 0

int seed = 0;
int roomId = 0; // client id
mutex cout_mtx, clients_mtx, rooms_mtx;

string color(int code);
void set_name(int id, char name[]);
void shared_print(string str, bool endLine);
int broadcast_message(string message, int sender_id);
int broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);

int main()
{
	rooms[roomId++] = {"Lobby", MAX_CLIENTS, -1};

	// initial server socket
	int server_socket;
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket: ");
		exit(EXIT_FAILURE);
	}

	int option = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(10000);
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(&server.sin_zero, 0); // It binds the socket to all available interfaces.

	// bind
	if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
	{
		perror("bind error: ");
		exit(-1);
	}

	// listen
	if ((listen(server_socket, MAX_CLIENTS)) == -1)
	{
		perror("listen error: ");
		exit(-1);
	}

	struct sockaddr_in client;
	int client_socket;
	unsigned int len = sizeof(sockaddr_in);

	cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl
		 << def_color;

	cout << colors[NUM_COLORS - 1] << "Room: " << rooms[0].name << " is estiblished" << endl
		 << def_color;

	// listen for the incoming client
	while (1)
	{
		// accept
		// create a new socket for the incoming client
		if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;
		// create a new thread for the new client socket
		// thread(function, arg1, arg2, ...)
		thread t(handle_client, client_socket, seed);

		//   init lock_guard ->   lock mutex
		// deinit lock_guard -> unlock mutex
		lock_guard<mutex> guard(clients_mtx);

		// add new client to the clients
		// move??? xvalue???
		clients[seed] = {string("Anonymous"), client_socket, (move(t))};
	}

	// release all clients before closing server socket
	// 使用join()時，main thread會賭塞，等待目前被調用的thread終止，然後main thread回收被調用的thread之資源
	for (auto &c : clients)
	{
		if (c.second.th.joinable())
		{
			c.second.th.join();
		}
	}

	// close the server socket
	close(server_socket);
	return 0;
}

string color(int code)
{
	return colors[code % NUM_COLORS];
}

// Set name of client
void set_name(int id, char name[])
{
	clients[id].name = string(name);
}

// For synchronisation of cout statements
void shared_print(string str, bool endLine = true)
{
	lock_guard<mutex> guard(cout_mtx);
	cout << str;
	if (endLine)
		cout << endl;
}

// Broadcast message to all clients except the sender
int broadcast_message(string message, int sender_id, int room_id)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());

	for (auto &clientID : rooms[room_id].clients)
	{
		if (clientID != sender_id)
		{
			send(clients[clientID].socket, temp, sizeof(temp), 0);
		}
	}
}

// Broadcast a number to all clients except the sender
int broadcast_message(int num, int sender_id, int room_id)
{
	for (auto &clientID : rooms[room_id].clients)
	{
		if (clientID != sender_id)
		{
			send(clients[clientID].socket, &num, sizeof(num), 0);
		}
	}
}

void end_connection(int id)
{
	// 使用detach()時，main thread繼續進行，被調用thread會在後台繼續進行，main thread無法在取得該thread的控制權
	// 當該thread結束時，runtime library會回收該thread的相關資源
	lock_guard<mutex> guard(clients_mtx);
	clients[id].th.detach();
	close(clients[id].socket);
	clients.erase(id);
}

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN], str[MAX_LEN], msg[MAX_LEN];

	// get client name
	recv(client_socket, name, sizeof(name), 0);
	set_name(id, name);

	// set client id
	char setId[MAX_LEN];
	combine(setId, ":", {SET_CIENT_ID, to_string(id).c_str()});
	send(client_socket, setId, sizeof(setId), 0);

	// welcome message
	string welcome_message = color(id) + string("*") + string(name) + string(" has joined");

	shared_print(color(id) + welcome_message + def_color);
	broadcast_message(welcome_message, id, 0);

	// recv msg from this client
	while (1)
	{
		if (recv(client_socket, str, sizeof(str), 0) <= 0)
			return;

		// Handle commands
		if (str[0] == '#')
		{
			/****** EXIT = "#exit" ******/
			if (strcmp(str, EXIT) == 0)
			{
				// Display leaving message
				string message = string(name) + string(" has left");
				// broadcast_message("#NULL", id);
				// broadcast_message(id, id);
				// broadcast_message(message, id);
				shared_print(color(id) + message + def_color);
				end_connection(id);
				return;
			}

			/****** SHOW_ROOMS = "#SR" ******/
			if (strcmp(str, SHOW_ROOMS) == 0)
			{
				string roomsStr;

				for (auto &c : rooms)
				{
					string r = "(" + to_string(c.first) + ")" + c.second.name + ", ";
					roomsStr += r;
				}

				// remove ", " from the back of roomsStr
				roomsStr.pop_back();
				roomsStr.pop_back();

				// Format -> #SR:msg
				combine(str, ":", {SHOW_ROOMS, roomsStr.c_str()});

				cout << str << endl;

				send(client_socket, str, sizeof(str), 0);
			}
			/****** CREATE_ROOM = "#CR" ******/
			else if (string(str).rfind(CREATE_ROOM) == 0)
			{
				// Format ->  #create_room:room_name:room_cap
				vector<string> splits = split(str, ":");
				string command = splits[0], room_name = splits[1], room_cap = splits[2];

				// atoi(): convert string to int
				lock_guard<mutex> guard(rooms_mtx);
				rooms[roomId] = {room_name, atoi(room_cap.c_str()), id, {}};

				// Format -> #CR:msg:roomID
				combine(str, ":", {CREATE_ROOM, string(room_name + " has been created.").c_str(), to_string(roomId).c_str()});

				roomId++;

				cout << colors[NUM_COLORS - 1] << str << endl
					 << def_color;

				send(client_socket, str, sizeof(str), 0);
			}
			/****** ENTER_ROOM = "#ER"******/
			else if (string(str).rfind(ENTER_ROOM) == 0)
			{
				vector<string> splits = split(str, ":");
				string command = splits[0];
				int roomId = atoi(splits[1].c_str());

				if (rooms.count(roomId) == 0)
				{
					combine(msg, "\0", {"*", "The room you select do not exist, please try another room."});
					send(client_socket, msg, sizeof(msg), 0);
					continue;
				}
				else if (rooms[roomId].clients.size() == rooms[roomId].capacity)
				{
					combine(msg, "\0", {"*", "The room you select is full, please try another room."});
					send(client_socket, msg, sizeof(msg), 0);
					continue;
				}

				lock_guard<mutex> guard_rooms(rooms_mtx);
				rooms[roomId].clients.insert(id);

				lock_guard<mutex> guard_clients(clients_mtx);
				clients[id].select_room_id = roomId;

				// send to clients who has joined this room
				combine(msg, "\0", {"*", name, " has joined the room \'", rooms[roomId].name.c_str(), "\'"});

				send(client_socket, msg, sizeof(msg), 0);
				broadcast_message(msg, id, roomId);
				shared_print(msg);
			}
			continue;
		}

		// Handle messages of the room

		// Format -> name:colorId:roomId:msg
		combine(msg, ":", {name, to_string(id).c_str(), to_string(clients[id].select_room_id).c_str(), str});
		broadcast_message(msg, id, clients[id].select_room_id);
		continue;
	}
}
