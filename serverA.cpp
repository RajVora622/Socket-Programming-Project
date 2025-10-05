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

#define UDP_PORT_SERVERM "44849"
#define UDP_PORT_SERVERA "41849"
#define LOCALHOST "127.0.0.1"
#define MAXDATASIZE 1024 // max number of bytes we can get at once

int sock_udp_serverP;

struct sockaddr_in addr_serverA;
struct sockaddr_in addr_serverM;
socklen_t addr_len_serverA, addr_len_serverM;
char fromServerM[MAXDATASIZE];
char toServerM[MAXDATASIZE];

map <string, string> members;

/** 
 * Read members from a file which has the format "Username EncryptedPassword" Eg:
 *  James abcde 
 *  Mary  d32daa
 *  Patricia asdad
 */
void readMembers(){
    ifstream file("members.txt");
    if (!file) {
        //cerr << "Error opening members file." << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string username, password;
        ss >> username >> password;

        //convert username to lowercase
        for (char &c : username) {
            c = tolower(c);
        }
        members[username] = password;
    }

    file.close();
    

}

/**
 * Booting up the server and binding the socket to the UDP port
 * and initializing the address structures for both servers
 */
void bootPhase(){
    cout<<"[Server A] Booting up using UDP on port "<<UDP_PORT_SERVERA<<"."<<endl;
    readMembers();

    if((sock_udp_serverP = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("Socket UDP");
        exit(1);
    }

    addr_serverA.sin_family = AF_INET;
    addr_serverA.sin_port = htons(atoi(UDP_PORT_SERVERA));
    addr_serverA.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverA.sin_zero), '\0', 8);

    if(bind(sock_udp_serverP, (struct sockaddr *)&addr_serverA, sizeof(addr_serverA)) == -1){
        perror("Bind UDP");
        exit(1);
    }
    
    addr_serverM.sin_family = AF_INET;
    addr_serverM.sin_port = htons(atoi(UDP_PORT_SERVERM));
    addr_serverM.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(addr_serverM.sin_zero), '\0', 8);

   
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
void receiveServerData(struct sockaddr_in addr_server){

    memset(fromServerM, 0, sizeof(fromServerM));
    addr_len_serverM = sizeof(struct sockaddr_in);
    if (recvfrom(sock_udp_serverP, fromServerM, sizeof(fromServerM), 0, (struct sockaddr *)&addr_serverM, &addr_len_serverM) == -1) {
        perror("Receive UDP from Server.");
        exit(1);
    }
}

/**
 * Function to authorize the user

 */
void authorizeUser() {

    

    while(1){
        
        receiveServerData(addr_serverM);
        string username = string(fromServerM).substr(0, string(fromServerM).find(' '));
        string encryptedPassword = string(fromServerM).substr(string(fromServerM).find(' ') + 1);
        cout << "[Server A] Received username "<< username << " and password ******." << endl;

        // Check if the user exists in the members map
        if (members.find(username) != members.end()) {
            if(members[username] == encryptedPassword){
                cout << "[Server A] Member " << username << " has been authenticated." << endl;
                // Send authorization response
                string response = "successful";
                sendServerData(response);
            }
            else{
                cout << "[Server A] The username "<< username << " or password ****** is incorrect." << endl;
                // Send authorization response
                string response = "unsuccessful";
                sendServerData(response);
            }
        }
        else{
            cout << "[Server A] The username "<< username << " or password ****** is incorrect." << endl;
            // Send authorization response
            string response = "unsuccessful";
            sendServerData(response);
        }
        
        
    }
}

/**
 * Main function to start the server
 * Run the server by executing the command: ./serverA
 */
int main() {
    bootPhase();
    authorizeUser();
    return 0;
}
    

    