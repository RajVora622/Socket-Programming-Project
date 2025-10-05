#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

#define TCP_PORT "45849"  // the port client will be connecting to
#define UDP_PORT_SERVERM "44849"	// the port other servers will be connecting to
#define UDP_PORT_SERVERA "41849"	// the port server A will be connecting to
#define UDP_PORT_SERVERP "42849"	// the port server P will be connecting to  
#define UDP_PORT_SERVERQ "43849"	// the port server Q will be connecting to
#define LOCALHOST "127.0.0.1"
#define MAXDATASIZE 1024 // max number of bytes we can get at once

int sock_tcp, sock_udp;
int sock_tcp_child;

struct sockaddr_in addr_TCP, addr_UPD;
struct sockaddr_in addr_client, addr_serverA, addr_serverP, addr_serverQ;
socklen_t addr_len_client, addr_len_serverA, addr_len_serverP, addr_len_serverQ;

char fromClient[MAXDATASIZE];
char toServer[MAXDATASIZE];
char toClient[MAXDATASIZE];
char fromServer[MAXDATASIZE];

/**
 * Function to boot the server
 * This function creates a TCP socket and binds it to the specified port
 * It also creates a UDP socket and binds it to the specified port
 * It initializes the address structures for both servers
 * and sets the server to listen for incoming connections
 * 
 */
void bootPhase(){

    cout<<"[Server M] Booting up using UDP on port "<<UDP_PORT_SERVERM<<"."<<endl;

    // 
    if((sock_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket TCP");
        exit(1);
    }

    /**
     * From Beej's Guide to Network Programming
     * https://beej.us/guide/bgpd/
     * 
     * 
     */
    addr_TCP.sin_family = AF_INET;
    addr_TCP.sin_port = htons(atoi(TCP_PORT));
    addr_TCP.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_TCP.sin_zero), '\0', 8);

    if(bind(sock_tcp, (struct sockaddr *)&addr_TCP, sizeof(addr_TCP)) == -1){
        perror("Bind TCP");
        exit(1);
    }

    if(listen(sock_tcp, 10) == -1){
        perror("Listen TCP");
        exit(1);
    }



    if((sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket UDP");
        exit(1);
    }

    addr_UPD.sin_family = AF_INET;
    addr_UPD.sin_port = htons(atoi(UDP_PORT_SERVERM));
    addr_UPD.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_UPD.sin_zero), '\0', 8);

    
    // From Beej's Guide to Network Programming
    //https://www.beej.us/guide/bgnet/html/index-wide.html#bind

    if(bind(sock_udp, (struct sockaddr *)&addr_UPD, sizeof(addr_UPD)) == -1){
        perror("Bind UDP");
        exit(1);
    }
    

    addr_serverP.sin_family = AF_INET;
    addr_serverP.sin_port = htons(atoi(UDP_PORT_SERVERP));
    addr_serverP.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverP.sin_zero), '\0', 8);

    addr_serverA.sin_family = AF_INET;
    addr_serverA.sin_port = htons(atoi(UDP_PORT_SERVERA));
    addr_serverA.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverA.sin_zero), '\0', 8);

    addr_serverQ.sin_family = AF_INET;
    addr_serverQ.sin_port = htons(atoi(UDP_PORT_SERVERQ));
    addr_serverQ.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverQ.sin_zero), '\0', 8);
}


/**
 * Function to process a sell request
 * 
 */
void sellRequest(string username, string stockName, int numShares){
    // Check if the user exists in the portfolio
    cout << "[Server M] Processing sell request for user: " << username << endl;
    cout << "[Server M] Selling " << numShares << " shares of " << stockName << " for user " << username << "." << endl;

    addr_len_serverP = sizeof(struct sockaddr_in);
    string processedData = string(username) + " sell " + stockName + " " + to_string(numShares);
    strncpy(toServer, processedData.c_str(), MAXDATASIZE);
    cout << "[Server M] Sending sell request to Server P." << endl;

    if (sendto(sock_udp, toServer, sizeof(toServer), 0, (struct sockaddr *)&addr_serverP, addr_len_serverP) == -1) {
        perror("Send UDP to Server P");
        exit(1);
    }

    cout << "[Server M] Sent sell request to Server P." << endl;
    if (recvfrom(sock_udp, fromServer, sizeof(fromServer), 0, (struct sockaddr *)&addr_serverP, &addr_len_serverP) == -1) {
        perror("Receive UDP from Server P");
        exit(1);
    }
    cout << "[Server M] Received response from Server P: " << fromServer << endl;
    strcpy(toClient, fromServer);
    cout << "[Server M] Sending sell confirmation to client." << endl;
    if (send(sock_tcp_child, toClient, strlen(toClient), 0) == -1) {
        perror("Send TCP");
        close(sock_tcp_child);
        exit(1);
    }

    if (recv(sock_tcp_child, fromClient, sizeof(fromClient), 0) == -1) {
        perror("Receive TCP");
        close(sock_tcp_child);
        exit(1);
    }
    cout << "[Server M] Received confirmation from client: " << fromClient << endl;

    //send confirmation to server P
    addr_len_serverP = sizeof(struct sockaddr_in);
    string confirmation = string(fromClient);
    strncpy(toServer, confirmation.c_str(), MAXDATASIZE);
    cout << "[Server M] Sending confirmation to Server P." << endl;
    if (sendto(sock_udp, toServer, sizeof(toServer), 0, (struct sockaddr *)&addr_serverP, addr_len_serverP) == -1) {
        perror("Send UDP to Server P");
        exit(1);
    }
    cout << "[Server M] Sent confirmation to Server P." << endl;
    cout << "[Server M] Sent sell confirmation to client: " << toClient << endl;
    cout << "[Server M] Finished processing sell request." << endl;
    
}




/**
 * Encrypts the string using the the following scheme:
 * Offset each character and/or digit by 3
 *  character: cyclic alphabetic (A-Z, a-z) update for overflow
 *  digit: cyclic numeric (0-9) update for overflow
 * The scheme is case sensitive, so 'A' and 'a' are treated differently.
 * Special characters are not encrypted.
 * @param data The string to be encrypted
 * @return The encrypted string
 */
string encrypt(string data) {
    string encryptedData = "";
    for (char c : data) {
        if (isalpha(c)) {
            if (isupper(c)) {
                c = (c - 'A' + 3) % 26 + 'A';
            } else {
                c = (c - 'a' + 3) % 26 + 'a';
            }
        } else if (isdigit(c)) {
            c = (c - '0' + 3) % 10 + '0';
        }
        encryptedData += c;
    }
    return encryptedData;
    
}


/**
 * Function to send data to the client
 * @param message The message to be sent
 */
void sendClientData(string message) {
    // Simulate sending data to the client
    if (send(sock_tcp_child, message.c_str(), message.length(), 0) == -1) {
        perror("Send TCP");
        close(sock_tcp_child);
        exit(1);
    }
}

/**
 * Function to receive data from the client
 * Based on Beej's Guide to Network Programming
 * https://www.beej.us/guide/bgnet/html/index-wide.html#sendrecv
 */
void receiveClientData() {

    memset(fromClient, 0, sizeof(fromClient));

    if (recv(sock_tcp_child, fromClient, sizeof(fromClient), 0) == -1) {
        perror("Receive TCP");
        close(sock_tcp_child);
        exit(1);
    }
}

/**
 * Based on Beej's Guide to Network Programming
 * https://www.beej.us/guide/bgnet/html/index-wide.html#sendrecv
 * 
 * Function to send data to Server A, Server P, or Server Q
 * @param message The message to be sent
 * @param addr_server The address of the server
 */
void sendServerData(string message, struct sockaddr_in addr_server){

    if (sendto(sock_udp, message.c_str(), message.length(), 0, (struct sockaddr *)&addr_server, sizeof(addr_server)) == -1) {
        perror("Send UDP to Server A");
        exit(1);
    }
    
}

/**
 * Based on Beej's Guide to Network Programming
 * https://www.beej.us/guide/bgnet/html/index-wide.html#sendtorecv
 * Function to receive data from Server A, Server P, or Server Q
 * @param addr_server The address of the server
 */
void receiveServerData(struct sockaddr_in addr_server){

    memset(fromServer, 0, sizeof(fromServer));
    if (recvfrom(sock_udp, fromServer, sizeof(fromServer), 0, (struct sockaddr *)&addr_server, &addr_len_serverA) == -1) {
        perror("Receive UDP from Server.");
        exit(1);
    }
}


/**
 * Function to authorize the user
 * @param username The username of the user
 * @param password The password of the user
 * @return 1 if the user is authorized, 0 otherwise
 */
int authorizeUser(string username, string password) {
    addr_len_serverA = sizeof(struct sockaddr_in);
    
    //Creating message to send to server A
    string encryptedPassword = encrypt(password);
    string message = username + " " + encryptedPassword;
    strncpy(toServer, message.c_str(), MAXDATASIZE);
    sendServerData(message, addr_serverA);
    cout << "[Server M] Sent the authentication request to Server A" << endl;

    receiveServerData(addr_serverA);
    
    cout << "[Server M] Received the response from Server A using UDP over "<< UDP_PORT_SERVERM<< endl;

    sendClientData(fromServer);
    cout << "[Server M] Sent the response from server A to the client using TCP over port "<< TCP_PORT << endl;

    // Check if the user is authorized
    if (strcmp(fromServer, "successful") == 0) {
        return 1;
    }
    
    return 0;
    
}


/**
 * Function to process the quote request
 * @param username The username of the user
 * @param command The command to be processed
 */
void processQuote(string username, string command){
    
    vector<string> commandParts;
    stringstream ss(command);
    string part;
    while (ss >> part) {
        commandParts.push_back(part);
    }

    if(commandParts.size() == 1){
        cout << "[Server M] Received a quote request from "<< username << ", using TCP over port "<< TCP_PORT << endl; 
        sendServerData(command, addr_serverQ);
        cout<<"[Server M] Forwarded the quote request to server Q."<<endl;
        receiveServerData(addr_serverQ);
        cout<<"[Server M] Received the quote response from server Q using UDP over "<< UDP_PORT_SERVERM<< endl;


    }

    else{
        cout << "[Server M] Received a quote request from "<< username << " for stock " << commandParts[1] << ", using TCP over port "<< TCP_PORT << "." << endl;
        sendServerData(command, addr_serverQ);
        cout<<"[Server M] Forwarded the quote request to server Q."<<endl;
        receiveServerData(addr_serverQ); 
        cout<<"[Server M] Received the quote response from server Q for stock: " << commandParts[1] << " using UDP over "<< UDP_PORT_SERVERM<< endl;
    }
    
    // Send the quote response to the client
    sendClientData(fromServer);
    cout<<"[Server M] Forwarded the quote response to the client."<< endl;

}

/**
 * Function to process the buy request
 * @param username The username of the user
 * @param command The command to be processed
 */
void processBuy(string username, string command){
    cout << "[Server M] Received a buy request from member "<< username << " using TCP over port "<< TCP_PORT << "." << endl;
    
    // Send Server Q a quote request and receive the response
    sendServerData(command, addr_serverQ);
    cout<<"[Server M] Sent the quote request to Server Q."<<endl;

    receiveServerData(addr_serverQ);
    // if the stock name does not exist, send the error message to the client and return
    if(strcmp(fromServer, "unsuccessful") == 0){
        sendClientData(fromServer);
        return;
    }
    else{

        // Send quote response to client and receive confirmation
        string quote = string(fromServer).substr(string(fromServer).find(' ')+1, string(fromServer).find('\n')-string(fromServer).find(' ')-1);
        string stockName = string(fromServer).substr(0, string(fromServer).find(' '));
        sendClientData(fromServer);
        cout<<"[Server M] Sent the buy confirmation to the client."<<endl;
        receiveClientData();
        
        if(strcmp(fromClient, "confirm") == 0){
            
            // Send the buy request to server P and receive the buy result
            cout<<"[Server M] Buy approved"<<endl;
            sendServerData(command+" "+username+" "+quote, addr_serverP);
            cout<<"[Server M] Forwarded the buy confirmation response to server P."<<endl;
            receiveServerData(addr_serverP);

            //Send buy result to client
            sendClientData(fromServer);
            cout<<"[Server M] Forwarded the buy result to the client."<<endl;
            //cout<<"[Server M] Received the buy confirmation from server P using UDP over "<< UDP_PORT_SERVERM<< endl;
        }
        else{
            cout<<"[Server M] Buy denied"<<endl;
        }

        cout<<"[Server M] Sent a time forward request for "<< stockName << "."<<endl;
        sendServerData("timeshift "+stockName, addr_serverQ);


    }       
}

/**
 * Function to process the sell request
 * @param username The username of the user
 * @param command The command to be processed
 */
void processSell(string username, string command){
    cout << "[Server M] Received a sell request from member "<< username << " using TCP over port "<< TCP_PORT << endl;
    
    sendServerData(command, addr_serverQ);
    cout<<"[Server M] Sent the quote request to Server Q."<<endl;

    receiveServerData(addr_serverQ);

    if(strcmp(fromServer, "unsuccessful") == 0){
        sendClientData(fromServer);
    }
    else{
        string quote = string(fromServer).substr(string(fromServer).find(' ')+1, string(fromServer).find('\n')-string(fromServer).find(' ')-1);
        string stockName = string(fromServer).substr(0, string(fromServer).find(' '));

        //checking if enough shares are available in portfolio
        sendServerData(command+" "+username, addr_serverP);

        cout<<"[Server M] Forwarded the sell request to server P."<<endl;
        receiveServerData(addr_serverP);

        if(strcmp(fromServer, "notenough") == 0){
            sendClientData(fromServer);
            return;
        }
        else if(strcmp(fromServer, "enough") == 0){
            cout<<"[Server M] Forwarded the sell confirmation to the client."<<endl;
            sendClientData(quote.c_str());
        }
        
        receiveClientData();
        
        sendServerData(fromClient, addr_serverP);

        cout<<"[Server M] Forwarded the sell confirmation response to server P."<<endl;
        receiveServerData(addr_serverP);
        sendClientData(fromServer);
        cout<<"[Server M] Forwarded the sell result to the client."<<endl;

        //Time shift requuest
        sendServerData("timeshift "+stockName, addr_serverQ);
        cout<<"[Server M] Sent a time forward request for "<< stockName << "."<<endl;

       
    }       
}

/**
 * Function to process the position request
 * @param username The username of the user
 * @param command The command to be processed
 */
void processPosition(string username, string command){
    cout << "[Server M] Received a position request from Member to check "<< username << "'s gain using TCP over port "<< TCP_PORT << endl;
    
    sendServerData(command+" "+username, addr_serverP);
    cout<<"[Server M] Forwarded the position request to server P."<<endl;

    receiveServerData(addr_serverP);
    cout<<"[Server M] Received user's portfolio from server P using UDP over "<< UDP_PORT_SERVERM<< endl;

    string portfolio = string(fromServer);
    
    vector<string> portfolioParts;
    stringstream ss(portfolio);
    string part;
    string result;
    while (ss >> part) {
        portfolioParts.push_back(part);
    }
    // print portfolio
    double profit = 0;
    for (size_t i = 0; i < portfolioParts.size(); i++) {
        if(i % 3 == 0){
            sendServerData("quote "+portfolioParts[i], addr_serverQ);
            receiveServerData(addr_serverQ);
            string quote = string(fromServer).substr(string(fromServer).find(' ')+1, string(fromServer).find('\n')-string(fromServer).find(' ')-1);
            double price = stod(quote);
            int shares = stoi(portfolioParts[i+1]);
            double avgPrice = stod(portfolioParts[i+2]);
            profit += (price - avgPrice) * shares;
        }
    }

    
    sendClientData(portfolio+" " + to_string(profit));
    cout<<"[Server M] Forwarded the gain to the client."<<endl;
}

/**
 * Function to process commands entered by the user and received by the client
 * This function keeps the server "Always on"
 * 
 * @param username The username of the user
 */
void processCommands(string username){
    while(true){
        memset(fromClient, 0, sizeof(fromClient));
        memset(toClient, 0, sizeof(toClient));
        
        receiveClientData();
        
        // Simulate sending a response back to the client

        //extract first word from fromClient
        //cout<<"Command: "<< fromClient << endl;
        string command(fromClient);
        string action = command.substr(0, command.find(' '));

        
        if(strcmp(action.c_str(), "exit") == 0){
            
            break;
            //exit(0);
        }

        else if(strcmp(action.c_str(), "quote") == 0){
            processQuote(username, command);
        }
        
        else if(strcmp(action.c_str(), "buy") == 0){
            processBuy(username, command);
        }
        
        else if(strcmp(action.c_str(), "sell") == 0){
            processSell(username, command);
        }
        else if(strcmp(action.c_str(), "position") == 0){
            processPosition(username, command);
        }
        
        

        
       
    }
}

/**
 * Function to start the client session
 * This function keeps accepting connections in a loop
 * and forks a new process to handle each client
 */
void startClientSession() {
    while (true) { // Keep accepting connections in a loop
        addr_len_client = sizeof(struct sockaddr_in);

        // Accept a new connection
        // From Beej's Guide to Network Programming
        // https://www.beej.us/guide/bgnet/html/index-wide.html#acceptthank-you-for-calling-port-3490.

        if ((sock_tcp_child = accept(sock_tcp, (struct sockaddr *)&addr_client, &addr_len_client)) == -1) {
            perror("Accept TCP");
            continue; // Continue to accept new connections even if one fails
        }


        // Fork a new process to handle the client
        // Based on Beej's Guide to Network Programming
        // https://www.beej.us/guide/bgnet/html/index-wide.html#a-simple-stream-server

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            close(sock_tcp_child);
            continue;
        }

        if (pid == 0) { // Child process
            close(sock_tcp); // Child doesn't need the listening socket

            //Client Authorization
            int clientAuthorized = 0;
            string username = "";
            while(clientAuthorized== 0){
                
                receiveClientData();
                username = string(fromClient).substr(0, string(fromClient).find(' '));
                string password = string(fromClient).substr(string(fromClient).find(' ') + 1);
                cout << "[Server M] Received username "<< username << " and password ****." << endl;
                
                clientAuthorized= authorizeUser(username, password);
                }

            
            processCommands(username);
            
            // Close the connection and exit the child process
            close(sock_tcp_child);
            exit(0); // Exit the child process
        } else { // Parent process
            close(sock_tcp_child); // Parent doesn't need the connected socket
        }
    }
}




    

/**
 * Signal handler for SIGCHLD
 * This function reaps all dead child processes
 */
void sigchld_handler(int s) {
    (void)s; // Suppress unused variable warning
    while (waitpid(-1, NULL, WNOHANG) > 0); // Reap all dead child processes
}

/**
 * Main function
 * This function sets up the signal handler and starts the client session
 * Run the program in the command line by executing the following commands.
 * make all
 * ./serverM
 */
int main() {

    // reaping zombie processes
    // Based on Beej's Guide to Network Programming
    // https://beej.us/guide/bgnet/source/examples/server.c
    
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    bootPhase();
    startClientSession();
}
