/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年02月23日 14时44分42秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "cJSON.h"

//gcc -o test json_sample.c ../lib/cJSON.c -I../include/ -lm

int main ()
{
	/* --------------------------pare----------------------------- */
	char *json_str= "{\"code\":\"4\",\"seckey\":\"OW2XK3fb_kV0H5QgNZCLsTMjE6L0YiqeeUp26RdlxXU\"}";
	cJSON *root = cJSON_Parse(json_str);
	if(!root) {
		printf("get root faild !\n");
		return -1;
	}
	cJSON *code = cJSON_GetObjectItem(root, "code");
	if (!code) {
		printf("no code key!\n");
		return -1;
	}
	printf("type:[%d]\n", code->type);
	printf(" key:[%s]\n", code->string);
	printf(" val:[%s]\n", code->valuestring);

	cJSON *seckey = cJSON_GetObjectItem(root, "seckey");
	if (!seckey) {
		printf("no seckey key!\n");
		return -1;
	}
	printf("type:[%d]\n", seckey->type);
	printf(" key:[%s]\n", seckey->string);
	printf(" val:[%s]\n", seckey->valuestring);
	cJSON_Delete(root);
	/* ----------------------------assemble--------------------------------- */
	cJSON *object_w = cJSON_CreateObject();
	cJSON *object = cJSON_CreateObject();
	cJSON *array=cJSON_CreateArray();
	cJSON_AddStringToObject(object, "name", "wifi");
	cJSON_AddStringToObject(object, "accessid", "C3LZDGVTMTQYMJQZMZGWMDEXOA==");
	cJSON_AddStringToObject(object, "aid", "5958295805");
	cJSON_AddStringToObject(object, "did", "q0001");

	cJSON_AddItemToArray(array, object);
	cJSON_AddItemToObjectCS(object_w, "clients", array);
	char *rendered=cJSON_Print(object_w);
	printf("json to string format:%s\n",rendered);
	char renderedx = cJSON_PrintUnformatted(object_w);
	printf("json to string noformat:%s\n",renderedx);
	//if (rendered)
	//free(rendered);
	cJSON_DeleteItemFromObject(object_w, "clients");
	//cJSON_Delete(array);
	rendered=cJSON_Print(object_w);
	printf("json to string:%s\n",rendered);

	return 0;
}
