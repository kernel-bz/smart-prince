/**
 *  file name:  motor.c
 *	  comments:   motor Module for SmartPrince
 *  author:     JungJaeJoon(rgbi3307@nate.com)
 *  Copyright (c) www.kernel.bz
 *  This is under the GPL License (see <http://www.gnu.org/licenses/>)
 */

#include "devices/motor.h"
#include "common/config.h"

MotorControl_T MotorControl = {
    .act = MD_GO,
    .dir = MD_WAIT,
    .cnt = 0,
    .mode = CONFIG_MODE_RUN
};




