/*
Author: Madhura Bhat
Class: ECE6122 (A)
Last Date Modified: 11/25/2023
Description: 
This file has functions for client, Used two background threads each one for user input and receiving packet from server


*/

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Network.hpp>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <atomic>
#include <chrono>

// tcpMessage packet defination
struct tcpMessage {
    unsigned char nVersion;
    unsigned char nType;
    unsigned short nMsgLen;
    char chMsg[1000];
};

// Global Variables used across background threads running
sf::TcpSocket socket;
unsigned int updatedVersion; 
bool       exitProgram = false; // Flag used to exit the program
tcpMessage latestMsg; 

// atomic variable used to synchronization and exits the terminal
std::atomic<bool> exitFlag(false);
std::atomic<bool> newMsgReceived(false);

bool isNaturalNumber(const std::string& str) 
{
    if (str.empty()) 
    {
        return false;
    }
    
    for (char ch : str) 
    {
        
        if (!std::isdigit(ch)) {
            return false;
        }
    }
    // Convert the string to an integer and check if it's non-negative
    int num = std::stoi(str);
    return num >= 0;
}

bool split(const std::string& str, char delimiter, int maxSplits, std::vector<std::string>& result) {
    int iteration = 0;

    size_t start = 0;
    size_t end = 0;
    while (iteration < maxSplits && (end = str.find(delimiter, start)) != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        iteration++;
    }

    // Add the last part of the string
    result.push_back(str.substr(start));

    return (iteration > 0);
}

void* runUserInput(void* port)
{
    // Dummy Logic
    unsigned short serverPort = *reinterpret_cast<unsigned short*>(port);
    if(serverPort == 0)
    {
        serverPort = *reinterpret_cast<unsigned short*>(port);
    }

    UserInput:
    tcpMessage txMsg;
    std::cout << "Please enter command: ";
    std::string ipString;
    std::vector<std::string> result; 

    // Use getline to read a line from cin into the string
    std::getline(std::cin, ipString);
    split(ipString,' ',2,result);
    if(result[0] == "t")
    {
        // Validating the user input data for command 't'
        if((isNaturalNumber(result[1])) && (result.size() == 3))
        {
            txMsg.nVersion = static_cast<unsigned char>(updatedVersion);
            txMsg.nType    = static_cast<unsigned char>(std::stoi(result[1]));
            result[2].copy(txMsg.chMsg, result[2].size());
            txMsg.chMsg[result[2].size()] = '\0';
            txMsg.nMsgLen  = static_cast<unsigned short>(result[2].size()); 
            void* buffer =  &txMsg;
            // Sending the TCP message to server
            if (socket.send(buffer, sizeof(txMsg)) != sf::Socket::Done)
                return nullptr;
        }
        else 
        {
            std::cout << "Please enter valid input for command t " << std::endl;
        }
    }
    else if(result[0] == "v")
    {
        if((isNaturalNumber(result[1])) && (result.size() == 2))
        {
            // Updating the version variable which is used for future messages
            updatedVersion = static_cast<unsigned int>(std::stoi(result[1]));
        }
        else 
        {
            std::cout << "Please enter valid input for command v " << std::endl;
        }
    }
    else if((result[0] != "q") && (result[0] != ""))
    {
        std::cout << "Please enter valid command " << std::endl;
    }

    // if command is not 'q' we need to rerun the command for user inputs
    if(result[0] != "q") 
        goto UserInput;

    exitProgram = true;

    // Disconnecting the client from the server, which results is removal of this client from socketselector for which server listens the data
    socket.disconnect();

    // Flaggin the exiting logic
    exitFlag.store(true, std::memory_order_relaxed);
    return nullptr;
}

void* runTcpClient(void* port)
{
    // Dummy Logic
    unsigned short serverPort = *reinterpret_cast<unsigned short*>(port);
    if(serverPort == 0)
    {
        serverPort = *reinterpret_cast<unsigned short*>(port);
    }

    tcpMessage rxMsg;
    Receive:
    std::size_t received;
    void* buffer = &rxMsg;

    // Receive the packet from server
    if (socket.receive(buffer, sizeof(rxMsg), received) == sf::Socket::Done)
    {
        tcpMessage* receivedMsg = static_cast<tcpMessage*>(buffer);
        latestMsg.nVersion = receivedMsg->nVersion ;
        latestMsg.nType    = receivedMsg->nType    ;
        for (int i = 0; i < 1000; ++i) 
        {
            latestMsg.chMsg[i] = receivedMsg->chMsg[i];
        }
        latestMsg.nMsgLen  = receivedMsg->nMsgLen  ;
        std::cout << std::endl;
        // Printing the message received from the server
        std::cout << "Received Msg Type: " << static_cast<unsigned int>(latestMsg.nType) << "; Msg: " << latestMsg.chMsg  << std::endl;

        // Flagging the newMsgReceived for reinvoking the user-input background thread (written in main)
        newMsgReceived.store(true, std::memory_order_relaxed);

        // Continue receiving the packets from server
        goto Receive;
    }

 
    exitFlag.store(true, std::memory_order_relaxed);
    return nullptr;
}


////////////////////////////////////////////////////////////
/// Entry
///

int main(int argc, char* argv[]) 
{
    // Choose an arbitrary port for opening sockets
    unsigned short port = 50001;

    // Validating the inputs given from command line
    if (argc != 3) 
    {
        std::cout << "Invalid arguments proper usage: " << argv[0] << " <server IP> <port>" << std::endl;
        return EXIT_FAILURE;
    }
    else 
    {
        if(!isNaturalNumber(argv[2]))
        {
            std::cout << "Invalid arguments proper usage: " << argv[0] << " <server IP> <port>" << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    // Ask for the server address
    sf::IpAddress serverIP;
    serverIP = argv[1];
    port = static_cast<unsigned short>(std::stoi(argv[2]));

    // Connect to the server
    if (socket.connect(serverIP, port) != sf::Socket::Done)
    {
        std::cout << "Cant Connected to server " << serverIP << std::endl;
        return EXIT_FAILURE;
    }


    RunClient:
    pthread_t backgroundUserThread;
    pthread_t backgroundClientThread;
    // Reset the atomic variable which will be setted from receive background thread
    newMsgReceived.store(false, std::memory_order_relaxed);

    pthread_create(&backgroundUserThread,nullptr,runUserInput, &port);
    pthread_create(&backgroundClientThread,nullptr,runTcpClient, &port);
    

    while (true) 
    {
        if (exitFlag.load(std::memory_order_relaxed)) 
        {
            // Exit the forever loop and end the program
            break;
        }
        if (newMsgReceived.load(std::memory_order_relaxed)) 
        {
            // Reinvoke the background threads when new message is received
            goto RunClient;
        }
    
    }

    return EXIT_SUCCESS;
}