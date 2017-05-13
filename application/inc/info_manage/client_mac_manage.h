/*
 * client_mac_manage.h
 *
 *  Created on: 2017年3月7日
 *      Author: leon
 */

#ifndef INC_INFO_MANAGE_CLIENT_MAC_MANAGE_H_
#define INC_INFO_MANAGE_CLIENT_MAC_MANAGE_H_





/*
 * MAC映射表
 */
typedef struct {

	unsigned int mac_number;
	unsigned short mac_table;

}connect_mac_table,*Pconnect_mac_table;

int cmm_search_client_mac(Pclient_info info);

#endif /* INC_INFO_MANAGE_CLIENT_MAC_MANAGE_H_ */
