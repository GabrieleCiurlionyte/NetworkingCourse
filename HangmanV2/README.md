# Hagman

A client and a client server for playing the game "Hangman" written in with WinSock2 and C programming language. Multi-client service is done with winsock function select() and parallel threading.

### To compile client server
```bash
gcc clientServer.c -o clientServer.exe -lws2_32
```

### To run client server
```bash
.\clientServer DEVICE_IP_ADDRESS
```

### To compile multi-client server
```bash
gcc multiClientServer.c -o multiClientServer.exe -lws2_32   
```

### To run multi-client server
```bash
.\multiClientServer  
```
