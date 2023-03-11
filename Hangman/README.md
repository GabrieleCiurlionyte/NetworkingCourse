# Hagman

A client and a multi-client server for playing the game "Hangman" written in C

### To compile client server
```bash
gcc clientServer.c -o clientServer.exe -lws2_32
```

### To run client client server
```bash
.\clientServer DEVICE_IP_ADDRESS
```

### To compile multi client server
```bash
gcc multiClientServer.c -o multiClientServer.exe -lws2_32   
```

### To run multi client server
```bash
.\multiClientServer  
```
