# Socket-Programming-Project
Socket Programming Project

# Assignment README

**Name:** Raj Vora  
**Student ID:** 9094527849  

**Project Environment:** Ubuntu 22.04 ARM64  

---

## Assignment Details

### Code Files

#### `serverM.cpp`
- Handles all the requests from clients and servers.  
- Main server the client interfaces with.  

#### `serverA.cpp`
- Handles authentication of clients logging in.  
- Takes encrypted password and checks against database.  
- Interfaces **only** with `serverM`.  

#### `serverP.cpp`
- Stores the portfolio of all users.  
- Updates portfolio based on buy/sell activity.  
- Validates if the user has enough shares to sell.  
- Interfaces **only** with `serverM`.  

#### `serverQ.cpp`
- Stores quotes of available stocks for trading.  
- Provides current prices and updates them based on trading activity.  
- Interfaces **only** with `serverM`.  

#### `client.cpp`
- Handles all I/O for the end user.  
- Allows the user to view stock prices, trade (buy/sell), and view their portfolio.  
- Interfaces **only** with `serverM`.  

---

## Message Format
- Messages are **space-delimited strings**.  

---

## References

Reused / Referenced code from [Beej's Networking Guide](https://www.beej.us/guide/bgnet/html/index-wide.html):

- [Creating a socket](https://www.beej.us/guide/bgnet/html/index-wide.html#socket)  
- [Binding a socket](https://www.beej.us/guide/bgnet/html/index-wide.html#bind)  
- [Connecting to a socket](https://www.beej.us/guide/bgnet/html/index-wide.html#connect)  
- [Listen for datagrams](https://www.beej.us/guide/bgnet/html/index-wide.html#listen)  
- [Accepting a connection](https://www.beej.us/guide/bgnet/html/index-wide.html#acceptthank-you-for-calling-port-3490.)  
- [Sending and receiving (TCP)](https://www.beej.us/guide/bgnet/html/index-wide.html#sendrecv)  
- [Sending and receiving (UDP)](https://www.beej.us/guide/bgnet/html/index-wide.html#sendtorecv)  
- [Fork logic](https://www.beej.us/guide/bgnet/html/index-wide.html#a-simple-stream-server)  
- [Stream client example](https://www.beej.us/guide/bgnet/html/index-wide.html#a-simple-stream-client)  
- [Datagrams](https://www.beej.us/guide/bgnet/html/index-wide.html#datagram)  

---

## System Information
- **Ubuntu Version:** Ubuntu 22.04 ARM64
