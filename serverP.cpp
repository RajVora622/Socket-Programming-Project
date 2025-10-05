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
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define UDP_PORT_SERVERM "44849"	// the port other servers will be connecting to
#define UDP_PORT_SERVERP "42849"	// the port server A will be connecting to
#define LOCALHOST "127.0.0.1"
#define MAXDATASIZE 1024 // max number of bytes we can get at once

int sock_udp_serverP;

struct sockaddr_in addr_serverP;
struct sockaddr_in addr_serverM;
socklen_t addr_len_serverP, addr_len_serverM;
char fromServerM[MAXDATASIZE];
char toServerM[MAXDATASIZE];

struct stockInfo {
    int numShares;
    double avgPrice;
};

map<string, map<string, stockInfo>> portfolio;


/**
 * Read portfolio from a file which has the format eg:
 *  James
    S1 400 353.47
    S2 178 500.00
    Mary
    S1 188 477.98
    S2 100 436.56   
    Patricia
    S1 333 503.59
    S2 200 510.24
 */
void readPortfolios() {

    ifstream file("portfolios.txt");
    if (!file) {
        cerr << "Error opening portfolios file." << endl;
        return;
    }
    string line;
    string currentUser;
    while (getline(file, line)) {

        istringstream iss(line);
        vector<string> elements;
        string element;
        while (iss >> element) {
            elements.push_back(element);
        }
        if (elements.size() == 1) {
            currentUser = elements[0];
            for (char &c : currentUser) {
                c = tolower(c);
            }
            portfolio[currentUser] = map<string, stockInfo>();
        } 
        
        else {
            string stockName = elements[0];
            int numShares = stoi(elements[1]);
            double avgPrice = stod(elements[2]);
            portfolio[currentUser][stockName] = {numShares, avgPrice};
        }
          
        
        
    }

    file.close();
    
    
}

/**
 * Function to send data to the main server
 * @param message The message to be sent
 */
void sendServerData(string message){
    if (sendto(sock_udp_serverP, message.c_str(), message.length(), 0, (struct sockaddr *)&addr_serverM, sizeof(addr_serverM)) == -1) {
        perror("Send UDP to Server M");
        exit(1);
    }
}

/**
 * Function to receive a message from the main server
 */
void receiveServerData(){
    memset(fromServerM, 0, sizeof(fromServerM));
    addr_len_serverM = sizeof(struct sockaddr_in);
    if (recvfrom(sock_udp_serverP, fromServerM, sizeof(fromServerM), 0, (struct sockaddr *)&addr_serverM, &addr_len_serverM) == -1) {
        perror("Receive UDP from Server M");
        exit(1);
    }
}

/**
 * Function to process the boot phase of the server
 * This function will create a UDP socket and bind it to the specified port
 * and initialize the address structures for both servers
 */
void bootPhase(){
    cout<<"[Server P] Booting up using UDP on port "<<UDP_PORT_SERVERP<<"."<<endl;
    if((sock_udp_serverP = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket UDP");
        exit(1);
    }
    addr_serverP.sin_family = AF_INET;
    addr_serverP.sin_port = htons(atoi(UDP_PORT_SERVERP));
    addr_serverP.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverP.sin_zero), '\0', 8);

    if(bind(sock_udp_serverP, (struct sockaddr *)&addr_serverP, sizeof(addr_serverP)) == -1){
        perror("Bind UDP");
        exit(1);
    }
    

    addr_serverM.sin_family = AF_INET;
    addr_serverM.sin_port = htons(atoi(UDP_PORT_SERVERM));
    addr_serverM.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverM.sin_zero), '\0', 8);
    addr_len_serverM = sizeof(struct sockaddr_in);

    readPortfolios();

}

/**
 * Function to process the position request
 * @param username The username of the user
 */
void processPosition(string username) {
    cout << "[Server P] Received a position request from the main server for Member: " << username << endl;

    string response = "";
    for (const auto& stock : portfolio[username]) {
        response += "\n"+stock.first + " " + to_string(stock.second.numShares) + " " + to_string(stock.second.avgPrice);
    }
    strncpy(toServerM, response.c_str(), MAXDATASIZE);
    sendServerData(toServerM);

    cout<<"[Server P] Finished sending the gain and portfolio of "<< username << " to the main server."<<endl;
    
}
/**
 * Function to process the buy request
 * @param username The username of the user
 * @param stockName The name of the stock
 * @param numShares The number of shares to be bought
 * @param price The price of the stock
 */

void processBuy(string username, string stockName, int numShares, double price) {
    // Check if the user exists in the portfolio
    cout << "[Server P] Received a buy request from the client." << endl;

    // Check if the stock exists in the portfolio
    if (portfolio[username].find(stockName) != portfolio[username].end()) {
        portfolio[username][stockName].avgPrice = (portfolio[username][stockName].numShares*portfolio[username][stockName].avgPrice + numShares*price) / (portfolio[username][stockName].numShares + numShares);
        portfolio[username][stockName].numShares += numShares;
    } else {
        portfolio[username][stockName].numShares+= numShares;
        portfolio[username][stockName].avgPrice = price;
    }
    // Send server M confirmation about the buy
    cout << "[Server P] Successfully bought " << numShares << " shares of " << stockName << " and updated " << username << "'s portfolio." << endl;
    sendServerData("successful");
}

/**
 * Function to process the sell request
 * @param stockName The name of the stock
 * @param numShares The number of shares to be sold
 * @param username The username of the user
 */
void processSell(string stockName, int numShares, string username) {
    cout << "[Server P] Received a sell request from the main server." << endl;

    // check if enough shares are available and send server M data about it
    if (portfolio[username].find(stockName) != portfolio[username].end()) {
        if (portfolio[username][stockName].numShares >= numShares) {
            cout<<"[Server P] Stock "<< stockName << " has sufficient shares in "<< username << "'s portfolio. Requesting users' confirmation for selling stock."<<endl;
            sendServerData("enough");
        } else {
            cout<<"[Server P] Stock "<< stockName << " does not have enough shares in "<< username << "'s portfolio. Unable to sell "<< numShares << " shares of "<< stockName << "."<<endl;
            sendServerData("notenough");
            return;
        }
    } else {
        cout<<"[Server P] Stock "<< stockName << " does not have enough shares in "<< username << "'s portfolio. Unable to sell "<< numShares << " shares of "<< stockName << "."<<endl;
        sendServerData("notenough");
        return;
    }

    receiveServerData();

    if(strcmp(fromServerM, "confirm") == 0){
        cout<<"[Server P] User approves selling the stock."<<endl;
        portfolio[username][stockName].numShares -= numShares;
        cout << "[Server P] Successfully sold " << numShares << " shares of " << stockName << " and updated " << username << "'s portfolio." << endl;
        sendServerData("successful");
    }
    else{
        cout<<"[Server P] Sell denied."<<endl;
        sendServerData("unsuccessful");
    }
}

/**
 * Function to process commands entered by the user. 
 * This function will always be running and keep the server "Always on"
 * It will process the commands
 * and send the appropriate requests to the main server.
 */
void processCommands() {
    while(1){
        memset(fromServerM, 0, sizeof(fromServerM));
        receiveServerData();
        string command(fromServerM);
        vector<string> commandParts;
        stringstream ss(command);
        string part;
        while (ss >> part) {
            commandParts.push_back(part);
        }

        if (strcmp(commandParts[0].c_str(), "buy") == 0) {
            processBuy(commandParts[3],commandParts[1], stoi(commandParts[2]), stod(commandParts[4]));
        } 
        
        else if (strcmp(commandParts[0].c_str(), "sell") == 0) {
            processSell(commandParts[1], stoi(commandParts[2]), commandParts[3]);
        } 
        
        else if (strcmp(commandParts[0].c_str(), "exit") == 0) {
            cout << "[Server P] Client requested to exit." << endl;
            break;
        }
        else if (strcmp(commandParts[0].c_str(), "position") == 0) {
            processPosition(commandParts[1]);
        } 
        
        
        else {
            cout << "[Server P] Unknown command: " << command << endl;
            sendServerData("Unknown command");
        }
    }
}

/**
 * Main function
 * This function will call the boot phase and process commands
 * Run the server using the command: ./serverP
 */
int main(){
    bootPhase();
    processCommands();
    close(sock_udp_serverP);
}