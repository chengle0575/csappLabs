#include <stdio.h>
#include <stdint.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h> 
#include <unistd.h>


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";



//prototype
void proxyserverinit(char *);

int main()
{
    
    printf("%s", user_agent_hdr);

    //parse the parameter read from command line
    
    
    //buffer to 
    char buffer[MAX_CACHE_SIZE];
    

    //read the data rom origin client to buffer
    proxyserverinit(buffer);


    return 0;
}




void proxyserverinit(char *buffer){
    //work as server to the original client

    //create a internet socket
    int proxyfds=socket(AF_INET,SOCK_STREAM,0);
    if(proxyfds==-1){printf("create proxy client fd failed\n");}
    
    
    //struct the address
    struct sockaddr_in myaddr; //sin_port and sin_addr memebers shall be in network byte order
    memset(&myaddr,0,sizeof(myaddr)); //clear the memory for this socket address and fill them all 0
    myaddr.sin_family=AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&(myaddr.sin_addr)); //converts numbers-and-dots notation into network byte order, and copies to myaddr
    uint16_t port=8000;
    myaddr.sin_port=htons(port); //convert port number to network byte order and assign to myaddr

    
    //bind the addr to my proxy machine
    bind(proxyfds,(struct sockaddr *)&myaddr,sizeof(myaddr)); //use generic sockaddr struct cast to fullfill bind parameter needs


    listen(proxyfds,1); 

    
    struct sockaddr originclientaddr; //this is the structure to store peer address
    socklen_t addlen=sizeof(originclientaddr);
    memset(&originclientaddr,0,sizeof(originclientaddr));
    

    
    while (1)
    {
        int connectfd=accept(proxyfds,&originclientaddr,&addlen); 
        if(connectfd==-1){printf("not able to accept\n");}
        else{
             //read buffer
             read(connectfd,buffer,sizeof(buffer)); //read from file to memory
             printf("server recived:%s\n",buffer);
        }
        
       

    }

}



void proxyclientinit(char *addr, uint16_t port, char * buffer){
    /*work as client to the original server*/
    
    //create a intenet socket
    int proxyfdc=socket(AF_INET,SOCK_STREAM,0);
    if(proxyfdc==-1){printf("create proxy client fd failed\n");}

    //struct the socket address
    struct sockaddr_in originalserver;
    originalserver.sin_family=AF_INET;
    inet_pton(AF_INET,addr,&(originalserver.sin_addr));
    originalserver.sin_port=htons(port);

    if(connect(proxyfdc,(struct sockaddr *)&originalserver,sizeof(addr))==-1)
        {printf("connect to original server fail\n"); 
         return;
        }
    else{
        printf("connected to original server successfully\n");

        //keep send data from buffer to the original server
        while(1){
              write(proxyfdc,buffer,sizeof(buffer));
              //do some error checking, when stop and end
        }
    
    };



     
    
}



