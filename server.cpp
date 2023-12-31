
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Network.hpp>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <list>
#include<cstring>
#include <pthread.h>
 
// tcpMessage packet defination
struct tcpMessage {
    unsigned char nVersion;
    unsigned char nType;
    unsigned short nMsgLen;
    char chMsg[1000];
};
 
 
tcpMessage lastMsg; // Latest message received among all the clients
bool exitProgram = false; // Indication to exit the code
bool newMsgReceived = false; // Indication when new message is received, which is used for loop back and broadcasting
unsigned short newMsgReceivedClientID;
// Create a list to store the future clients
std::list<sf::TcpSocket*> clients;
 
 
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
 
void* runTcpServer(void* serverPort)
{
    // Create a socket to listen to new connections
    unsigned short port = *reinterpret_cast<unsigned short*>(serverPort);
    sf::TcpListener listener;
    listener.listen(port);
    // Create a selector
    sf::SocketSelector selector;
    // Add the listener to the selector
    selector.add(listener);
    unsigned int nVersionLatest = 0;
    unsigned int nTypeLatest    = 0;
    // Endless loop that monitors connections
    while (!exitProgram)
    {
        if(newMsgReceived)
        {
            for (auto it1 = clients.begin(); it1 != clients.end(); ++it1)
            {
                sf::TcpSocket& clientLoc = **it1;
                
                if(((clientLoc.getRemotePort() != newMsgReceivedClientID) && (nTypeLatest == 77)) ||
                   ((clientLoc.getRemotePort() == newMsgReceivedClientID) && (nTypeLatest == 201))  )
                {
if (nTypeLatest == 201)
                {
                    // Reverse the message
                    std::string reversedMsg(lastMsg.chMsg);
                    std::reverse(reversedMsg.begin(), reversedMsg.end());
 
                    // Update the message with the reversed one
                    strncpy(lastMsg.chMsg, reversedMsg.c_str(), sizeof(lastMsg.chMsg));
}
 
                    void* buffer =  &lastMsg;
                    if (clientLoc.send(buffer, sizeof(lastMsg)) != sf::Socket::Done)
                        std::cout << "not able  to send message to client " << clientLoc.getRemotePort() << " from server !";
                }
 
            }
            newMsgReceived = false;
        }
        // Make the selector wait for data on any socket
        if (selector.wait())
        {
            // Test the listener
            if (selector.isReady(listener))
            {
                // The listener is ready: there is a pending connection
                sf::TcpSocket* client = new sf::TcpSocket;
                if (listener.accept(*client) == sf::Socket::Done)
                {
                
                    clients.push_back(client);
                    // Add the new client to the selector so that we will
 
                    selector.add(*client);
                }
                else
                {
                    // Error, we won't get a new connection, delete the socket
                    delete client;
                }
            }
            else
            {
                
    	        for (auto it = clients.begin(); it != clients.end();)
                {
                    sf::TcpSocket& client = **it;
                    if (selector.isReady(client))
                    {
                        
    		            tcpMessage rxMsg;
        	            std::size_t received;
                        void* buffer = &rxMsg;
        	            if (client.receive(buffer, sizeof(rxMsg), received) == sf::Socket::Done)
                        {
                            tcpMessage* receivedMsg = static_cast<tcpMessage*>(buffer);
                            nVersionLatest = static_cast<unsigned int>(receivedMsg->nVersion);
                            nTypeLatest    = static_cast<unsigned int>(receivedMsg->nType);
                            //  if version == 102 
                            if(nVersionLatest == 102)
                            {
    			                lastMsg.nVersion = receivedMsg->nVersion ;
    			                lastMsg.nType    = receivedMsg->nType    ;
                                for (int i = 0; i < 1000; ++i) 
                                {
                                    lastMsg.chMsg[i] = receivedMsg->chMsg[i];
                                }
    			                lastMsg.nMsgLen  = receivedMsg->nMsgLen  ;
                                newMsgReceived = true;
                                newMsgReceivedClientID =  client.getRemotePort();
                            }
                        }
                        else
                        {
                            // Client disconnected, remove from the list
                            selector.remove(client);
                            it = clients.erase(it);
                            delete &client;
                            continue;
                        }
                    }
                    ++it;
                }
            }
        }
    }
    return nullptr;
}
 
 
////////////////////////Main function////////////////////////////////////
int main(int argc, char* argv[])
{
    //Assigning socket port
    unsigned short port = 50001;

    if (argc == 2) 
    {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
 
    }
    pthread_t serverThread;
    pthread_create(&serverThread,nullptr,runTcpServer,&port);
 
    UserInput:
    std::cout << "Please enter command: ";
    std::string ipAdd;
    std::vector<std::string> result;
 
    // reading the ipAddress
    std::getline(std::cin, ipAdd);
    split(ipAdd,' ',3,result);
    bool valid = (result.size() == 1);
    valid = valid && ((result[0] == "msg") || (result[0] == "clients") || (result[0] == "exit"));
    if(!valid )
    {
        std::cout << "Please enter valid command with no spaces " << std::endl;
    }
    if(result[0] == "msg") 
        std::cout << "Last Message: " << lastMsg.chMsg << std::endl;
    if(result[0] == "clients" ) 
    {
        std::cout << "Number of Clients: " << clients.size() << std::endl;
        // Printing all the clients from list which we added in socketselector
        for (const auto& client : clients) 
        {
            
            unsigned short remotePort = client->getRemotePort();
 
            std::cout << "IP Address: " << "localhost" <<" | Port: " << remotePort <<std::endl;
        }
    }
 
    if(result[0] != "exit") 
    {
        goto UserInput;
    }
 
    exitProgram = true;
 
    
 
    return 0;
}