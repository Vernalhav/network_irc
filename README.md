# Internet Relay Chat in C
A C-level implementation of a chatting system. Made to consolidate networking concepts in practice.  

# Authors  
Giovani Lucafó  
Victor Giovannoni  
Victor Graciano  
  
# Instructions:
This program was developed with a Linux environment in mind (particularly, Ubuntu 18, but it should work with other Linux distros).  
It was compiled with GCC 7.5.0, but it should work with other GCC compilers because it is ANSI C compliant.  
## Build
After cloning the repo, simply run  
```make all```

## Run
First, run the server:  
```make server_test```  
Then, open up several Bash instances and in each one run:  
```make client_test```  
You can change the maximum number of users in the `MAX_USERS` macro in `server.c`.  
  
**NOTE:** You can also run this in serveral separate computers, with a few caveats. Simply change the client's server IP through the `/connect` command (make sure the server's ports are forwarded correctly).
  
  
## Usage  
Upon starting the client program, you are able to change the server connection settings (server's IPv4 address and port number), change your nickname and connect to the server.  
The commands to do this are  
```/connect - Connects to the server```  
```/server <IPv4> <port> - Change server connection settings```  
```/nickname <nickname> - Change your current nickname```  
  
Once you are connected to the server, you can send messages to all other connected clients or send commands just by typing in the terminal. The available commands are  
```/nickname <new name> - Change your nickname```  
```/ping - Check connection to server```  
```/join <channel name> - Join a channel or create a new one if name is unique```  
```/kick <user> - Admins can kick users from their channel```  
```/mute <user> - Admins can mute users from sending messages in their channel```  
```/whois <user> - Admins can see a user's IP address (not shown to all channel members)```  
```/unmute <user> - Admins can unmute users from sending messages in their channel```  
```/mode (+|-)<modes> - Admins can add or remove channel mode. For now the only option is i for invite-only```  
```/invite <user> - Admins can invite user to invite-only channel```  
```/quit - Exit the server (CTRL+D also terminates the application)```
