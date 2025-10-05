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
#include <limits>

using namespace std;

#define TCP_PORT "45849"  // the port client will be connecting to

#define LOCALHOST "127.0.0.1"
#define MAXDATASIZE 1024 // max number of bytes we can get at once

int client_tcp;
struct sockaddr_in mainServerAddr_TCP;

char toMain[MAXDATASIZE];
char fromMain[MAXDATASIZE];
string clientPort;

string username;
string password;

/**
 * Function to boot the client
 */
void bootAndLogin(){

    cout<<"[Client] Booting up."<<endl;
    cout<<"[Client] Logging in."<<endl;
    

}

/**
 * Function to send a message to the main server
 * @param message The message to be sent
 */
void sendServerData(string message){


    if (send(client_tcp, message.c_str(), message.length(), 0) == -1) {
        perror("Send TCP");
        exit(1);
    }
}

/**
 * Function to receive a message from the main server
 */
void receiveServerData(){

    memset(fromMain, 0, sizeof(fromMain));
    if (recv(client_tcp, fromMain, sizeof(fromMain), 0) == -1) {
        perror("Receive TCP");
        exit(1);
    }
}

/**
 * Function to check if the command entered by the user is valid
 * @param command The command to be checked
 * @return 1 if the command is valid, 0 otherwise
 */
int isCommandValid(string command) {

    //split command into individual elements and store in a vector
    vector<string> commandParts;
    stringstream ss(command);
    string part;
    while (ss >> part) {
        commandParts.push_back(part);
    }


    // Check if the command is valid
    string action = commandParts[0];
    if (action == "quote" || action == "buy" || action == "sell" || action == "position" || action == "exit") {
        if(action == "quote" && (commandParts.size() == 1 || commandParts.size() == 2)){
            return 1;
        }
        // check if action is buy and if the command has 3 parts and the third part is an integer
    

        else if((action == "buy"||action=="sell") && commandParts.size() >= 1){
            
            if(commandParts.size()<3){
                cout<<"[Client] Error: stock name/shares are required. Please specify a stock name to buy"<<endl;
                return 0;
            }

            else if(commandParts.size() > 3){
                return 0;
            }

            try{
                stoi(commandParts[2]);
            }
            catch(invalid_argument& e){
                return 0;
            }

            return 1;            
        }
        
        else if(action == "position" && commandParts.size() == 1){
            return 1;
        }
        else if(action == "exit" && commandParts.size() == 1){
            return 1;
        }        

    }
    return 0;
}


/**
 * Function to authorize the user
 */
void authorization() {
    while (true) {
        cout << "Please enter the username: ";
        cin >> username;

        // Used AI to understand how to clear the input buffer
        // Prompt used: "How to clear the input buffer in C++ after using cin?"
        // LLM used: GPT-4o
        cin.clear(); // Clear any error flags
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore leftover input

        cout << "Please enter the password: ";
        cin >> password;
        cin.clear(); // Clear any error flags
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore leftover input

        // Convert username to lowercase
        for (char &c : username) {
            c = tolower(c);
        }
        strcpy(toMain, username.c_str());
        strcat(toMain, " ");
        strcat(toMain, password.c_str());

        sendServerData(toMain);
        receiveServerData();

        if (strcmp(fromMain, "successful") == 0) {
            cout << "[Client] You have been granted access." << endl;
            break; // Exit the loop on successful login
        } else {
            cout << "[Client] The credentials are incorrect. Please try again." << endl;
        }
    }
}

/**
 * Function to process the quote request
 * @param command The command to be processed
 */
void processQuote(string command){

    sendServerData(command);
    cout<<"[Client] Sent a quote request to the main server."<<endl;

    receiveServerData();
    cout<<"[Client] Received the response from the main server using TCP over "<< clientPort <<"."<<endl<<endl;
    cout<<fromMain;

    cout<<"-----Start a new request-----"<<endl;
}

/**
 * Function to process the buy request
 * @param command The command to be processed
 */
void processBuy(string command){
    // Send buy request to the main server and receive the response
    sendServerData(command);
    
    receiveServerData();

    if(strcmp(fromMain, "unsuccessful") == 0){
        cout<<"[Client] Error: stock name does not exist. Please check again."<<endl;
        return;
    }
    else{
        // extract number of shares from last part of the command
        string numShares = command.substr(command.find(' ', command.find(' ')+1)+1);
        string stockName = string(fromMain).substr(0, string(fromMain).find(' '));
        string price = string(fromMain).substr(string(fromMain).find(' ') + 1, string(fromMain).find('\n') - string(fromMain).find(' ') - 1);
        cout<<"[Client] "<< stockName << "'s current price is "<< price << ". Proceed to buy?(Y/N)"<<endl;
        string response;
        cin>> response;

        // Used AI to understand how to clear the input buffer
        // Prompt used: "How to clear the input buffer in C++ after using cin?"
        // LLM used: GPT-4o
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore leftover input
        // send buy confirmation to the main server
        if(response == "Y" || response == "y"){
            
            sendServerData("confirm");
            receiveServerData();
            //Receive buy result from the main server
            cout<<"[Client] Received the response from the main server using TCP over port"<< clientPort<<"."<< endl;

            cout<<"\n"<<username<< " successfully bought "<< numShares << " shares of "<< stockName << "."<<endl;

            
        }
        else if(response == "N" || response == "n"){
            sendServerData("deny");
        }
        
        cout<<"\n-----Start a new request-----"<<endl;
    }
        
    
}
    
/**
 * Function to process the sell request
 * @param command The command to be processed
 */
void processSell(string command){
    // Process sell request
    string stockName = command.substr(command.find(' ')+1, command.find(' ', command.find(' ')+1) - command.find(' ')-1);
    string numShares = command.substr(command.find(' ', command.find(' ')+1)+1);
    sendServerData(command);
    
    receiveServerData();

    if(strcmp(fromMain, "unsuccessful") == 0){
        cout<<"[Client] Error: stock name does not exist. Please check again."<<endl;
        
    }

    else if(strcmp(fromMain, "notenough") == 0){
        cout<<"[Client] Error: "<< username << " does not have enough shares of "<< stockName << " to sell. Please try again"<<endl;
    }
    else{
        string price = string(fromMain);
        cout<<"[Client] "<< stockName << "'s current price is: "<< price << ". Proceed to sell?(Y/N)"<<endl;
        string response;
        cin>> response;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore leftover input
        if(response == "Y" || response == "y"){
            
            sendServerData("confirm");
            
        }
        else if(response == "N" || response == "n"){
            sendServerData("deny");
        }
        
        receiveServerData();

        if(strcmp(fromMain, "successful") == 0){
            cout<<"\n"<<username<< " successfully sold "<< numShares << " shares of "<< stockName << "."<<endl;
        }

    }
    
    cout<<"\n-----Start a new request-----"<<endl;

    
}

/**
 * Function to process the position request
 * @param command The command to be processed
 */
void processPosition(string command){
    // Process position request
    cout<<"[Client] "<< username << " sent a position request to the main server."<<endl;
    sendServerData(command);
    
    receiveServerData();
    cout<<"[Client] Received the response from the main server using TCP over "<< clientPort<< endl<<endl;
    cout<<"stock\tshares\tavg_buy_price"<<endl<<endl;
    vector<string> portfolioParts;
    stringstream ss(fromMain);
    string part;
    string result;
    while (ss >> part) {
        portfolioParts.push_back(part);
    }
    for(int i = 0; i < portfolioParts.size()-1; i += 3) {
        string stockName = portfolioParts[i];
        string numShares = portfolioParts[i + 1];
        string avgPrice = portfolioParts[i + 2];
        if(numShares == "0"){
            continue;
        }
        cout << stockName << "\t" << numShares << "\t" << avgPrice << endl<<endl;
    }
    cout<<username<< "'s current profit is: "<< portfolioParts[portfolioParts.size()-1]<<endl;
}

/**
 * Function to process commands entered by the user. This function will 
 * always be running until the user enters "exit". It will process the commands
 * and send the appropriate requests to the main server.
 */
void processCommands() {
    string command = "";
    while (command != "exit") {
        cout << "\n[Client] Please enter the command: " << endl << endl;
        cout << "<quote>" << endl << endl;
        cout << "<quote <stock name>>" << endl << endl;
        cout << "<buy <stock name> <number of shares>>" << endl << endl;
        cout << "<sell <stock name> <number of shares>>" << endl << endl;
        cout << "<position>" << endl << endl;
        cout << "<exit>" << endl << endl;

        getline(cin, command); // Read the full command
       

        if (command.empty()) {
            continue; // Skip empty input
        }

        string action = command.substr(0, command.find(' '));
        if (strcmp(action.c_str(), "quote") == 0 && isCommandValid(command) == 1) {
            processQuote(command);
        } else if (strcmp(action.c_str(), "buy") == 0 && isCommandValid(command) == 1) {
            processBuy(command);
        } else if (strcmp(action.c_str(), "sell") == 0 && isCommandValid(command) == 1) {
            processSell(command);
        } else if (strcmp(action.c_str(), "position") == 0 && isCommandValid(command) == 1) {
            processPosition(command);
        } else {
        }
    }
    sendServerData(command);
    exit(0);
    close(client_tcp);
}

/**
 * Reference: used code provided in project documentation
 * 
 * Function to get the port number of the client
 */
void getClientPort() {
    // Get the port number of the client
    struct sockaddr_in clientAddr;
    socklen_t addr_len = sizeof(clientAddr);
    if (getsockname(client_tcp, (struct sockaddr *)&clientAddr, &addr_len) == -1) {
        perror("getsockname");
        exit(1);
    }
    clientPort = to_string(ntohs(clientAddr.sin_port));
}

/**
 * Function to connect to the main server: create a TCP socket, connect to the server, and authorize the user
 */
void connectMain(){

    if((client_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket TCP");
        exit(1);
    }
    mainServerAddr_TCP.sin_family = AF_INET;
    mainServerAddr_TCP.sin_port = htons(atoi(TCP_PORT));
    mainServerAddr_TCP.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(mainServerAddr_TCP.sin_zero), '\0', 8);
    if(connect(client_tcp, (struct sockaddr *)&mainServerAddr_TCP, sizeof(mainServerAddr_TCP)) == -1){
        perror("Connect TCP");
        exit(1);
    }

    getClientPort();
    authorization();

    // Used AI to understand how to clear the input buffer
    // Prompt used: "How to clear the input buffer in C++ after using cin, but not doing anything if buffer is empty?"
    // LLM used: GPT-4o
    cin.sync();

    processCommands();
    close(client_tcp);


}


/**
 * Run the program in the command line by executing the following commands.
 * make all
 * ./client
 */
int main(){
    bootAndLogin();
    connectMain();
}


