Name: RAJ VORA
Student ID: 9094527849

Project Environment: Ubuntu 22.04 ARM64

Assignment details:

Code Files:

serverM.cpp:

This handles all the requests from the various clients and servers. This is the only server the client
interfaces with.

serverA.cpp:

This handles the authentication of clients logging in. It takes the encrypted password and 
checks it's database to check if the password matches. This only interfaces with ServerM.

serverP.cpp:

This stores the portfolio of all the users, and makes changes based on activity by the user (buying/selling)
It also validates that the user has enough shares to sell. This only interfaces with ServerM.

serverQ.cpp

This stores the quotes of the various stocks available to trade. It will give the current prices of any stock
and update the current price based on trading activity. This only interfaces with ServerM.
client.cpp

client.cpp

This handles all the I/O for the end user, and allows the user to see the current price of stocks, trade(Buy/sell),
view it's current portfolio.
This interfaces only with Server M.

Format of messages that are being sent:
space delimited strings

Reused/Referenced code from:

Beej Networking guide references used:

https://www.beej.us/guide/bgnet/html/index-wide.html#socket : creating a socket
https://www.beej.us/guide/bgnet/html/index-wide.html#bind : binding a socket
https://www.beej.us/guide/bgnet/html/index-wide.html#connect : connecting to a socket
https://www.beej.us/guide/bgnet/html/index-wide.html#listen :listen for datagrams
https://www.beej.us/guide/bgnet/html/index-wide.html#acceptthank-you-for-calling-port-3490. : accepting a connection
https://www.beej.us/guide/bgnet/html/index-wide.html#sendrecv : sending and receiving for TCP
https://www.beej.us/guide/bgnet/html/index-wide.html#sendtorecv : sending and receiving for UDP 
https://www.beej.us/guide/bgnet/html/index-wide.html#a-simple-stream-server : used logic for fork()
https://www.beej.us/guide/bgnet/html/index-wide.html#a-simple-stream-client : miscelleneous 
https://www.beej.us/guide/bgnet/html/index-wide.html#datagram : miscelleneous


Ubuntu Version: Ubuntu 22.04 ARM64