#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 4096

int ReceiveInt(SOCKET ConnectSocket, char *recvbuf, int recvbuflen);
char *ReceiveString(SOCKET ConnectSocket, char *recvbuf, int recvbuflen);
void sendLetter(char c, SOCKET ConnectSocket, char *sendbuf);

int isLetter(char c);
int isIn(char str[], char c);

int main(int argc, char *argv[])
{

    WSADATA wsaData; // Contains data about Windows sockets implementation
    int iResult;

    fd_set read_set;

    // Make request for version 2.2 of Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Creating address info for socket
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // To use IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP protocol
    hints.ai_protocol = IPPROTO_TCP;

    // Get IP address from the command line
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    // Attempt to connect to the first valid address that matches hints structure.
    ptr = result;
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                           ptr->ai_protocol);

    // Check if socket is valid
    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Connect to server.
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }
    freeaddrinfo(result);
    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Defining buffers
    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];

    // Receive information from the game:
    int lives, win, it;
    // TODO: get initial values from server, IT HAS TO SEND TOO BECAUSE NO IMPLEMENTATION
    lives = 6;
    win = 0;
    it = 0;

    int iteration = 0;

    while (lives > 0 && !win)
    {
        // Receive lifes
        lives = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
        // Receive win
        win = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
        // Receive current word
        char *receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
        printf("%s", receivedString);

        // Receive hanged man
        for (int i = 0; i < 6; i++)
        {
            char *receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
            printf("%s", receivedString);
        }

        // Receive incorrect letters
        for (int i = 0; i < 2; i++)
        {
            receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
            printf("%s", receivedString);
        }

        char letter;
        do
        {
            // Receive a string to enter a letter
            receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
            printf("%s", receivedString);
            fflush(stdin);
            scanf("%c", &letter);
            // Send entered letter
            sendLetter(letter, ConnectSocket, sendbuf);
        } while (!isLetter(letter));

        // Receive new line
        receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
        printf("%s", receivedString);

        // Results of check
        int isIncorrectWord, isCorrectAlreadyTried, isIncorrectNew;

        // Receive if correct word
        isIncorrectWord = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
        // printf("Is incorrect word %d", isIncorrectWord);
        if (isIncorrectWord)
        {
            // Receive if isAlredyTried
            isCorrectAlreadyTried = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
            // printf("isCorrectAlreadyTried %d", isCorrectAlreadyTried);
            if (isCorrectAlreadyTried)
            {
                // Receive notification
                receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
                // printf("%s", receivedString);
            }
        }
        else
        {
            // Notification that word is incorrect;
            receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
            printf("%s", receivedString);

            // Receive is incorrect new
            isIncorrectNew = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
            if (!isIncorrectNew)
            {
                // Receive notification
                receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
                printf("%s", receivedString);
            }
        }

        lives = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
        win = ReceiveInt(ConnectSocket, recvbuf, recvbuflen);
        // system("@cls||clear");
    }

    system("@cls||clear");
    if (win)
    {
        char *receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
        printf("%s", receivedString);
    }
    else
    {
        char *receivedString = ReceiveString(ConnectSocket, recvbuf, recvbuflen);
        printf("%s", receivedString);
    }
    printf("Thanks for playing!");
    clock_t start_time = clock();
    while (clock() < start_time + 1600)
        ;

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Closing of the socket
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

int ReceiveInt(SOCKET ConnectSocket, char *recvbuf, int recvbuflen)
{
    int iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        return atoi(recvbuf);
    }
    else if (iResult == 0)
    {
        printf("Connection closed\n");
        return -1;
    }
    else
    {
        printf("recv int failed: %d\n", WSAGetLastError());
        return -1;
    }
}

char *ReceiveString(SOCKET ConnectSocket, char *recvbuf, int recvbuflen)
{
    int iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        return recvbuf;
    }
    else if (iResult == 0)
    {
        return "Connection closed\n";
    }
    else
    {
        return "recv string failed\n";
    }
}

void sendLetter(char c, SOCKET ConnectSocket, char *sendbuf)
{
    sprintf(sendbuf, "%c", c);
    int buf_len = strlen(sendbuf);
    int iResult = send(ConnectSocket, sendbuf, buf_len, 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        exit(1);
    }
}

int isLetter(char c)
{
    if (c < 65 || (c > 90 && c < 97) || c > 122)
        return 0;
    else
        return 1;
}

int isIn(char str[], char c)
{
    for (int i = 0; str[i] != '\0'; i++)
        if (str[i] == c)
            return 1;
    return 0;
}