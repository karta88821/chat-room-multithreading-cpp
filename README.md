# Chat Room Application

This side project create chat room application for clients are able to communicate each other.
This project has serveral features as follows:
- The client can see the list of rooms on the server.
- The client can create its own room, or it can enter the room which has been created.
- Clients who is joined the specific room can talk with each other.
- The client can leave the room, server will notify clients who is joined this room that the client leave. 
- The client can terminate the program directly.

## Library
- socket
- thread
- mutex
- signal

## How to run
Compile and link:
```
make
```

Clean output files:
```
make clean
```

Run the server
```
./server
```

Run the client
```
./client
```

## Commands of the client

| Command | Full Name  | Parameters     |
| ------- | ---------  | ----------     |
| #SR     | SHOW_ROOMS | None           |
| #CR     | CREATE_ROOM| name, capacity |
| #ER     | ENTER_ROOM | room_id        |
| #LR     | LEFT_ROOM  | None           |
| #EXIT   | EXIT       | None           |