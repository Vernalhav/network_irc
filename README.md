# Internet Relay Chat in C
A C-level implementation of a chatting system. Made to consolidate networking concepts in practice.  

# Authors  
Victor Giovannoni
Victor Graciano
Giovani Decico
  
# Instructions:
This program was developed with a Linux environment in mind (particularly, Ubuntu 18, but it should work with other Linux distros).  
It was compiled with GCC 7.5.0, but it should work with other GCC compilers because it is ANSI C compliant.  
## Build
After cloning the repo, simply run  
```make all```

## Run
First, run the server:  
```make server_test```  
Then, open up two more Bash instances and in each one run:  
```make client_test```  
  
**NOTE:** You can also run this in serveral separate computers, with a few caveats. If you wish to run this with several computers connected to the same LAN, simply change the client's `SERVER_ADDR` macro in `network_utils.h` to the IP address of the server computer. To achieve the same effect on different computers, it should suffice to perform the same steps after opening the corresponding port on the server's modem, but this has not been tested. If this distributed approach is used, it is not required to run `make server_test` on the client machines.


## Usage
You can send messages to all other connected clients just by typing in the terminal. To quit, either run `/quit` or send an EOF (ctrl+d).  
