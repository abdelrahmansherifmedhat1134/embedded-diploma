/*
 * TERM_cfg.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 */

#ifndef SERVICE_TERM_TERM_CFG_H_
#define SERVICE_TERM_TERM_CFG_H_

/* bytes each ring can really hold (the storage arrays are one byte bigger , see RINGBUF) ; max 254 */
#define TERM_RX_BUFFER_SIZE    32
#define TERM_TX_BUFFER_SIZE    128
#define TERM_LINE_MAX          16                      /* typed characters per line */
#define TERM_ECHO_DEFAULT      TERM_ECHO_NORMAL        /* TERM_ECHO_OFF for phone apps that echo locally */

#endif /* SERVICE_TERM_TERM_CFG_H_ */
