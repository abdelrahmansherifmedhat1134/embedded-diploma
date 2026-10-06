/*
 * UIREM_cfg.h
 *
 *  Created on: Oct 4, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef APP_UIREM_UIREM_CFG_H_
#define APP_UIREM_UIREM_CFG_H_

#define UIREM_IDLE_TIMEOUT_S        120                /* seconds without a received byte -> logout (D-11 , max 255) */
/* free TX bytes needed before a step prints : every line of the protocol (with its "\r\n") is shorter */
#define UIREM_TX_RESERVE            64
#define UIREM_MENU_AFTER_COMMAND    0                  /* 1 = reprint the whole menu after every command (D-8) */

#endif /* APP_UIREM_UIREM_CFG_H_ */
