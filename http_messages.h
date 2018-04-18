// @author Nicholas Siviglia 31360256
// @Class CS356-101 (Evening)
// @version 1.0
// @date 2018-03-20
// @assignment HTTP data structures and functions 
#ifndef HTTP_MESSAGES
#define HTTP_MESSAGES

#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif

//Macros and globals
#define SMALL_BUFF_SIZE 256
#define MED_BUFF_SIZE 512
#define LARGE_BUFF_SIZE 4096
const char TIME_FORMAT[] = "%a, %d %b %Y %H:%M:%S %Z\r\n";

typedef struct http_response{

    int status_line;
    time_t  date;
    time_t last_modified;
    int content_length;
    char *body;

} http_response;

typedef struct http_request{

    char filename[NAME_MAX];
    char host[HOST_NAME_MAX];
    time_t if_modified_since;

} http_request;

in_addr_t address_to_ns(char *str_addr){

    //Check if already valid
    in_addr_t ns_addr = inet_addr( str_addr );

    if(ns_addr != (in_addr_t) -1){
        
        return ns_addr;
    }

    //Convert to ipv4
    struct hostent *server = NULL;

    server = gethostbyname(str_addr);

    if(server == NULL){
       
        return 0;
    }

    //Pain in the butt
    //Citation: https://paulschreiber.com/blog/2005/10/28/simple-gethostbyname-example/
    return inet_addr(  inet_ntoa( *(struct in_addr*) server->h_addr_list[0]) );
}

void parse_http_request(struct http_request *request, const char *in_msg){
    
    //Copy message
    char *msg = (char*) calloc( strlen(in_msg) + 1, sizeof(char) );
    
    assert(msg != NULL);

    strncpy(msg, in_msg, strlen(in_msg));

    //Split msg into tokens
    char delim[] = "\r\n";

    char *token = strtok(msg, delim);

    //Loop through tokens
    while( token != NULL ){
        
        if( strncmp("GET", token, 3) == 0){

            char *start = strchr(token, '/'); 

            char *end = strrchr(token, ' ');

            strncpy(request->filename, ".", NAME_MAX);

            strncat(request->filename, start, end - start);
        }
        else if( strncmp("Host:", token, 5) == 0){

            strncpy(request->host, token + 6, HOST_NAME_MAX);
        
        }
        else if( strncmp("If-Modified-Since:", token, 18) == 0){
            
            char date[MED_BUFF_SIZE];

            strncpy(date, token + 19, MED_BUFF_SIZE);

            struct tm *in_date = (struct tm*) calloc(1, sizeof(struct tm) );

            strptime(date, TIME_FORMAT, in_date);

            request->if_modified_since = mktime(in_date);

            free(in_date);
        }
        
        token = strtok(NULL, delim);
    }//END while

    //Cleanup
    free(msg);
}

void parse_http_response(struct http_response *response, const char *in_msg){
    
    //Copy message
    char *msg = (char*) calloc( strlen(in_msg) + 1, sizeof(char) );
    
    assert(msg != NULL);

    strncpy(msg, in_msg, strlen(in_msg));

    //Split msg into tokens
    char delim[] = "\r\n";

    char *token = strtok(msg, delim);

    //Loop through tokens
    while( token != NULL ){

        if( strncmp("HTTP/1.1", token, 8) == 0){

            char str_status_code[SMALL_BUFF_SIZE];

            memset(str_status_code, 0, SMALL_BUFF_SIZE);

            strncpy(str_status_code, token + 9, 3);

            response->status_line = atoi(str_status_code);
        }
        else if( strncmp("Date:", token, 5) == 0){
            
            char date[SMALL_BUFF_SIZE];

            strncpy(date, token + 6, SMALL_BUFF_SIZE);

            struct tm *in_date = (struct tm*) calloc(1, sizeof(struct tm) );

            strptime(date, TIME_FORMAT, in_date);

            response->date = mktime(in_date);

            free(in_date);
        }
        else if( strncmp("Last-Modified:", token, 14) == 0){
            
            char date[SMALL_BUFF_SIZE];

            strncpy(date, token + 15, SMALL_BUFF_SIZE);

            struct tm *in_date = (struct tm*) calloc(1, sizeof(struct tm) );

            strptime(date, TIME_FORMAT, in_date);

            response->last_modified = mktime(in_date);

            free(in_date);
        }
        else if( strncmp("Content-Length", token, 14) == 0){

            char content_length[SMALL_BUFF_SIZE];

            char *start = token + 15;

            strncpy(content_length, start, SMALL_BUFF_SIZE);

            response->content_length = atoi(content_length);
        }
        else if( strncmp("<html>", token, 6) == 0){

            response->body = (char*) calloc( strlen(token) + 1, sizeof(char) ); 

            assert(response->body != NULL);

            strncpy(response->body, token, strlen(token));
        }

        token = strtok(NULL, delim);
    }
    
    //Cleanup
    free(msg);
}

void create_http_request_msg(const struct http_request *request, char *msg){

    //Request line
    char request_line[MED_BUFF_SIZE];

    snprintf(request_line, MED_BUFF_SIZE, "GET /%s HTTP/1.1\r\n", request->filename);

    strncpy(msg, request_line, MED_BUFF_SIZE);

    //Host line
    char host_line[MED_BUFF_SIZE];

    snprintf(host_line, MED_BUFF_SIZE, "Host: %s\r\n", request->host);

    strcat(msg, host_line);

    //If modified line
    if( request->if_modified_since != 0){
        
        char time[SMALL_BUFF_SIZE];

        memset(time, 0, SMALL_BUFF_SIZE);

        strftime(time, SMALL_BUFF_SIZE, TIME_FORMAT, gmtime(&request->if_modified_since ));

        char if_modified_line[SMALL_BUFF_SIZE];

        memset(if_modified_line, 0, SMALL_BUFF_SIZE);
    
        snprintf(if_modified_line, MED_BUFF_SIZE, "If-Modified-Since: %s", time);
    
        strcat(msg, if_modified_line);
    } 

    //Blank line
    strcat(msg, "\r\n");
}

void create_http_response_msg(const struct http_response *response, char *msg, size_t msg_size){
    
    //Date
    char time[SMALL_BUFF_SIZE];

    memset(time, 0, SMALL_BUFF_SIZE);

    strftime(time, SMALL_BUFF_SIZE, TIME_FORMAT, gmtime(&response->date) );

    char date_line[SMALL_BUFF_SIZE] = "Date: ";

    strncat(date_line, time, SMALL_BUFF_SIZE);

    //Status line
    if(response->status_line == 200){

        strcpy(msg, "HTTP/1.1 200 OK\r\n");

        //Date
        strcat(msg, date_line);

        //Last modified
        memset(time, 0, SMALL_BUFF_SIZE);

        strftime(time, SMALL_BUFF_SIZE, TIME_FORMAT, gmtime(&response->last_modified));

        char modified_line[SMALL_BUFF_SIZE] = "Last-Modified: ";

        strcat(modified_line, time);

        strcat(msg, modified_line);

        //Content length
        char content_len_line[SMALL_BUFF_SIZE];

        char int_str[10];

        snprintf(int_str, 10, "%d", response->content_length);

        snprintf(content_len_line, SMALL_BUFF_SIZE, "Content-Length: %s\r\n", int_str);

        strcat(msg, content_len_line);

        //Content type
        char content_type_line[] = "Content-Type: text/html; charset=UTF-8\r\n";

        strcat(msg, content_type_line);

        //Blank line
        strcat(msg, "\r\n");

        //Body
        size_t msg_len = strlen(msg);

        if(msg_len + response->content_length > msg_size){

            char *temp_msg = msg;

            msg = (char*) calloc( (response->content_length + msg_size) * 2, sizeof(char) );

            assert(msg != NULL);

            strcpy(msg, temp_msg);

            free(temp_msg);
        }

        strcat(msg, response->body);
    }
    else if (response->status_line == 304){

        strcpy(msg, "HTTP/1.1 304 Not Modified\r\n");

        strcat(msg, date_line);

        strcat(msg, "\r\n");
    }
    else{
        strcpy(msg, "HTTP/1.1 404 Not Found\r\n");

        strcat(msg, date_line);
        
        strcat(msg, "\r\n");
    }
}

void free_http_response(struct http_response *response){

    if(response->body != NULL){
    
        free(response->body);

        response->body = NULL;
    }
}

