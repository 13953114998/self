/*
 * serialtest_sh.h
 *
 *  Created on: Aug 8, 2017
 *      Author: nyb
 */
#include "config.h"
#ifdef SYS86
#ifndef __SERIALTEST_SH_H__
#define __SERIALTEST_SH_H__

S32 send_sh_serial_request(U8 ma, S32 cmd_type);
void init_sh_serial_switch();

#endif /* #ifndef __SERIALTEST_SH_H__ */
#endif /* SYS86 */

