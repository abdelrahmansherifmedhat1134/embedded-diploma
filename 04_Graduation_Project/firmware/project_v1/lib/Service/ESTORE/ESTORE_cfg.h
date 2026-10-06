/*
 * ESTORE_cfg.h
 *
 *  Created on: Oct 3, 2026
 *      Author: Abdelrahman Sherif Medhat
 *
 *  The EEPROM map as code (REQ-EEP-02). Explained in docs/eeprom_map.md.
 */

#ifndef SERVICE_ESTORE_ESTORE_CFG_H_
#define SERVICE_ESTORE_ESTORE_CFG_H_

/* chip area used : pages 0..12 of block 0 */
#define ESTORE_PAGE_SIZE             16
#define ESTORE_PAGE_COUNT            13
#define ESTORE_IMAGE_SIZE            (ESTORE_PAGE_SIZE * ESTORE_PAGE_COUNT)     /* 208 = 0xD0 */

/* page 0 : header (written on first boot only) */
#define ESTORE_ADDR_MAGIC            0x00
#define ESTORE_ADDR_VERSION          0x01
#define ESTORE_MAGIC_VALUE           0xA5
#define ESTORE_VERSION_VALUE         0x01

/* page 1 : settings */
#define ESTORE_ADDR_HEATER_SET       0x10

/* page 2 : admin record , pages 3..7 : remote users , pages 8..12 : keypad users */
#define ESTORE_ADDR_ADMIN            0x20
#define ESTORE_ADDR_REMOTE_USERS     0x30
#define ESTORE_ADDR_KEYPAD_USERS     0x80

/* account record : one page */
#define USERDB_RECORD_SIZE           16
#define USERDB_NAME_OFFSET           0
#define USERDB_NAME_SIZE             8
#define USERDB_PASS_OFFSET           8
#define USERDB_PASS_SIZE             8
#define USERDB_REMOTE_SLOTS          5
#define USERDB_KEYPAD_SLOTS          5

/* factory defaults (REQ-SEC-09 , REQ-HTR-03) */
#define ESTORE_DEFAULT_HEATER_SET    60
#define ESTORE_DEFAULT_ADMIN_NAME    "admin"
#define ESTORE_DEFAULT_ADMIN_PASS    "1234"

/* write-behind */
#define ESTORE_POLL_LIMIT            5       /* 5 x 10 ms : longest wait for one write cycle */

#endif /* SERVICE_ESTORE_ESTORE_CFG_H_ */
