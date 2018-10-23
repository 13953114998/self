/*
 * ============================================================================
 *
 *       Filename:  lua_action.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年09月04日 09时02分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  first name surname (), 
 *   Organization:  
 *
 * ============================================================================
 */

#include "lua_action.h"
static void *l_alloc (void *ud, void *ptr, size_t osize,
	size_t nsize) {
	(void)ud;  (void)osize;  /*   not used */
	if (nsize == 0) {
		if (ptr)
			free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}

/*  Set PATH of LUA */
void set_lua_path()
{
	char lua_path[1024] = {0};
	char *p = getenv("LUA_PATH");
	snprintf(lua_path, 1024, "LUA_PATH=%s%s", MY_LUA_PATH, p);
	int ret = putenv(lua_path);
	if (ret == -1) {
		perror("setenv LUA_PATH error!\n");
		exit(-1);
	}
}

/* Open and load LUA file ,return lua_State */
static lua_State *lua_open_file(char *file)
{
	lua_State *luaenv;
	luaenv = lua_newstate(l_alloc, NULL);
	if (!luaenv) {
		printf("Creat lua file [%s] error!\n", file);
		return NULL;
	}
	luaL_openlibs(luaenv);

	int ret = luaL_loadfile(luaenv, file);
	if (ret) {
		perror("load file");
		printf("Lua load file [%s] failed!\n", file);
		return NULL;
	}
	return luaenv;
}

/* Close luaenv that open by function lua_open_file(char *file) */
void lua_close_luaenv(lua_State *luaenv)
{
	lua_close(luaenv);
}

/* Pare variable and return string */
static char* lua_variable_string(lua_State *luaenv, char *variable)
{
	lua_getglobal(luaenv, variable);
	if(lua_isnil(luaenv, -1)) {
		printf("%s is invaild\n", variable);
		return NULL;
	}
	size_t totallen;
	const char *buf = lua_tolstring(luaenv, -1, &totallen);
	if(!buf) {
		printf("Can not get value of: %s\n", variable);
		return NULL;
	}
	char *buffer = calloc(1, totallen + 1);
	if (!buffer) {
		printf("calloc buffer (size :[%d] failed!\n",
			(int)totallen + 1);
		return NULL;
	}
	strncpy(buffer, buf, totallen);
	lua_pop(luaenv, 1);
	return buffer;
	
}
/* LUA run file and return string */
char *lua_do_file(char *file)
{
	lua_State *luaenv = lua_open_file(file);
	if (!luaenv) {
		return NULL;
	}
	lua_pcall(luaenv, 0, 0, 0);
	char *ret_str = lua_variable_string(luaenv, "JSTR");
	lua_close_luaenv(luaenv);
	return ret_str;
}
/* 
 * Lua run file call function pass param and return luaenv;
 * If function hava return, function that call this need pare
 * and need close luaenv by self;
 * */
lua_State *lua_dofile_func(char *file, char *func, char *param)
{
//	printf("file:%s\n",file);
//	printf("func:%s\n",func);
//	printf("param:%s\n",param);
	lua_State *luaenv;
	luaenv = lua_open_file(file);
	if (!luaenv) {
		return NULL;
	}
	lua_pcall(luaenv, 0, 0, 0);
	lua_getglobal(luaenv, func);
	lua_pushlstring(luaenv, param, strlen(param));
	lua_call(luaenv, 1, LUA_MULTRET);
	return luaenv;
}
#if 0
int main ()
{
	set_lua_path();
	char *dofile_str = NULL;
	lua_State *luaenv = NULL;
	printf("do file+++++++++++++++++++++++++++++++++++++++++\n");
	dofile_str = lua_do_file(TEST_RUN_FILE);
	if (!dofile_str) {
		return -1;
	}
	printf("dofile_str == :%s\n", dofile_str);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
	/* ========================== */
	printf("do func+++++++++++++++++++++++++++++++++++++++++\n");
	luaenv = lua_dofile_func(TEST_RUN_FILE, "printf_string_retstr", dofile_str);
	if (!luaenv) {
		free(dofile_str);
		return -1;
	}
	const char* dofunc_str = lua_tostring(luaenv, -1);
	if (dofunc_str) {
		printf("dofunc_str:%s\n", dofunc_str);
	}
	lua_close_luaenv(luaenv);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
	/* ========================= */
	printf("do func+++++++++++++++++++++++++++++++++++++++++\n");
	luaenv = lua_dofile_func(TEST_RUN_FILE, "printf_string_retint", dofile_str);
	if (!luaenv) {
		return -1;
	}
	const int dofunc_int = lua_tointeger(luaenv, -1);
	printf("dofunc_int == %d\n", dofunc_int);
	lua_close_luaenv(luaenv);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
	free(dofile_str);
	dofile_str = NULL;
	return 0;
}
#endif
