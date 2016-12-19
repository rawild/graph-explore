#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "connect.h"
#include "timer.h"
#include "graph-explore.h"


// HTTP CONNECTION MANAGEMENT

// helper functions
char* http_truncate_response(http_connection* conn);




// http_connect(ai)
//    Open a new connection to the server described by `ai`. Returns a new
//    `http_connection` object for that server connection. Exits with an
//    error message if the connection fails.
http_connection* http_connect(struct addrinfo* ai) {
    // connect to the server
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    int r = connect(fd, ai->ai_addr, ai->ai_addrlen);
    if (r < 0) {
        perror("connect");
		close(fd);
        exit(1);
    }

    // construct an http_connection object for this connection
    http_connection* conn =
        (http_connection*) malloc(sizeof(http_connection));
    conn->fd = fd;
    conn->state = HTTP_REQUEST;
    conn->eof = 0;
	conn->buf = malloc(10000000);  //initialized to an absurd length -> tried hard to get this dynamically allocated
								   //but I could not get it to grow fast enough
    return conn;
}


// http_close(conn)
//    Close the HTTP connection `conn` and free its resources.
void http_close(http_connection* conn) {
    close(conn->fd);
	if(strlen(conn->buf) > 0){  //must now also free buf
    	free(conn->buf);
	}

	if(conn){
		free(conn);
	}
}


// http_send_request(conn, uri)
//    Send an HTTP POST request for `uri` to connection `conn`.
//    Exit on error.
void http_send_request(http_connection* conn, char* uri, char *host) {
    assert(conn->state == HTTP_REQUEST || conn->state == HTTP_DONE);

    // prepare and write the request
    char reqbuf[BUFSIZ];
    size_t reqsz = snprintf(reqbuf, sizeof(reqbuf),
                            "POST /%s HTTP/1.0\r\n"
                            "Host: %s\r\n"
                            "Connection: keep-alive\r\n"
                            "\r\n",
                            uri, host);
    assert(reqsz < sizeof(reqbuf));

    size_t pos = 0;
    while (pos < reqsz) {

        ssize_t nw = write(conn->fd, &reqbuf[pos], reqsz - pos);


		if (nw == 0)
            break;
        else if (nw == -1 && errno != EINTR && errno != EAGAIN) {
            perror("write");
			http_close(conn);
            exit(1);
        } else if (nw != -1)
            pos += nw;
    }

    if (pos != reqsz) {
        fprintf(stderr, "%.3f sec: connection closed prematurely\n",
                elapsed());
		http_close(conn);
        exit(1);
    }

    // clear response information
    conn->state = HTTP_INITIAL;
    conn->status_code = -1;
    conn->content_length = 0;
    conn->has_content_length = 0;
    conn->len = 0;
    conn->has_connects = 1;
    conn->match = 0;
}


// http_receive_response_headers(conn)
//    Read the server's response headers. On return, `conn->status_code`
//    holds the server's status code. If the connection terminates
//    prematurely, `conn->status_code` is -1.
void http_receive_response_headers(http_connection* conn) {
    assert(conn->state != HTTP_REQUEST);
    if (conn->state < 0)
        return;

    // read & parse data until told `http_process_response_headers`
    // tells us to stop

    while (process_response_headers(conn)) {

		ssize_t nr = read(conn->fd, &conn->buf[conn->len], BUFSIZ);
        if (nr == 0)
            conn->eof = 1;
        else if (nr == -1 && errno != EINTR && errno != EAGAIN) {
            perror("read");
			http_close(conn);
            exit(1);
        } else if (nr != -1)
            conn->len += nr;
    }

    // Status codes >= 500 mean we are overloading the server
    // and should exit.
    if (conn->status_code >= 500) {

        fprintf(stderr, "%.3f sec: exiting because of "
                "server status %d (%s)\n", elapsed(),
                conn->status_code, http_truncate_response(conn));

		http_close(conn);
        exit(1);
    }
}


// http_receive_response_body(conn)
//    Read the server's response body. On return, `conn->buf` holds the
//    response body, which is `conn->len` bytes long and has been
//    null-terminated.
void http_receive_response_body(http_connection* conn, struct search *search) {
    assert(conn->state < 0 || conn->state == HTTP_BODY);
    if (conn->state < 0)
        return;

    // read response body (http_check_response_body tells us when to stop)
    while (check_response_body(conn, search) > 0) {
		ssize_t nr = read(conn->fd, &conn->buf[conn->len], BUFSIZ);

		if (nr == 0)
            conn->eof = 1;
        else if (nr == -1 && errno != EINTR && errno != EAGAIN) {
            perror("read");
			http_close(conn);
            exit(1);
        } else if (nr != -1)
            conn->len += nr;
    }

    // null-terminate body
    conn->buf[conn->len] = 0;
}




// http_truncate_response(conn)
//    Truncate the `conn` response text to a manageable length and return
//    that truncated text. Useful for error messages.
char* http_truncate_response(http_connection* conn) {
    char *eol = strchr(conn->buf, '\n');
    if (eol)
        *eol = 0;
    if (strnlen(conn->buf, 100) >= 100)
        conn->buf[100] = 0;
    return conn->buf;
}

