/*
 * graph-explore.h
 *
 *  Created on: Dec 16, 2016
 *      Author: awilde
 */

#ifndef GRAPH_EXPLORE_H_
#define GRAPH_EXPLORE_H_

#define MAX_ATTRIBUTE 25
#define MAX_VALUE 25
#define MAX_HOST 50
#define MAX_URI 50
#define TABLE_SIZE 500000

typedef struct search{
	char attr[MAX_ATTRIBUTE];
	char value[MAX_VALUE];
}search;

typedef struct uri_list{
	char host[MAX_HOST];
	char uri[MAX_URI];
	struct uri_list *next;
	struct uri_list *prev;
}uri_list;

uri_list* new_uri_list();


#endif /* GRAPH_EXPLORE_H_ */
