/*
 * tcp_ctrl_api.h
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_API_H_
#define HEADER_TCP_CTRL_API_H_


int scanf_client();


int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj);

int get_the_conference_parameters(int fd_value);


int set_the_conference_vote_result();


#endif /* HEADER_TCP_CTRL_API_H_ */
