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
#include <iostream>
#include <cstring>
#include <sstream>
#include <thread>
#include <pthread.h>

struct tcpMessage
{
    unsigned char nVersion;
    unsigned char nType;
    unsigned short nMsgLen;
    char chMsg[1000];
};
// Function to split a string into a vector of strings using a delimiter

bool split(const std::string& s, char delimiter, std::vector<std::string>& results)
{
    bool bRC = true;
    if (s.empty())
    {
        bRC = false;
    }
    else
    {
        std::string token;
        std::istringstream tokenStream(s);

        while (std::getline(tokenStream, token, delimiter))
        {
            if (token != " ")
                results.push_back(token);
        }
    }

    return bRC;
}


void receiveMessage(sf::TcpSocket& socket)
{
   while(true)
   {
      tcpMessage receivedMessage;
      std::size_t received;
      if (socket.receive(&receivedMessage, sizeof(receivedMessage), received) == sf::Socket::Done)
      {
        return;
      }

   }
}

int main(int argc, char** argv)
{
    // Create a socket for communicating with the server
    sf::TcpSocket socket;
    // Start a separate thread to this thread send the receive function and its arguments
    std::thread receiverthread(receiveMessage, std::ref(socket));
     //This IPAddress is given to sfml serverIP application
    sf::IpAddress serverIP;
//This is for command line arguments
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <ip address> <port>" << std::endl;
        return -1;
    }

// This is when client gets IPAddress its string index is 1
//sf:: is for SFML
   
    serverIP = argv[1]; 

    unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));

    

    // Connect to the server from TCP.cpp program
    if (socket.connect(serverIP, port) != sf::Socket::Done)
    {
        std::cerr << "Failed to connect to the server" << std::endl;
        return -1;
    }

    std::cout << "Connected to server " << serverIP << " | Port: " << port << std::endl;

    // Set up the tcpMessage structure
    tcpMessage sendMessage;

    while (true)
    {
        // Prompt user for commands
        std::string command;
        std::cout << "Please enter command: ";
        std::getline(std::cin, command);
// send the whole command to bool split function
        std::vector<std::string> chkcmd;
        split(command,' ', chkcmd);
        if (chkcmd[0] == "v")
        {  // Set version number
            
            sendMessage.nVersion = static_cast<unsigned char>(std::stoi(chkcmd[1]));
        }
        else if (chkcmd[0] == "t")
        {
            // this prof gave Split the command into tokens using space as delimiter // this is the only thing i didnt get
            if (chkcmd.size() >= 2)
            {
                // Extract type number and message string
                sendMessage.nType = static_cast<unsigned char>(std::stoi(chkcmd[1]));

                // Combine the remaining tokens as the message string because t and "" is not counted
                // to find msglen 
                
                std::string message;
                for (unsigned int i = 2;i < chkcmd.size(); ++i)
                {
                   message += chkcmd[i];
                   if(i < chkcmd.size() - 1)
                   {
                     message += ' ';
                   }
                }
                sendMessage.nMsgLen = static_cast<unsigned short>(message.size());
                std::strncpy(sendMessage.chMsg, message.c_str(), sizeof(sendMessage.chMsg) - 1);
                sendMessage.chMsg[sizeof(sendMessage.chMsg) - 1] = '\0';
                // Display any messages received from the server
// Here we are taking pointer to the structure and not the full data like in tcp code            // Send the message to the server
                if (socket.send(&sendMessage, sizeof(sendMessage)) != sf::Socket::Done)
                {
                    std::cerr << "Failed to send message to the server" << std::endl;
                    break;
                }
                std::cout << sendMessage.chMsg << std::endl;
            }
        }
        else if (chkcmd[0] == "q")
        {
            // Close the socket and terminate the program
            socket.disconnect();
            break;
        }
        else
        {
            std::cout << "Invalid command. Please enter 'v', 't', or 'q'." << std::endl;
        }
    }
    receiverthread.join();
    return 0;
}


