/*
 * =====================================================================================
 *
 *       Filename:  lua_action.h
 *        
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年09月04日 13时23分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xiaojian jiang (Adams), linfeng2010jxj@163.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __LUA_ACTION_H__
#define __LUA_ACTION_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <error.h>
#include <errno.h>

#define MY_LUA_PATH "/usr/lib/lua/?.lua;;"
#define GET_STATE_LUA_FILE "/usr/lib/lua/get_status.lua"
#define SET_STATE_LUA_FILE "/usr/lib/lua/set_status.lua"

/* Set PATH of LUA */
void set_lua_path();
/* Close luaenv that open by function lua_open_file(char *file) */
void lua_close_luaenv(lua_State *luaenv);
/* LUA run file and return string */
char *lua_do_file(char *file);
/* 
 * Lua run file call function pass param and return luaenv;
 * If function hava return, function that call this need pare
 * and need close luaenv by self;
 * */
lua_State *lua_dofile_func(char *file, char *func, char *param);

#endif /* #ifndef __LUA_ACTION_H__ */
