#include <SFML/Network.hpp>
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>

struct tcpMessage
{
    unsigned char nVersion;
    unsigned char nType;
    unsigned short nMsgLen;
    char chMsg[1000];
};
tcpMessage lastReceivedMessage;
std::mutex messageMutex;

void handleClient(sf::TcpSocket& client)
{
    while (true)
    {
        tcpMessage receivedMessage;
        std::size_t received;

        // Receive Message from the client
        if (client.receive(&receivedMessage, sizeof(receivedMessage), received) == sf::Socket::Done)
        {
            std::string strmsg = receivedMessage.chMsg;
            std::cout << strmsg << std::endl;

            // Handle received message
            if (receivedMessage.nVersion == 102)
            {
                if (receivedMessage.nType == 77)
                {
                    // Forward the message to the same client
                    client.send(&receivedMessage, sizeof(receivedMessage));
                    std::lock_guard<std::mutex> lock(messageMutex);
                    std::memcpy(lastReceivedMessage.chMsg, receivedMessage.chMsg, sizeof(receivedMessage.chMsg));
                }
                else if (receivedMessage.nType == 201)
                {
                    // Reverse the message and send it back to the same client
                    std::reverse(receivedMessage.chMsg, receivedMessage.chMsg + receivedMessage.nMsgLen);

                    client.send(&receivedMessage, sizeof(receivedMessage));
                    std::lock_guard<std::mutex> lock(messageMutex);
                    std::memcpy(lastReceivedMessage.chMsg, receivedMessage.chMsg, sizeof(receivedMessage.chMsg));
                }
            }
        }
        else
        {
            break; // Break out of the loop 
        }
    }
}

void acceptClient(sf::TcpListener& listener, sf::TcpSocket& client)
{
    // Wait for a new client to connect, this is blocking function
    if (listener.accept(client) != sf::Socket::Done)
    {
        std::cerr << "Failed to accept a client connection" << std::endl;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    unsigned short port = static_cast<unsigned short>(std::stoi(argv[1]));

    // Set up the listener
    sf::TcpListener listener;
    if (listener.listen(port) != sf::Socket::Done)
    {
        std::cerr << "Failed to bind to port " << port << std::endl;
        return 0;
    }

    std::cout << "Server is listening on port " << port << std::endl;

    // Start a thread for accepting the connected client
    sf::TcpSocket client;
    std::thread clientAcceptor(acceptClient, std::ref(listener), std::ref(client));

    // Start a thread for handling the connected client
    std::thread clientHandler(handleClient, std::ref(client));

    // Main thread for user input
    while (true)
    {
        // Prompt user for commands
        std::string command;
        std::cout << "Please enter command: ";
        std::cin >> command;
        // Handle user commands here
        if (command == "msg")
        {
            std::lock_guard<std::mutex> lock(messageMutex);
            std::string strmsg = lastReceivedMessage.chMsg;
            std::cout << "Last Message: " << strmsg << std::endl;
        }
        else if (command == "client")
        {
            std::cout << "IP Address: " << client.getRemoteAddress() << " | Port: " << client.getRemotePort() << std::endl;
        }
        else if (command == "exit")
        {
            // Close the socket and terminate the program
            std::cout << "get out";
            client.disconnect();
            break;
        }
    }

    // Wait for the client handling and accepting threads to finish before exiting
    clientAcceptor.join();
    clientHandler.join();
    return 0;
}

server from mine
