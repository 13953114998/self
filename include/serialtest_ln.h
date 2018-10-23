/*
 * serialtest_ln.h
 *
 *  Created on: Aug 8, 2017
 *      Author: nyb
 */
#ifndef __SERIALTEST_LN_H__
#define __SERIALTEST_LN_H__
#include "config.h"

void init_ln_serial_port();
S32 send_ln_serial_request(U8 ma, S32 cmd_type);

#endif /* #ifndef __SERIALTEST_LN_H__ */

