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
#define UDP_PORT_SERVERQ "43849"	// the port server A will be connecting to
#define LOCALHOST "127.0.0.1"
#define MAXDATASIZE 1024 // max number of bytes we can get at once

int sock_udp_serverP;

struct sockaddr_in addr_serverQ;
struct sockaddr_in addr_serverM;
socklen_t addr_len_serverQ, addr_len_serverM;
char fromServer[MAXDATASIZE];
char toServer[MAXDATASIZE];

map<string, vector<double>> stockPrices;
map<string, int> timeIndex;
/**
 * Read stock prices from a file (quotes.txt) which has the format: into a map for stock prices
 * Additionally, the time index is also stored in a map for easy access and is initialized to 0
 * S1 697.46 697.05 700.13 699.89 700.08 703.1 701.6 702.92 704.2 698.23
    S2 464.61 465.92 464.52 466.19 465.19 465.23 463.57 464.47 462.77 464.36
 * 
 */

void readStockPrices() {
    ifstream file("quotes.txt");
    if (!file) {
        cerr << "Error opening stock prices file." << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string stockName;
        ss >> stockName;

        vector<double> prices;
        double price;
        while (ss >> price) {
            prices.push_back(price);
        }

        // Store the stock prices in the map
        stockPrices[stockName] = prices;

        // Initialize time index to 0 for each stock
        timeIndex[stockName] = 0;
    }

    file.close();
}

/**
 * Booting up the server and binding the socket to the UDP port
 * and initializing the address structures for both servers
 */

void bootPhase(){


    cout<<"[Server Q] Booting up using UDP on port "<<UDP_PORT_SERVERQ<<"."<<endl;
    if((sock_udp_serverP = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket UDP");
        exit(1);
    }

    addr_serverQ.sin_family = AF_INET;
    addr_serverQ.sin_port = htons(atoi(UDP_PORT_SERVERQ));
    addr_serverQ.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverQ.sin_zero), '\0', 8);

    if(bind(sock_udp_serverP, (struct sockaddr *)&addr_serverQ, sizeof(addr_serverQ)) == -1){
        perror("Bind UDP");
        exit(1);
    }
    

    addr_serverM.sin_family = AF_INET;
    addr_serverM.sin_port = htons(atoi(UDP_PORT_SERVERM));
    addr_serverM.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverM.sin_zero), '\0', 8);
    addr_len_serverM = sizeof(struct sockaddr_in);

    readStockPrices();

}

/**
 * Function to send a message to the main server
 * @param message The message to be sent
 */
void sendServerData(string message){
    addr_len_serverM = sizeof(struct sockaddr_in);

    if (sendto(sock_udp_serverP, message.c_str(), message.length(), 0, (struct sockaddr *)&addr_serverM, sizeof(addr_serverM)) == -1) {
        perror("Send UDP to Server M");
        exit(1);
    }
}

/**
 * Function to receive a message from the main server
 */
void receiveServerData(){
    memset(fromServer, 0, sizeof(fromServer));
    addr_len_serverM = sizeof(struct sockaddr_in);
    if (recvfrom(sock_udp_serverP, fromServer, sizeof(fromServer), 0, (struct sockaddr *)&addr_serverM, &addr_len_serverM) == -1) {
        perror("Receive UDP from Server.");
        exit(1);
    }
}

/**
 * Increment the time index for a specific stock
 * @param stockName The name of the stock
 */
void incrementTimeIndex(string stockName){

    timeIndex[stockName]++;
    cout<<"[Server Q] Received a time forward request for "<< stockName << ", the current price of that stock is "<< stockPrices[stockName][timeIndex[stockName]%10] << " at time "<< timeIndex[stockName] << endl;

}

/**
 * Packaging the current stock prices based on the time index
 * and sending it to the main server
 */
void sendAllQuotes(){
    cout<<"[Server Q] Received a quote request from the main server."<<endl;

    stringstream ss;
    for (const auto& stock : stockPrices) {
        string stockName = stock.first;
        double price = stock.second[(timeIndex[stockName]%10)];
        ss << stockName << " " << price << "\n\n";
    }
    string message = ss.str();
    sendServerData(message);
    cout<<"[Server Q] Returned all stock quotes."<<endl;
}

/**
 * Packaging the current stock prices of specific stock based on the time index
 * and sending it to the main server
 * @param stockName The name of the stock
 * @return 1 if the stock exists, 0 otherwise
 */
int sendQuote(string stockName){
    
    cout<<"[Server Q] Received a quote request for stock from the main server for stock "<< stockName << "."<<endl;     
    if (stockPrices.find(stockName) == stockPrices.end()) {
        string message = stockName + " does not exist. Please try again.\n\n";
        sendServerData(message);
        return 0;
    }

    stringstream ss;
    double price = stockPrices[stockName][timeIndex[stockName]%10];
    ss << stockName << " " << price << "\n\n";
    string message = ss.str();
    sendServerData(message);
    cout<<"[Server Q] Returned the stock quote of "<< stockName << "."<< endl;

    return 1;
}

/**
 * Process the buy request for a specific stock
 * @param stockName The name of the stock
 */
void processBuy(string stockName){
    cout<<"[Server Q] Received a quote request for stock from the main server for stock "<< stockName << "."<< endl;     
    if (stockPrices.find(stockName) == stockPrices.end()) {
        string message = stockName + " does not exist. Please try again.\n\n";
        sendServerData("unsuccessful");
        return;
    }

    
    
    stringstream ss;
    double price = stockPrices[stockName][timeIndex[stockName]%10];
    ss << stockName << " " << price << "\n\n";
    string message = ss.str();
    sendServerData(message);

    cout<<"[Server Q] Returned the stock quote of "<< stockName << "."<< endl;


}

/**
 * Process the sell request for a specific stock
 * @param stockName The name of the stock
 */
void processSell(string stockName){
    cout<<"[Server Q] Received a quote request for stock from the main server for stock "<< stockName <<"."<< endl;     
    if (stockPrices.find(stockName) == stockPrices.end()) {
        string message = stockName + " does not exist. Please try again.\n\n";
        sendServerData("unsuccessful");
        return;
    }


    stringstream ss;
    double price = stockPrices[stockName][timeIndex[stockName]%10];
    ss << stockName << " " << price << "\n\n";
    string message = ss.str();
    sendServerData(message);
    cout<<"[Server Q] Returned the stock quote of "<< stockName << "."<< endl;

    
    
}

/**
 * Process the commands entered by the user
 * This function will always be running to keep the server "Always on"
 * It will process the commands and send the appropriate requests to the main server
 */
void processCommands(){

    while(1){

        memset(fromServer, 0, sizeof(fromServer));
        receiveServerData();
        string command(fromServer);
        vector<string> commandParts;
        stringstream ss(command);
        string part;
        while (ss >> part) {
            commandParts.push_back(part);
        }

        if (strcmp(commandParts[0].c_str(), "quote") == 0) {
        
            if(commandParts.size() == 1){
                sendAllQuotes();

            }
            else{
                sendQuote(commandParts[1]);
            }
        }

        else if (strcmp(commandParts[0].c_str(), "buy") == 0) {
            processBuy(commandParts[1]);
        }

        else if (strcmp(commandParts[0].c_str(), "sell") == 0) {
            processSell(commandParts[1]);
        }

        else if (strcmp(commandParts[0].c_str(), "timeshift") == 0) {
            incrementTimeIndex(commandParts[1]);
        }

        else if (strcmp(commandParts[0].c_str(), "exit") == 0) {
            cout << "[Server Q] Exiting." << endl;
            break;
        }
        else {
            cout << "[Server Q] Unknown command: " << command << endl;
            sendServerData("Unknown command");
        }


    }
}

/**
 * Main function to start the server
 * Run the program by executing the command: ./serverQ
 */
int main(){
    bootPhase();
    processCommands();
    close(sock_udp_serverP);
}