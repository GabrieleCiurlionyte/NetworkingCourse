# Hagman

A client and a client server for playing the game "Hangman" written in with WinSock2 and C programming language.

### To compile client server
```bash
gcc clientServer.c -o clientServer.exe -lws2_32
```

### To run client client server
```bash
.\clientServer DEVICE_IP_ADDRESS
```

### To compile client server
```bash
gcc multiClientServer.c -o multiClientServer.exe -lws2_32   
```

### To run client server
```bash
.\multiClientServer  
```
