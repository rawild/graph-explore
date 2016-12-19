/*
 * connect.h
 *
 *  Created on: Dec 16, 2016
 *      Author: awilde
 */

#ifndef CONNECT_H_
#define CONNECT_H_

#include "graph-explore.h"
#ifndef PORT
# define PORT "80"
#endif

#ifndef USER
# define USER "_j_com"
#endif

// `http_connection::state` constants
#define HTTP_REQUEST 0      // Request not sent yet
#define HTTP_INITIAL 1      // Before first line of response
#define HTTP_HEADERS 2      // After first line of response, in headers
#define HTTP_BODY    3      // In body
#define HTTP_DONE    (-1)   // Body complete, available for a new request
#define HTTP_CLOSED  (-2)   // Body complete, connection closed
#define HTTP_BROKEN  (-3)   // Parse error
#define CONN_NUM	 29	// number of allowed open thread connections



// http_connection
//    This object represents an open HTTP connection to a server.
typedef struct http_connection http_connection;
struct http_connection {
    int fd;                 // Socket file descriptor
    int state;              // Response parsing status (see below)
    int status_code;        // Response status code (e.g., 200, 402)
    size_t content_length;  // Content-Length value
    int has_content_length; // 1 iff Content-Length was provided
    int eof;                // 1 iff connection EOF has been reached
    int has_connects;		// BUFSIZ multiple index at which other links in file of are located
    int match;				// BUFSIZ multiple index at which match is found
    char *buf;       		// Response buffer is a string
    size_t len;             // Length of response buffer
};

http_connection* http_connect(struct addrinfo* ai);
void http_close(http_connection* conn) ;
void http_send_request(http_connection* conn, char* uri, char *host);
void http_receive_response_headers(http_connection* conn);
void http_receive_response_body(http_connection* conn, struct search *search);
char* http_truncate_response(http_connection* conn);


#endif /* CONNECT_H_ */
