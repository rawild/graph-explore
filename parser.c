// HTTP CONNECTION MANAGEMENT
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





// HTTP PARSING

// process_response_headers(conn)
//    Parse the response represented by `conn->buf`. Returns 1
//    if more header data remains to be read, 0 if all headers
//    have been consumed.
 int process_response_headers(http_connection* conn) {

	size_t i = 0;
	while ((conn->state == HTTP_INITIAL || conn->state == HTTP_HEADERS)
			&& i + 2 <= conn->len) {
		if (conn->buf[i] == '\r' && conn->buf[i+1] == '\n') {
			conn->buf[i] = 0;
			if (conn->state == HTTP_INITIAL) {
				int minor;
				if (sscanf(conn->buf, "HTTP/1.%d %d",
						&minor, &conn->status_code) == 2)
					conn->state = HTTP_HEADERS;
				else
					conn->state = HTTP_BROKEN;
			} else if (i == 0)
				conn->state = HTTP_BODY;
			else if (strncmp(conn->buf, "Content-Length: ", 16) == 0) {
				conn->content_length = strtoul(conn->buf + 16, NULL, 0);
				conn->has_content_length = 1;
			}

			memmove(conn->buf, conn->buf + i + 2, conn->len - (i + 2));
			conn->len -= i + 2;
			i = 0;
		} else
			++i;
	}

	if (conn->eof)
		conn->state = HTTP_BROKEN;
	return conn->state == HTTP_INITIAL || conn->state == HTTP_HEADERS;
}


// check_response_body(conn, search)
//    Returns 1 if more response data should be read into `conn->buf`,
//    0 or less if the response is done.
// 	  Checks for the presence of the search values
 int check_response_body(http_connection* conn, struct search *search) {

	 static size_t i;
	 while (conn->state == HTTP_BODY && i < conn->len) {
		 char *value = NULL;
		 char *attr = strstr(conn->buf, search->attr);

		 if(attr){
			 value = strchr(attr, '\n');
			 if(!value){
				 value = strchr(attr, '\0');
			 }
			 if(value){
				 char *substr = malloc(sizeof(char) * (value-attr));
				 strncpy(substr, attr, value-attr);
				 if(strstr(substr, search->value)){
					 if(i == 0){
						 conn->match = 1;
					 }else{
						 conn->match = i;
					 }
				}
			 }
		 }

		 if ( (conn->has_content_length || conn->eof)
				 && conn->len >= conn->content_length ) {
			 conn->state = HTTP_DONE;
		 }
		 if (conn->eof && conn->state == HTTP_DONE){
			 conn->state = HTTP_CLOSED;
			 i = 0;
		 }
		 else if (conn->eof){
			 conn->state = HTTP_BROKEN;
		 }

		 i += BUFSIZ;
	 }
	 if(i>conn->len){
		 conn->state = HTTP_DONE;
	 }
	return conn->state;
}

 //parse_body(conn)
 //		Returns a uri_list list of list objects for all connected to: values in a response_body
 uri_list *parse_body(http_connection *conn){
	 char* start = &conn->buf[conn->has_connects];
	 char *iter = NULL;
	 char *end = NULL;
	 uri_list *new = new_uri_list();


	 while(1){
		 iter = strstr(start, "Connected to");
		 if(!iter){
			 break;
		 }
		 iter = strstr(iter, "http://");
		 iter += strlen("http://");
		 if (!iter){
			 break;
		 }
		 end = strstr(iter, "/");
		 if(!end){
			 break;
		 }
		 //copy host
		 if(end-iter < MAX_HOST){
			 strncpy(new->host, iter, end-iter);
		 }else{
			 strncpy(new->host, iter, MAX_HOST);

		 }
		 // find uri
		 iter = end + 1;
		 end = strchr(iter, '\n');
		 if(!end){
			 end = strchr(iter, '\0');
			 if(!end){
				 break;
			 }
		 }
		 //copy uri
		 if(end-iter < MAX_URI){
			 strncpy(new->uri, iter, end-iter);
		 }else{
			 strncpy(new->uri, iter, MAX_URI);
		 }

		 // add to list
		 new->next = new_uri_list();
		 new->next->prev = new;
		 new = new->next;
		 conn->has_connects += iter-start;
		 start = end;
	 }


	 //remove any trailing list item
	 if(strlen(new->uri) == 0){
		 uri_list *catch = new;
		 new = new->prev;
		 if(new){
			 new->next = NULL;
		 }
		 free(catch);
	 }
	 // go to front
	 if(new){
		 while(new->prev){
			 new = new->prev;
		 }
	 }
	 return new;
 }
