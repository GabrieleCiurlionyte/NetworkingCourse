#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 4096

void sendIntToClient(int number, SOCKET ClientSocket);
void sendStringToClient(SOCKET ClientSocket, char *sent_string);
void sendHangman(int lives, SOCKET ClientSocket);
char ReceiveEnteredLetter(SOCKET ClientSocket, char *recvbuf, int recvbuflen);

void printTitle(char a[]);
char toCaps(char c);
int isLetter(char c);
int isIn(char str[], char c);
void printLettersToBuffer(char str[], char buffer[]);
int countlines(char *filename);

int main()
{

    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE; // Flag for bind

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    // Check if created socket is valid
    if (ListenSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Binding address to socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    // Listening for a connection:
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) // SOMAXCONN - constant
    {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accepting connection:

    // TODO: add threading. Do this for every single thread
    //  Create a temporary socket to listen to connections
    SOCKET ClientSocket;
    ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET)
    {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Receiving and sending data variables
    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // GAME PREPARATION
    // Game variables
    char letter;
    int lives, win;
    char correctWord[200];
    char currentWord[200];
    char incorrectLetters[27];
    int it; //"incorrectLetters" string iterator in the game

    FILE *words, *lineFile;
    int line;

    /*Check if "words.txt" exists*/
    words = fopen("words.txt", "r+");
    if (words == NULL)
    {
        printf("Error: words.txt not found");
        return 1;
    }
    fclose(words);

    /*Initiate randomizer*/
    srand(time(NULL));

    lives = 6;
    win = 0;
    it = 0;

    /*Generate random number*/
    line = rand() % countlines("words.txt");

    /*Get word from random number of line*/
    words = fopen("words.txt", "r+");
    if (words == NULL)
    {
        printf("Error: words.txt not found");
        return 1;
    }
    for (int i = 3; fgets(correctWord, 200, words) && i <= line; i++)
        ;
    fgets(correctWord, 200, words);
    fclose(words);

    /*Pass correct word to caps*/
    for (int i = 0; i < 200; i++)
        correctWord[i] = toCaps(correctWord[i]);

    /*Prepare current word*/
    strcpy(currentWord, correctWord);
    for (int i = 0; currentWord[i] != '\0'; i++)
        currentWord[i] = '_';
    incorrectLetters[0] = '\0';

    /*Game*/
    while (lives > 0 && !win)
    {
        // Send information of lives:
        sendIntToClient(lives, ClientSocket);
        // Send information of win:
        sendIntToClient(win, ClientSocket);
        // Send current word:
        char string[DEFAULT_BUFLEN];

        sprintf(string, "\n\nWord: %s\n", currentWord);
        sendStringToClient(ClientSocket, string);

        // Send hangman
        sendHangman(lives, ClientSocket);

        // Send incorrect letters
        sendStringToClient(ClientSocket, "\nIncorrect letters: ");
        if (incorrectLetters[0] != '\0')
        {
            printLettersToBuffer(incorrectLetters, string);
            sendStringToClient(ClientSocket, string);
        }
        else
        {
            sendStringToClient(ClientSocket, "none");
        }

        // Receive letter from Client
        do
        {
            // Send a request to enter a letter:
            sendStringToClient(ClientSocket, "\nEnter a letter: ");
            // Receive an entered letter:
            letter = ReceiveEnteredLetter(ClientSocket, recvbuf, recvbuflen);
        } while (!isLetter(letter));

        if (letter >= 97)
            letter = toCaps(letter);

        // Send request to print new line
        sendStringToClient(ClientSocket, "\n");

        int isInCorrectWord, isCorrectAlreadyTried, isIncorrectNew;

        // Send result if correct word
        isInCorrectWord = isIn(correctWord, letter);
        sendIntToClient(isInCorrectWord, ClientSocket);

        if (isInCorrectWord)
        {
            int isCorrectAlreadyTried = isIn(currentWord, letter);
            sendIntToClient(isCorrectAlreadyTried, ClientSocket);
            if (isCorrectAlreadyTried)
            {
                // Send notification
                sprintf(string, "Letter %c is already in your current word.", letter);
                sendStringToClient(ClientSocket, string);
                lives--;
            }
            else
            {
                for (int i = 0; correctWord[i] != '\0'; i++)
                    if (correctWord[i] == letter)
                        currentWord[i] = letter;
            }
        }
        else
        {
            sprintf(string, "Letter %c isn't in the correct word.", letter);
            sendStringToClient(ClientSocket, string);

            isIncorrectNew = !isIn(incorrectLetters, letter);
            sendIntToClient(isIncorrectNew, ClientSocket);

            if (isIncorrectNew)
            {
                incorrectLetters[it] = letter;
                incorrectLetters[it + 1] = '\0';
                it++;
            }
            else
            {
                sendStringToClient(ClientSocket, "\nAlso, you had tried this letter earlier.");
            }
            lives--;
        }

        if (strcmp(currentWord, correctWord) == 0)
            win = 1;
        if (lives == 0)
            win = 0;

        sendIntToClient(lives, ClientSocket);
        sendIntToClient(win, ClientSocket);
    }

    
    if (win)
    {
        sendStringToClient(ClientSocket, "You have won\n");
        printf("Sent win message");
    }
    else
    {
        char string[DEFAULT_BUFLEN];
        sprintf(string, "You have lost\n\n\nThe correct word was %s.\n", correctWord);
        sendStringToClient(ClientSocket, string);
        printf("Sent lost message");
    }

    printf("Game ended!");

    // shutdown send
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    printf("Done");

    return 0;
}

void sendIntToClient(int number, SOCKET ClientSocket)
{
    char recvbuf[DEFAULT_BUFLEN];
    memset(recvbuf, 0, sizeof(recvbuf));
    sprintf(recvbuf, "%d", number);
    int iSendResult = send(ClientSocket, recvbuf, sizeof(recvbuf), 0);
    if (iSendResult == SOCKET_ERROR)
    {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        exit(1);
    }
}

void sendStringToClient(SOCKET ClientSocket, char *string)
{
    char recvbuf[DEFAULT_BUFLEN];
    memset(recvbuf, 0, sizeof(recvbuf));
    sprintf(recvbuf, "%s", string);
    int iSendResult = send(ClientSocket, recvbuf, sizeof(recvbuf), 0);
    if (iSendResult == SOCKET_ERROR)
    {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        exit(1);
    }
}

// HANGED MAN FUNCTIONS
void sendHangman(int lives, SOCKET ClientSocket)
{
    char string[DEFAULT_BUFLEN];
    switch (lives)
    {
    case 6:
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    case 5:
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   O\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    case 4:
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   O\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   %c\n", 186, 179);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   %c\n", 186, 179);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    case 3:
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   O\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c %c%c%c\n", 186, 196, 196, 180);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   %c\n", 186, 179);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    case 2:
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   O\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c %c%c%c%c%c\n", 186, 196, 196, 197, 196, 196);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   %c\n", 186, 179);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    case 1:
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   O\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c %c%c%c%c%c\n", 186, 196, 196, 197, 196, 196);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   %c\n", 186, 179);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c  %c\n", 186, 47);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    case 0:
        printf("Last life reached.\n");
        sprintf(string, "%c%c%c%c%c\n", 201, 205, 205, 205, 187);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   O\n", 186);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c %c%c%c%c%c\n", 186, 196, 196, 197, 196, 196);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c   %c\n", 186, 179);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c  %c %c\n", 186, 47, 92);
        sendStringToClient(ClientSocket, string);
        sprintf(string, "%c\n", 202);
        sendStringToClient(ClientSocket, string);
        break;

    default:
        break;
    }
}

char ReceiveEnteredLetter(SOCKET ClientSocket, char *recvbuf, int recvbuflen)
{
    int iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        return recvbuf[0];
    }
    else
    {
        closesocket(ClientSocket);
        WSACleanup();
        return -1;
    }
}

void printLettersToBuffer(char str[], char buffer[])
{
    int offset = sprintf(buffer, "%c", 40);
    for (int i = 0; str[i] != '\0'; i++)
    {
        offset += sprintf(buffer + offset, "%c, ", str[i]);
    }
    offset += sprintf(buffer + offset, "%c%c", 8, 8);
    sprintf(buffer + offset, "%c", 41);
}

char toCaps(char c)
{
    if (isLetter(c))
        return c - 32;
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

int countlines(char *filename)
{
    FILE *file;
    int linesCount = 0;
    char c;

    file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error: %s not found", filename);
        return 1;
    }

    for (; c != EOF; c = fgetc(file))
        if (c == '\n')
            linesCount++;
    linesCount++;

    fclose(file);

    return linesCount;
}
