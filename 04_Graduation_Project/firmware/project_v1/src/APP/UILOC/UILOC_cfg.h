/*
 * UILOC_cfg.h
 *
 *  Created on: Oct 4, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_UILOC_UILOC_CFG_H_
#define APP_UILOC_UILOC_CFG_H_

#define UILOC_IDLE_TIMEOUT_S     30                    /* seconds without a key -> logout , status screen (Section 12 #8) */
#define UILOC_MESSAGE_TIME       200                   /* x 10 ms = 2 s : how long a message stays (max 255) */
#define UILOC_STATUS_PAGE_S      3                     /* seconds per status page (REQ-LUI-03) */

/* keys of the fitted calculator keypad (D-7) */
#define UILOC_KEY_ENTER          '='
#define UILOC_KEY_BACK1          '*'
#define UILOC_KEY_BACK2          'C'
#define UILOC_KEY_UP             '+'
#define UILOC_KEY_DOWN           '-'

#endif /* APP_UILOC_UILOC_CFG_H_ */
