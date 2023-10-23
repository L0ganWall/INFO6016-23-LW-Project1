To start: build the project in Visual Studio.
The library project should have first build order
Then you can either go into the debug folder and double click on the server.exe then client.exe (times the number of clients you want) or open a terminal, git bash or any other command line application and run the server first then the clients
Note: none of the inputs use ""s
The clients require a name to be entered first, then before you can send messages type "join the_room_name" to join a room, you can join multiple rooms.
To send a message in that room type "the_room_name: my_message_here and press enter"
To leave a room type "leave the_room_name"
To exit the client type "Exit"
To have the server exit type "shutdown" in it's window