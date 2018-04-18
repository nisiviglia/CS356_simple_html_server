// @author Nicholas Siviglia 31360256
// @Class CS356-101 (Evening)
// @version 1.0
// @date 2018-03-20
// @assignment HTTP Client 

#define _XOPEN_SOURCE 
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "./http_messages.h"

//Global Variables
const int NUM_OF_REQUESTS = 2;

int main(int argc, char **argv){

    //Check arguments
    if(argc != 2){
    
        printf("Invalid Arguments\n ./http_client address:port/filename\n");

        return 0;
    }

    //Split input
    char *argv_colon = strchr(argv[1], ':');

    char *argv_fslash = strchr(argv[1], '/');

    if(argv_colon == NULL || argv_fslash == NULL){

        printf("Invalid Arguments\n ./http_client address:port/filename\n");

        return 0;
    }

    //Get address
    char address[SMALL_BUFF_SIZE];

    memset(address, 0, SMALL_BUFF_SIZE);

    strncpy(address, argv[1], argv_colon - argv[1]);
    
    //Get Port
    char port_str[SMALL_BUFF_SIZE];

    memset(port_str, 0, SMALL_BUFF_SIZE);

    strncpy(port_str, argv_colon + 1, argv_fslash - argv[1]);

    size_t port = 0;

    port = atoi( port_str );
    
    //Get filename
    char filename[SMALL_BUFF_SIZE];

    memset(filename, 0, SMALL_BUFF_SIZE);

    strncpy(filename, argv_fslash + 1, SMALL_BUFF_SIZE);

    //Send two requests
    time_t if_modified_since_last = 0;

    for(int i=0; i < NUM_OF_REQUESTS; i++){
        
        //Open Socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if(sockfd == -1){

            printf("ERROR unable to open socket\nerrno: %s\n", strerror(errno));

            return -1;
        }

        //Bind the socket
        struct sockaddr_in client;

        memset(&client, 0, sizeof(struct sockaddr_in) );

        client.sin_family = AF_INET;

        client.sin_addr.s_addr = htonl(INADDR_ANY);

        client.sin_port = htons( 0 );

        int bind_rc = bind(sockfd, (struct sockaddr *) &client, sizeof(client));

        if(bind_rc){

            printf("ERROR unable to bind socket\nerrno: %s\n", strerror(errno));

            return -1;
        }
        
        //Fill in server info
        struct sockaddr_in server;

        memset(&server, 0, sizeof(server));

        server.sin_family = AF_INET;

        server.sin_addr.s_addr = address_to_ns( address ); 

        server.sin_port = htons( port );

        //Open connection
        int rc = connect(sockfd, (struct sockaddr*) &server, sizeof(server) ); 
        
        if(rc == -1){
            
            printf("ERROR unable to open connection to server\nerrno: %s\n"
                    , strerror(errno));
            
            return -1;
        }

        //Create request for first message
        http_request request;

        memset(&request, 0, sizeof(request) );

        snprintf(request.host, HOST_NAME_MAX, "%s:%lu", address, port);
        
        strncpy(request.filename, filename, NAME_MAX);

        request.if_modified_since = if_modified_since_last;

        char request_msg[LARGE_BUFF_SIZE];

        memset(request_msg, 0, LARGE_BUFF_SIZE);

        create_http_request_msg(&request, request_msg);

        //Send request
        rc = write(sockfd, request_msg, sizeof(char) * strlen(request_msg) );

        assert(rc != -1);

        //Get response
        char response_msg[LARGE_BUFF_SIZE];

        memset(response_msg, 0, LARGE_BUFF_SIZE);

        rc = read(sockfd, response_msg, LARGE_BUFF_SIZE);
             
        assert(rc != -1); 

        //parse response
        http_response response;

        memset(&response, 0, sizeof(response) );

        parse_http_response(&response, response_msg);

        //Record last modified time
        if_modified_since_last = response.last_modified;
        
        printf("-----------------------\n");

        //Print out response and reply
        printf("Request:\n%s\n", request_msg);

        printf("Response:\n%s\n", response_msg);

        printf("-----------------------\n");
        
        free_http_response(&response);
        
        //Cleanup 
        close(sockfd);
    }

    return 0;
}
