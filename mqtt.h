/*
 * =====================================================================================
 *
 *       Filename:  mqtt.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2018 08:42:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yanfu cheng (Hand), hand0317_cyf@163.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _MQTT_H_
#define _MQTT_H_

void analysis_after_data(char *mqtt_buf);
void making_dev_message(int type, dev_node_t *node);
void mosquitto_set(struct mosquitto *mosq);
void my_connect_callback(struct mosquitto *mosq, void *userdata, int result);
void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);


#endif
