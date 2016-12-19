/*
 ============================================================================
 Name        : graph-explore.c
 Author      : A W
 Version     : 1.0
 ============================================================================
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "graph-explore.h"
#include "connect.h"
#include "parser.h"
#include "timer.h"
#include "murmur-hash.h"

char* init_host;
static const char* port = PORT;
static const char* user = USER;




typedef struct id_bucket{
	struct uri_list *list;
	int size;
}id_bucket;

id_bucket con_table[TABLE_SIZE];

double elapsed_base = 0;

// usage()
//    Explain how graph-explore should be run.
static void usage(void) {
	fprintf(stderr, "Usage: ./graph-explore [-h HOST] [-p PORT] [-u URI] [-a ATTRIBUTE] [-v VALUE] \n");
	exit(1);
}


/************************************
 * Helper functions for 			*
 *	handling the data structs		*
 *	of main							*
 * 									*
 * 	Found : list of matched	items	*
 *  Queue : list of unvisited 		*
 *  		nodes					*
 *  								*
 ************************************/


// generates a new list item
uri_list *new_uri_list(){
	uri_list *new = malloc(sizeof(struct uri_list));
	memset(new, 0, sizeof(uri_list));
	return new;
}

// Add new list item to the
// end of the list
uri_list *push_queue(uri_list *item, uri_list *queue){
	// find the end

	if(!queue){
		queue = item;
		return queue;
	}
	if(strlen(queue->uri)==0){
		strcpy(queue->host, item->host);
		strcpy(queue->uri, item->uri);
		return queue;
	}
	while(queue->next){
		queue = queue->next;
	}
	queue->next = item;
	queue->next->prev = queue;
	queue->next->next = NULL;
	return queue;
}

// Removes list_item from front of list
uri_list *pop_queue(uri_list *queue){
		// find the end
		uri_list *popped;
		popped = queue;
		return popped;
}

// Searches for list_item in a list
int find_in_list(uri_list *item, uri_list *list){
	int found = 0;
	while(list){
		if(strcmp(item->host, list->host)== 0 && strcmp(item->uri, list->uri) == 0){
			found = 1;
			break;
		}
		list = list->next;
	}
	return found;
}

// Tokenizes the host/uri string
int hash(char *uri, char *host){
	uint32_t r = (uint32_t) rand();
	char *new = malloc(sizeof(char)*(strlen(uri)+strlen(host))+2);
	strcpy(new,host);
	new = strcat(new, "/");
	new = strcat(new, uri);
	return (unsigned int) murmur3_32((uint8_t *) new, strlen(new), r) % TABLE_SIZE;
}

// Checks to see if the token exists the hash table
// If it does, it has been visited already
int check_visited(uri_list *uri){
	unsigned int i = hash(uri->uri, uri->host);
	int v = 0;
	if(!con_table[i].list){
		con_table[i].list = uri;
		return v ;
	}else{
		v = find_in_list(uri, con_table[i].list);
		if(!v){
			uri->next = con_table[i].list;
			if(uri->next){
				uri->next->prev = uri;
			}
			uri->prev = NULL;
			con_table[i].list = uri;
		}
	}
		return v;
}


// Add list item to the front of the list
void add_found(char *uri, char *host, uri_list *list){
	if (strlen(list->uri) == 0){
		strcpy(list->uri, uri);
		strcpy(list->host, host);
	}else{
		uri_list *new = new_uri_list();
		strcpy(new->uri, uri);
		strcpy(new->host, host);
		list->next = new;
		new->prev = list;
	}
}


// Add unvisited host/uri pairs to the end of the queue
uri_list *add_connects(http_connection *conn, uri_list *queue){
	uri_list *list =parse_body(conn);
	uri_list *iter = list;
	if(!queue){
		queue = list;
		return queue;
	}
	while(iter){
		int v = check_visited(iter);
		if(!v){
			queue = push_queue(list, queue);
		}
		iter = iter->next;
	}
	return queue;

}
/**
 * Launches the graph-explorer.
 *
 * The following describes the arguments to the program:
 * graph-explorer [-h HOST] [-p PORT] [-u URI] [-a ATTRIBUTE] [-v VALUE]
 *
 * -h : the host of the file
 *
 * -p : the port for connecting
 *
 * -u : the uri of the object
 *
 * -a : the attribute searched for
 *
 * -v : the value of the attribute searched for
 *
 .
 */

int main(int argc, char **argv) {

	// parse arguments
	int ch = 0;
	char *uri;
	search *search = malloc(sizeof(struct search));
	memset(search, 0, sizeof(struct search));
	uri = "file_one.txt";
	init_host = "localhost";
	while ((ch = getopt(argc, argv, "h:p:u:a:v:")) != -1) {
		switch(ch){
		case 'h':
			init_host = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'u':
			uri = optarg;
			break;
		case 'a':
			strncpy(search->attr, optarg, MAX_ATTRIBUTE);
			break;
		case 'v':
			strncpy(search->value, optarg, MAX_VALUE);
			break;
		default:
			usage();
		}
	}
	if (optind == argc - 1)
		user = argv[optind];
	else if (optind != argc)
		usage();
	struct addrinfo* addr;

	// look up network address of server
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	int r = getaddrinfo(init_host, port, &hints, &addr);
	if (r != 0) {
		fprintf(stderr, "problem looking up %s: %s\n",
				init_host, gai_strerror(r));
		exit(1);
	}

	elapsed_base = timestamp();

	// initialize global synchronization objects

	//initialize global found uri_list
	struct uri_list *found = malloc(sizeof(struct uri_list));
	memset(found, 0, sizeof(uri_list));

	struct uri_list *queue = malloc(sizeof(struct uri_list));
	memset(queue, 0, sizeof(uri_list));

	// explore graph

	//fetch from initial page
	http_connection* conn = http_connect(addr);
	http_send_request(conn, uri, init_host);
	http_receive_response_headers(conn);

	if (conn->status_code != 200) {
		fprintf(stderr, "bad response to \"%s\" RPC: %d %s\n", uri,
				conn->status_code, http_truncate_response(conn));
		http_close(conn);
		exit(1);
	}
	http_receive_response_body(conn, search);
	if(conn->match){
		add_found(uri, init_host, found);
	}
	if(conn->has_connects){
		queue = add_connects(conn, queue);
	}
	http_close(conn);

	while (queue) {
		uri_list *fetch = pop_queue(queue);
		if (!fetch){
			queue = fetch;
			break;
		}
		queue = queue->next;
		r = getaddrinfo(fetch->host, port, &hints, &addr);
		if (r != 0) {
			fprintf(stderr, "problem looking up %s: %s\n",
					init_host, gai_strerror(r));
			exit(1);
		}


		//fetch from initial page
		http_connection *n_conn = http_connect(addr);
		http_send_request(n_conn, fetch->uri, fetch->host);
		http_receive_response_headers(n_conn);

		if (n_conn->status_code != 200) {
			fprintf(stderr, "bad response to \"%s\" RPC: %d %s\n", uri,
					n_conn->status_code, http_truncate_response(n_conn));
			http_close(n_conn);
			exit(1);
		}
		http_receive_response_body(n_conn, search);
		if(n_conn->match){
			add_found(fetch->uri, fetch->host, found);
		}
		if(n_conn->has_connects){
			queue = add_connects(n_conn, queue);
		}
		http_close(n_conn);



	}

	while(found){
		fprintf(stdout, "http://%s/%s\n",found->host, found->uri);
		found = found->next;
	}

}
