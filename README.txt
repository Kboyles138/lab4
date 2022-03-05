KIRSTEN BOYLES

First run Server with the command
gcc Server.cpp -o Server -lstdc++ -pthread

Then start the server 
./Server <port num> <max burgers> <chefs>

Then you can run the client in another ubuntu window
gcc Client.cpp -o Client -lstdc++

And start the client
./Client <ip> <portnum> <burgers wanted>

So there are a couple issues with this code. The ideal situation for the server is to have as many clients connected as there are chefs. Once there are no burgers left in the server, it must have a client try and reconnect to detect that there are no more burgers for the server to start the shutdown sequence. 


