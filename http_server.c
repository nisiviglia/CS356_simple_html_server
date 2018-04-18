// @author Nicholas Siviglia 31360256
// @Class CS356-101 (Evening)
// @version 1.0
// @date 2018-03-20
// @assignment HTTP Server

#define _XOPEN_SOURCE 
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "./http_messages.h"

int main(int argc, char **argv){

    //Check arguments
    if(argc != 2){
    
        printf("Invalid Arguments\n ./server [port]\n");

        return 0;
    }

    //Open Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){

        printf("ERROR unable to open socket\n");

        return -1;
    }

    //Bind the socket
    struct sockaddr_in server;

    memset((char *) &server, 0, sizeof(server));

    server.sin_family = AF_INET;

    server.sin_addr.s_addr = htonl(INADDR_ANY);

    server.sin_port = htons( atoi(argv[1]) );

    int bind_rc = bind(sockfd, (struct sockaddr *) &server, sizeof(server));

    if(bind_rc){

        printf("ERROR unable to bind socket\n");

        return -1;
    }

    //listen for request 
    listen(sockfd, 5);

    printf("The server is ready to receive on port: %s\n", argv[1]);

    while(true){

        //Accept connection from waiting client
        struct sockaddr_in client;

        int client_sockfd;

        socklen_t client_len = sizeof(client);

        client_sockfd = accept(sockfd, (struct sockaddr *) &client, &client_len);

        assert( client_sockfd != -1);

        //Read message from client
        char *http_msg = (char*) calloc(LARGE_BUFF_SIZE, sizeof(char) );

        ssize_t msg_rc = 0;

        msg_rc = read(client_sockfd, http_msg, LARGE_BUFF_SIZE);

        assert( msg_rc != -1);

        //Parse message
        http_request request;

        memset(request.filename, 0, NAME_MAX);
        
        memset(request.host, 0, HOST_NAME_MAX);

        request.if_modified_since = 0;

        parse_http_request(&request, http_msg);

        //Printout conenction notification
        printf("Request for file: %s\n", request.filename);
        
        //Create response
        http_response response;

        response.body = NULL;

        response.date = 0;

        response.last_modified = 0;

        char *r_msg = (char*) calloc(LARGE_BUFF_SIZE, sizeof(char));

        //Put current time into response
        time_t rawtime;

        time(&rawtime);

        response.date = rawtime;
        
        //Find requested file
        struct stat file_stat;

        int file_rc = stat(request.filename, &file_stat);

        //File not found
        if(file_rc == -1){

            response.status_line = 404;

            create_http_response_msg(&response, r_msg, LARGE_BUFF_SIZE);            

            //print 9ut notification FNF
            printf("File, %s not found\n", request.filename);

        }
        
        //File found
        else{

            //Check last modified date
            response.last_modified = file_stat.st_mtime;

            //Not modified since last request, send 304
            if( request.if_modified_since >= file_stat.st_mtime ){

                response.status_line = 304;

                create_http_response_msg(&response, r_msg, LARGE_BUFF_SIZE);
            }
            
            //New response
            else{
                
                //Read data from file
                FILE *fp;

                fp = fopen(request.filename, "r");

                assert(fp != NULL);

                fseek(fp, 0, SEEK_END);

                response.content_length = ftell(fp);

                rewind(fp);

                response.body = (char*) calloc( response.content_length + 1, sizeof(char) ); 

                assert(response.body != NULL);

                size_t fread_rc = fread(response.body, response.content_length, 1, fp);

                fclose(fp);

                //Create response message
                response.status_line = 200;

                create_http_response_msg(&response, r_msg, LARGE_BUFF_SIZE);

            }

        }
        
        //Reply with response
        msg_rc = write(client_sockfd, r_msg, sizeof(char) * strlen(r_msg));

        assert( msg_rc != -1);

        //Cleanup 
        close(client_sockfd);

        free(http_msg);

        free(r_msg);

        free_http_response(&response);

    }//END while

    //Close socket
    close(sockfd);

    return 0;
}
