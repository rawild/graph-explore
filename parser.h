/*
 * parser.h
 *
 *  Created on: Dec 16, 2016
 *      Author: awilde
 */

#ifndef PARSER_H_
#define PARSER_H_
#include "connect.h"
#include "graph-explore.h"

int process_response_headers(http_connection* conn);
int check_response_body(http_connection* conn, struct search *search);
uri_list *parse_body(http_connection *conn);

#endif /* PARSER_H_ */
