/*
 * tcp_ctrl_api.h
 *
 *  Created on: 2016年10月17日
 *      Author: leon
 */

#ifndef HEADER_TCP_CTRL_API_H_
#define HEADER_TCP_CTRL_API_H_


int get_the_client_connect_info();

/*
 * CONFERENCE
 */
int set_the_conference_parameters(int fd_value,int client_id,char client_seat,
		char* client_name,char* subj);

int get_the_conference_parameters(int fd_value);


int set_the_conference_vote_result();

/*
 * EVENT
 */
int set_the_event_parameter_power(int fd_value,unsigned char value);

int get_the_event_parameter_power(int fd_value);

int set_the_event_parameter_ssid_pwd(int fd_value,char* ssid,char* pwd);











#endif /* HEADER_TCP_CTRL_API_H_ */
