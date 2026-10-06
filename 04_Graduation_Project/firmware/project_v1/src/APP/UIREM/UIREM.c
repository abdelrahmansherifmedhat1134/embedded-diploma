/*
 * UIREM.c
 *
 *  Created on: Oct 4, 2026
 *      Author: Abdelrahman Sherif Medhat
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/flash_str.h"
#include "../../../lib/Service/FMT/FMT.h"
#include "../../../lib/Service/TERM/TERM.h"
#include "../../../lib/Service/TERM/TERM_cfg.h"
#include "../../../lib/Service/EVQ/EVQ.h"
#include "../../../lib/Service/ESTORE/ESTORE.h"
#include "../../../lib/Service/ESTORE/ESTORE_cfg.h"
#include "../../../lib/Service/USERDB/USERDB.h"
#include "../../../lib/Service/USERDB/USERDB_cfg.h"
#include "../../../lib/HAL/LAMP/LAMP.h"
#include "../../../lib/HAL/LAMP/LAMP_cfg.h"
#include "../SEC/SEC.h"
#include "../ALARM/ALARM.h"
#include "../LIGHT/LIGHT.h"
#include "../LIGHT/LIGHT_cfg.h"
#include "../DOOR/DOOR.h"
#include "../CLIMATE/CLIMATE.h"
#include "../HEATER/HEATER.h"
#include "../HEATER/HEATER_cfg.h"
#include "UIREM.h"
#include "UIREM_cfg.h"

/* states (architecture.md 5.8). BANNER and MENU_PRINT of the design are not states here :
 * they are messages waiting in the print queue */
#define UIREM_STATE_ASK_NAME     0
#define UIREM_STATE_ASK_PASS     1
#define UIREM_STATE_PROMPT       2
#define UIREM_STATE_ARG          3                     /* waits for the argument of command 1 , 2 , 5 , 11 or 13 */
#define UIREM_STATE_ADD_NAME     4
#define UIREM_STATE_ADD_PASS     5
#define UIREM_STATE_NEW_PASS     6
#define UIREM_STATE_CONFIRM      7
#define UIREM_STATE_LOCKED       8

/* commands = menu numbers (uart_protocol.md Section 6) */
#define UIREM_CMD_LOGOUT         0
#define UIREM_CMD_LAMP           1
#define UIREM_CMD_DIMMER         2
#define UIREM_CMD_AC             3
#define UIREM_CMD_HEATER         4
#define UIREM_CMD_SET_TEMP       5
#define UIREM_CMD_STATUS         6
#define UIREM_CMD_DOOR_OPEN      7                     /* 7..16 : admin only (REQ-SEC-06) */
#define UIREM_CMD_DOOR_CLOSE     8
#define UIREM_CMD_KEYPAD         9
#define UIREM_CMD_ADD_REMOTE     10
#define UIREM_CMD_DEL_REMOTE     11
#define UIREM_CMD_ADD_KEYPAD     12
#define UIREM_CMD_DEL_KEYPAD     13
#define UIREM_CMD_LIST           14
#define UIREM_CMD_ADMIN_PASS     15
#define UIREM_CMD_RESET          16
#define UIREM_CMD_FIRST_ADMIN    UIREM_CMD_DOOR_OPEN
#define UIREM_CMD_LAST           UIREM_CMD_RESET

/* messages : what the print queue holds. One message = one line , except MENU , STATUS and USERS
 * (one line per step) */
#define UIREM_MSG_BANNER             1
#define UIREM_MSG_ASK_NAME           2
#define UIREM_MSG_ASK_PASS           3
#define UIREM_MSG_WARN_STORAGE       4
#define UIREM_MSG_ALERT              5
#define UIREM_MSG_ERR_UNKNOWN        6
#define UIREM_MSG_ERR_ROLE           7
#define UIREM_MSG_ERR_TOO_LONG       8
#define UIREM_MSG_ERR_LAMP           9
#define UIREM_MSG_ERR_DIMMER         10
#define UIREM_MSG_ERR_TEMP           11
#define UIREM_MSG_ERR_NAME           12
#define UIREM_MSG_ERR_PASS           13
#define UIREM_MSG_ERR_EXISTS         14
#define UIREM_MSG_ERR_FULL           15
#define UIREM_MSG_ERR_DIGITS         16
#define UIREM_MSG_ERR_NOT_FOUND      17
#define UIREM_MSG_ERR_DEVICE         18
#define UIREM_MSG_ASK_LAMP           19
#define UIREM_MSG_ASK_DIMMER         20
#define UIREM_MSG_ASK_TEMP           21
#define UIREM_MSG_ASK_REMOTE_NAME    22
#define UIREM_MSG_ASK_REMOTE_PASS    23
#define UIREM_MSG_ASK_KEYPAD_ID      24
#define UIREM_MSG_ASK_KEYPAD_PIN     25
#define UIREM_MSG_ASK_DEL_REMOTE     26
#define UIREM_MSG_ASK_DEL_KEYPAD     27
#define UIREM_MSG_ASK_ADMIN_PASS     28
#define UIREM_MSG_ASK_CONFIRM        29
#define UIREM_MSG_OK_ADMIN_PASS      30
#define UIREM_MSG_OK_RESET           31
#define UIREM_MSG_CANCELLED          32
/* lines with a number , a name or a state in them */
#define UIREM_MSG_WELCOME            40
#define UIREM_MSG_PROMPT             41
#define UIREM_MSG_ERR_LOGIN          42
#define UIREM_MSG_OK_LAMP            43
#define UIREM_MSG_OK_DIMMER          44
#define UIREM_MSG_OK_TEMP            45
#define UIREM_MSG_DOOR               46
#define UIREM_MSG_OK_KEYPAD          47
#define UIREM_MSG_OK_ADDED           48
#define UIREM_MSG_OK_REMOVED         49
#define UIREM_MSG_GOODBYE            50
#define UIREM_MSG_AC                 51
#define UIREM_MSG_HEATER             52
#define UIREM_MSG_EVENT              53
#define UIREM_MSG_TIMEOUT            54
/* more than one line */
#define UIREM_MSG_MENU               60
#define UIREM_MSG_STATUS             61
#define UIREM_MSG_USERS              62

/* the longest chain is : welcome , storage warning , menu , prompt (+ an event and its prompt) */
#define UIREM_QUEUE_SIZE             6

/* menu items : 0 = header , 1..16 = commands , then logout and '?' */
#define UIREM_MENU_ITEM_LOGOUT       17
#define UIREM_MENU_ITEM_HELP         18
#define UIREM_MENU_ADMIN_ITEMS       (UIREM_CMD_LAST - UIREM_CMD_FIRST_ADMIN + 1)
/* status block : step 5 (door) is the last line for a user , step 6 (keypad) for the admin */
#define UIREM_STATUS_STEP_DOOR       5

static u8 Global_u8State = UIREM_STATE_ASK_NAME ;
static u8 Global_u8Command = 0 ;                       /* command that waits for its argument / name / password */
static u8 Global_u8Arg = 0 ;                           /* number for the reply line (lamp , list , "door moved") */
static u8 Global_u8Event = 0 ;                         /* event being printed */
static u8 Global_u8EventArg = 0 ;
static u8 Global_u8IdleSeconds = 0 ;
static u8 Global_u8AtPrompt = 0 ;                      /* 1 = the cursor stands behind "name> " , not at a line start */
/* print queue : messages still to print , and the step inside the first one */
static u8 Global_u8Queue[UIREM_QUEUE_SIZE] ;
static u8 Global_u8QueueCount = 0 ;
static u8 Global_u8Step = 0 ;
/* the typed user name : a whole line , so a too long name can simply fail the login */
static c8 Global_c8Name[TERM_LINE_MAX + 1] ;
/* the account being added or removed (already checked : 8 characters at most) */
static c8 Global_c8Target[USERDB_NAME_TEXT_SIZE] ;

/**************************** small helpers ****************************/
static void UIREM_voidQueue(u8 Copy_u8Message){
	if(Global_u8QueueCount < UIREM_QUEUE_SIZE){
		Global_u8Queue[Global_u8QueueCount++] = Copy_u8Message ;
	}else{
		/* full : the message is dropped (can not happen with the chains used below) */
	}
}
/* password prompts : echo '*' (REQ-RUI-01). A terminal that echoes by itself (TERM_ECHO_OFF) stays off */
static void UIREM_voidSetMasked(u8 Copy_u8State){
	if(TERM_ECHO_DEFAULT == TERM_ECHO_OFF){
		TERM_voidSetEcho(TERM_ECHO_OFF);
	}else if(Copy_u8State == 1){
		TERM_voidSetEcho(TERM_ECHO_MASKED);
	}else{
		TERM_voidSetEcho(TERM_ECHO_NORMAL);
	}
}
static u8 UIREM_u8TextLength(const c8 * Copy_pc8Text){
	u8 Local_u8Length = 0 ;
	while(Copy_pc8Text[Local_u8Length] != '\0'){
		Local_u8Length++;
	}
	return Local_u8Length ;
}
static u8 UIREM_u8IsDigits(const c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		if((*Copy_pc8Text < '0') || (*Copy_pc8Text > '9')){
			return 0 ;
		}
		Copy_pc8Text++;
	}
	return 1 ;
}
static u8 UIREM_u8HasSpace(const c8 * Copy_pc8Text){
	while(*Copy_pc8Text != '\0'){
		if(*Copy_pc8Text == ' '){
			return 1 ;
		}
		Copy_pc8Text++;
	}
	return 0 ;
}
static u8 UIREM_u8IsEqual(const c8 * Copy_pc8Text1, const c8 * Copy_pc8Text2){
	while((*Copy_pc8Text1 != '\0') && (*Copy_pc8Text1 == *Copy_pc8Text2)){
		Copy_pc8Text1++;
		Copy_pc8Text2++;
	}
	return (*Copy_pc8Text1 == *Copy_pc8Text2) ? 1 : 0 ;
}
/* copies at most Copy_u8Size - 1 characters and always ends with '\0' */
static void UIREM_voidCopy(c8 * Copy_pc8To, const c8 * Copy_pc8From, u8 Copy_u8Size){
	u8 i = 0 ;
	while((Copy_pc8From[i] != '\0') && (i < (Copy_u8Size - 1))){
		Copy_pc8To[i] = Copy_pc8From[i] ;
		i++;
	}
	Copy_pc8To[i] = '\0' ;
}
static void UIREM_voidDropEvents(){
	u8 Local_u8Event ;
	u8 Local_u8Arg ;
	while(EVQ_u8Get(&Local_u8Event,&Local_u8Arg) == 1){
	}
}
/* number of used slots in one list */
static u8 UIREM_u8CountUsers(u8 Copy_u8List, u8 Copy_u8Slots){
	c8 Local_c8Name[USERDB_NAME_TEXT_SIZE] ;
	u8 Local_u8Count = 0 ;
	u8 i ;
	for(i = 0 ; i < Copy_u8Slots ; i++){
		Local_u8Count += USERDB_u8GetUserName(Copy_u8List,i,Local_c8Name);
	}
	return Local_u8Count ;
}
static u8 UIREM_u8UserExists(u8 Copy_u8List, u8 Copy_u8Slots, const c8 * Copy_pc8Name){
	c8 Local_c8Name[USERDB_NAME_TEXT_SIZE] ;
	u8 i ;
	for(i = 0 ; i < Copy_u8Slots ; i++){
		if((USERDB_u8GetUserName(Copy_u8List,i,Local_c8Name) == 1) && (UIREM_u8IsEqual(Local_c8Name,Copy_pc8Name) == 1)){
			return 1 ;
		}
	}
	return 0 ;
}

/**************************** print helpers ****************************/
/* a line that was not asked for (event , timeout , alert) starts on a line of its own */
static void UIREM_voidFreshLine(){
	if((Global_u8AtPrompt == 1) || (TERM_u8IsLineEmpty() == 0)){
		TERM_voidNewLine();
	}
	Global_u8AtPrompt = 0 ;
}
static void UIREM_voidPutOnOff(u8 Copy_u8State){
	TERM_voidPutFlash((Copy_u8State != 0) ? FLASH_STR("ON") : FLASH_STR("OFF"));
}
static void UIREM_voidPutElement(u8 Copy_u8Element){
	switch(Copy_u8Element){
	case HEATER_ELEMENT_HEATING: TERM_voidPutFlash(FLASH_STR("HEATING")); break ;
	case HEATER_ELEMENT_COOLING: TERM_voidPutFlash(FLASH_STR("COOLING")); break ;
	default:                     TERM_voidPutFlash(FLASH_STR("IDLE"));    break ;
	}
}
/* "OFF" or "ON (HEATING)" */
static void UIREM_voidPutHeaterState(){
	if(HEATER_u8IsOn() == 0){
		TERM_voidPutFlash(FLASH_STR("OFF"));
	}else{
		TERM_voidPutFlash(FLASH_STR("ON ("));
		UIREM_voidPutElement(HEATER_u8GetElement());
		TERM_voidPutChar(')');
	}
}
/* the answer to the terminal's own command starts with [OK] , the same change made elsewhere with [INFO] */
static void UIREM_voidPutPrefix(u8 Copy_u8Message){
	TERM_voidPutFlash((Copy_u8Message == UIREM_MSG_EVENT) ? FLASH_STR("[INFO] ") : FLASH_STR("[OK] "));
}
static void UIREM_voidPutLamp(u8 Copy_u8Message, u8 Copy_u8Lamp){
	UIREM_voidPutPrefix(Copy_u8Message);
	TERM_voidPutFlash(FLASH_STR("Lamp "));
	TERM_voidPutNumber(Copy_u8Lamp);
	TERM_voidPutFlash(FLASH_STR(" is now "));
	UIREM_voidPutOnOff(LIGHT_u8GetLamp(Copy_u8Lamp) == LAMP_ON);
}
static void UIREM_voidPutDimmer(u8 Copy_u8Message){
	UIREM_voidPutPrefix(Copy_u8Message);
	TERM_voidPutFlash(FLASH_STR("Dimmer is now "));
	TERM_voidPutNumber(LIGHT_u8GetDimmer());
	TERM_voidPutChar('%');
}
static void UIREM_voidPutSetTemp(u8 Copy_u8Message){
	UIREM_voidPutPrefix(Copy_u8Message);
	TERM_voidPutFlash(FLASH_STR("Heater set temperature is now "));
	TERM_voidPutNumber(HEATER_u8GetSetTemp());
	TERM_voidPutChar('C');
}

/* The lines that never change (uart_protocol.md , verbatim). Returns 0 if the message is not one of them.
 * Every case prints by itself : a switch that only returns a pointer is turned into a
 * pointer table in RAM by the compiler (64 bytes). */
static u8 UIREM_u8PutFixedText(u8 Copy_u8Message){
	switch(Copy_u8Message){
	case UIREM_MSG_BANNER:          TERM_voidPutFlash(FLASH_STR("==== SMART HOME + WATER HEATER ====")); break ;
	case UIREM_MSG_ASK_NAME:        TERM_voidPutFlash(FLASH_STR("Hey, please enter your username:")); break ;
	case UIREM_MSG_ASK_PASS:        TERM_voidPutFlash(FLASH_STR("Please enter your password:")); break ;
	case UIREM_MSG_WARN_STORAGE:    TERM_voidPutFlash(FLASH_STR("[WARN] EEPROM not responding. Changes will not be saved.")); break ;
	case UIREM_MSG_ALERT:           TERM_voidPutFlash(FLASH_STR("[ALERT] 3 wrong logins. SYSTEM LOCKED. Reset to restart.")); break ;
	case UIREM_MSG_ERR_UNKNOWN:     TERM_voidPutFlash(FLASH_STR("[ERR] Unknown command.")); break ;
	case UIREM_MSG_ERR_ROLE:        TERM_voidPutFlash(FLASH_STR("[ERR] Not allowed for your role.")); break ;
	case UIREM_MSG_ERR_TOO_LONG:    TERM_voidPutFlash(FLASH_STR("[ERR] Input too long.")); break ;
	case UIREM_MSG_ERR_LAMP:        TERM_voidPutFlash(FLASH_STR("[ERR] Lamp number must be 1 to 5.")); break ;
	case UIREM_MSG_ERR_DIMMER:      TERM_voidPutFlash(FLASH_STR("[ERR] Dimmer level must be 0 to 100 in steps of 10.")); break ;
	case UIREM_MSG_ERR_TEMP:        TERM_voidPutFlash(FLASH_STR("[ERR] Temperature must be 35 to 75 in steps of 5.")); break ;
	case UIREM_MSG_ERR_NAME:        TERM_voidPutFlash(FLASH_STR("[ERR] Username must be 1 to 8 characters, no spaces.")); break ;
	case UIREM_MSG_ERR_PASS:        TERM_voidPutFlash(FLASH_STR("[ERR] Password must be 4 to 8 characters.")); break ;
	case UIREM_MSG_ERR_EXISTS:      TERM_voidPutFlash(FLASH_STR("[ERR] This username already exists.")); break ;
	case UIREM_MSG_ERR_FULL:        TERM_voidPutFlash(FLASH_STR("[ERR] The user list is full (5).")); break ;
	case UIREM_MSG_ERR_DIGITS:      TERM_voidPutFlash(FLASH_STR("[ERR] Keypad users need digits only.")); break ;
	case UIREM_MSG_ERR_NOT_FOUND:   TERM_voidPutFlash(FLASH_STR("[ERR] User not found.")); break ;
	case UIREM_MSG_ERR_DEVICE:      TERM_voidPutFlash(FLASH_STR("[ERR] Device not responding.")); break ;
	case UIREM_MSG_ASK_LAMP:        TERM_voidPutFlash(FLASH_STR("Lamp number (1-5):")); break ;
	case UIREM_MSG_ASK_DIMMER:      TERM_voidPutFlash(FLASH_STR("Dimmer level in % (0-100, step 10):")); break ;
	case UIREM_MSG_ASK_TEMP:        TERM_voidPutFlash(FLASH_STR("Set temperature (35-75, step 5):")); break ;
	case UIREM_MSG_ASK_REMOTE_NAME: TERM_voidPutFlash(FLASH_STR("New remote username (1-8 characters):")); break ;
	case UIREM_MSG_ASK_REMOTE_PASS: TERM_voidPutFlash(FLASH_STR("Password (4-8 characters):")); break ;
	case UIREM_MSG_ASK_KEYPAD_ID:   TERM_voidPutFlash(FLASH_STR("New keypad ID (1-8 digits):")); break ;
	case UIREM_MSG_ASK_KEYPAD_PIN:  TERM_voidPutFlash(FLASH_STR("PIN (4-8 digits):")); break ;
	case UIREM_MSG_ASK_DEL_REMOTE:  TERM_voidPutFlash(FLASH_STR("Remote username to remove:")); break ;
	case UIREM_MSG_ASK_DEL_KEYPAD:  TERM_voidPutFlash(FLASH_STR("Keypad ID to remove:")); break ;
	case UIREM_MSG_ASK_ADMIN_PASS:  TERM_voidPutFlash(FLASH_STR("New admin password (4-8 characters):")); break ;
	case UIREM_MSG_ASK_CONFIRM:     TERM_voidPutFlash(FLASH_STR("This erases all users and settings. Type YES to confirm:")); break ;
	case UIREM_MSG_OK_ADMIN_PASS:   TERM_voidPutFlash(FLASH_STR("[OK] Admin password changed.")); break ;
	case UIREM_MSG_OK_RESET:        TERM_voidPutFlash(FLASH_STR("[OK] Factory reset done. Admin is admin / 1234.")); break ;
	case UIREM_MSG_CANCELLED:       TERM_voidPutFlash(FLASH_STR("Cancelled.")); break ;
	default:                        return 0 ;
	}
	return 1 ;
}

/* one menu line per step (REQ-RUI-03) ; returns 1 after the last line */
static u8 UIREM_u8PrintMenu(u8 Copy_u8Step){
	u8 Local_u8Item = Copy_u8Step ;
	/* the user menu has no items 7..16 : after item 6 comes "logout" */
	if((SEC_u8GetRole(SEC_SOURCE_REMOTE) != SEC_ROLE_ADMIN) && (Local_u8Item >= UIREM_CMD_FIRST_ADMIN)){
		Local_u8Item += UIREM_MENU_ADMIN_ITEMS ;
	}
	switch(Local_u8Item){
	case 0:                      TERM_voidPutFlash(FLASH_STR("---------- MENU ----------"));        break ;
	case UIREM_CMD_LAMP:         TERM_voidPutFlash(FLASH_STR(" 1  Lamp on/off (1-5)"));             break ;
	case UIREM_CMD_DIMMER:       TERM_voidPutFlash(FLASH_STR(" 2  Dimmer level"));                  break ;
	case UIREM_CMD_AC:           TERM_voidPutFlash(FLASH_STR(" 3  AC status"));                     break ;
	case UIREM_CMD_HEATER:       TERM_voidPutFlash(FLASH_STR(" 4  Heater status"));                 break ;
	case UIREM_CMD_SET_TEMP:     TERM_voidPutFlash(FLASH_STR(" 5  Heater set temperature"));        break ;
	case UIREM_CMD_STATUS:       TERM_voidPutFlash(FLASH_STR(" 6  System status"));                 break ;
	case UIREM_CMD_DOOR_OPEN:    TERM_voidPutFlash(FLASH_STR(" 7  Open the door"));                 break ;
	case UIREM_CMD_DOOR_CLOSE:   TERM_voidPutFlash(FLASH_STR(" 8  Close the door"));                break ;
	case UIREM_CMD_KEYPAD:       TERM_voidPutFlash(FLASH_STR(" 9  Keypad control: allow / block")); break ;
	case UIREM_CMD_ADD_REMOTE:   TERM_voidPutFlash(FLASH_STR("10  Add remote user"));               break ;
	case UIREM_CMD_DEL_REMOTE:   TERM_voidPutFlash(FLASH_STR("11  Remove remote user"));            break ;
	case UIREM_CMD_ADD_KEYPAD:   TERM_voidPutFlash(FLASH_STR("12  Add keypad user"));               break ;
	case UIREM_CMD_DEL_KEYPAD:   TERM_voidPutFlash(FLASH_STR("13  Remove keypad user"));            break ;
	case UIREM_CMD_LIST:         TERM_voidPutFlash(FLASH_STR("14  List users"));                    break ;
	case UIREM_CMD_ADMIN_PASS:   TERM_voidPutFlash(FLASH_STR("15  Change admin password"));         break ;
	case UIREM_CMD_RESET:        TERM_voidPutFlash(FLASH_STR("16  Factory reset"));                 break ;
	case UIREM_MENU_ITEM_LOGOUT: TERM_voidPutFlash(FLASH_STR(" 0  Logout"));                        break ;
	default:                     TERM_voidPutFlash(FLASH_STR(" ?  Show this menu"));                break ;
	}
	TERM_voidNewLine();
	return (Local_u8Item >= UIREM_MENU_ITEM_HELP) ? 1 : 0 ;
}

/* status block , one line per step (uart_protocol.md Section 7) ; returns 1 after the last line */
static u8 UIREM_u8PrintStatus(u8 Copy_u8Step){
	u8 Local_u8Lamp ;
	u8 Local_u8Admin = (SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_ADMIN) ? 1 : 0 ;
	u8 Local_u8Done = 0 ;
	switch(Copy_u8Step){
	case 0:
		TERM_voidPutFlash(FLASH_STR("---- System status ----"));
		break ;
	case 1:
		TERM_voidPutFlash(FLASH_STR("Lamps :"));
		for(Local_u8Lamp = 1 ; Local_u8Lamp <= LAMP_COUNT ; Local_u8Lamp++){
			TERM_voidPutChar(' ');
			TERM_voidPutNumber(Local_u8Lamp);
			TERM_voidPutChar('=');
			UIREM_voidPutOnOff(LIGHT_u8GetLamp(Local_u8Lamp) == LAMP_ON);
		}
		break ;
	case 2:
		TERM_voidPutFlash(FLASH_STR("Dimmer: "));
		TERM_voidPutNumber(LIGHT_u8GetDimmer());
		TERM_voidPutChar('%');
		break ;
	case 3:
		TERM_voidPutFlash(FLASH_STR("AC    : "));
		if(CLIMATE_u8IsTempValid() == 1){
			UIREM_voidPutOnOff(CLIMATE_u8IsAcOn());
			TERM_voidPutFlash(FLASH_STR(", room "));
			TERM_voidPutNumber(CLIMATE_u8GetRoomTemp());
			TERM_voidPutChar('C');
		}else{
			TERM_voidPutFlash(FLASH_STR("waiting for the first reading."));
		}
		break ;
	case 4:
		TERM_voidPutFlash(FLASH_STR("Heater: "));
		UIREM_voidPutHeaterState();
		TERM_voidPutFlash(FLASH_STR(", water "));
		TERM_voidPutNumber(HEATER_u8GetWaterTemp());
		TERM_voidPutFlash(FLASH_STR("C, set "));
		TERM_voidPutNumber(HEATER_u8GetSetTemp());
		TERM_voidPutChar('C');
		break ;
	case UIREM_STATUS_STEP_DOOR:
		TERM_voidPutFlash((DOOR_u8GetState() == DOOR_OPEN) ? FLASH_STR("Door  : OPEN") : FLASH_STR("Door  : CLOSED"));
		Local_u8Done = (Local_u8Admin == 1) ? 0 : 1 ;
		break ;
	default:
		/* admin only (REQ-SEC-08) */
		TERM_voidPutFlash((SEC_u8IsLocalAllowed() == 1) ? FLASH_STR("Keypad: ALLOWED, ") : FLASH_STR("Keypad: BLOCKED, "));
		TERM_voidPutFlash((SEC_u8GetRole(SEC_SOURCE_LOCAL) != SEC_ROLE_NONE) ? FLASH_STR("user logged in") : FLASH_STR("nobody logged in"));
		Local_u8Done = 1 ;
		break ;
	}
	TERM_voidNewLine();
	return Local_u8Done ;
}

/* user list (uart_protocol.md Section 8) : step 0 = remote header , then one slot per step ,
 * then the keypad header and its slots. An empty slot prints nothing. Passwords are never printed. */
static u8 UIREM_u8PrintUsers(u8 Copy_u8Step){
	c8 Local_c8Name[USERDB_NAME_TEXT_SIZE] ;
	u8 Local_u8List = USERDB_LIST_REMOTE ;
	u8 Local_u8Slots = USERDB_REMOTE_SLOTS ;
	u8 Local_u8Done = 0 ;
	if(Copy_u8Step > USERDB_REMOTE_SLOTS){
		Copy_u8Step -= (USERDB_REMOTE_SLOTS + 1) ;
		Local_u8List = USERDB_LIST_KEYPAD ;
		Local_u8Slots = USERDB_KEYPAD_SLOTS ;
		Local_u8Done = (Copy_u8Step >= USERDB_KEYPAD_SLOTS) ? 1 : 0 ;
	}
	if(Copy_u8Step == 0){
		TERM_voidPutFlash((Local_u8List == USERDB_LIST_REMOTE) ? FLASH_STR("Remote users (") : FLASH_STR("Keypad users ("));
		TERM_voidPutNumber(UIREM_u8CountUsers(Local_u8List,Local_u8Slots));
		TERM_voidPutFlash(FLASH_STR(" of "));
		TERM_voidPutNumber(Local_u8Slots);
		TERM_voidPutFlash(FLASH_STR("):"));
		TERM_voidNewLine();
	}else if(USERDB_u8GetUserName(Local_u8List,Copy_u8Step - 1,Local_c8Name) == 1){
		TERM_voidPutFlash(FLASH_STR("  "));
		TERM_voidPutRam(Local_c8Name);
		TERM_voidNewLine();
	}else{
	}
	return Local_u8Done ;
}

/* REQ-RUI-02 : one "[INFO]" line for a change made on the keypad , the heater panel or by itself */
static void UIREM_voidPrintEvent(){
	UIREM_voidFreshLine();
	switch(Global_u8Event){
	case EVQ_LAMP:   UIREM_voidPutLamp(UIREM_MSG_EVENT,Global_u8EventArg); break ;
	case EVQ_DIMMER: UIREM_voidPutDimmer(UIREM_MSG_EVENT);                 break ;
	case EVQ_AC:
		TERM_voidPutFlash(FLASH_STR("[INFO] AC is now "));
		UIREM_voidPutOnOff(Global_u8EventArg);
		break ;
	case EVQ_HEATER_POWER:
		TERM_voidPutFlash(FLASH_STR("[INFO] Heater is now "));
		UIREM_voidPutOnOff(Global_u8EventArg);
		break ;
	case EVQ_HEATER_ELEMENT:
		TERM_voidPutFlash(FLASH_STR("[INFO] Heater: "));
		UIREM_voidPutElement(Global_u8EventArg);
		break ;
	case EVQ_HEATER_SET: UIREM_voidPutSetTemp(UIREM_MSG_EVENT); break ;
	case EVQ_LOCAL_SESSION:
		TERM_voidPutFlash((Global_u8EventArg != 0) ? FLASH_STR("[INFO] Keypad user logged in") : FLASH_STR("[INFO] Keypad user logged out"));
		break ;
	case EVQ_STORAGE_FAULT:
		UIREM_u8PutFixedText(UIREM_MSG_WARN_STORAGE);
		break ;
	default:
		break ;
	}
	TERM_voidNewLine();
}

/* Prints ONE step of a message. Returns 1 when the message is finished.
 * Only called when TERM_u8TxFree() >= UIREM_TX_RESERVE , and no step is longer than that. */
static u8 UIREM_u8PrintStep(u8 Copy_u8Message, u8 Copy_u8Step){
	u8 Local_u8Number ;
	if(Copy_u8Message == UIREM_MSG_ALERT){
		UIREM_voidFreshLine();
	}
	if(UIREM_u8PutFixedText(Copy_u8Message) == 1){
		TERM_voidNewLine();
		return 1 ;
	}
	switch(Copy_u8Message){
	case UIREM_MSG_MENU:   return UIREM_u8PrintMenu(Copy_u8Step) ;
	case UIREM_MSG_STATUS: return UIREM_u8PrintStatus(Copy_u8Step) ;
	case UIREM_MSG_USERS:  return UIREM_u8PrintUsers(Copy_u8Step) ;
	case UIREM_MSG_EVENT:  UIREM_voidPrintEvent(); return 1 ;
	case UIREM_MSG_PROMPT:
		/* "admin> " : the only message without a line end */
		TERM_voidPutRam(Global_c8Name);
		TERM_voidPutFlash(FLASH_STR("> "));
		Global_u8AtPrompt = 1 ;
		return 1 ;
	case UIREM_MSG_WELCOME:
		TERM_voidPutFlash(FLASH_STR("Welcome "));
		TERM_voidPutRam(Global_c8Name);
		TERM_voidPutFlash((SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_ADMIN) ? FLASH_STR(". You are logged in as ADMIN.") : FLASH_STR(". You are logged in as USER."));
		break ;
	case UIREM_MSG_ERR_LOGIN:
		Local_u8Number = SEC_u8GetAttemptsLeft(SEC_SOURCE_REMOTE);
		TERM_voidPutFlash(FLASH_STR("[ERR] Wrong username or password. "));
		TERM_voidPutNumber(Local_u8Number);
		TERM_voidPutFlash((Local_u8Number == 1) ? FLASH_STR(" attempt left.") : FLASH_STR(" attempts left."));
		break ;
	case UIREM_MSG_OK_LAMP:   UIREM_voidPutLamp(UIREM_MSG_OK_LAMP,Global_u8Arg); break ;
	case UIREM_MSG_OK_DIMMER: UIREM_voidPutDimmer(UIREM_MSG_OK_DIMMER);          break ;
	case UIREM_MSG_OK_TEMP:   UIREM_voidPutSetTemp(UIREM_MSG_OK_TEMP);           break ;
	case UIREM_MSG_DOOR:
		/* Global_u8Arg : 1 = the door moved , 0 = it was already there */
		if(Global_u8Arg == 1){
			TERM_voidPutFlash((DOOR_u8GetState() == DOOR_OPEN) ? FLASH_STR("[OK] Door is now OPEN") : FLASH_STR("[OK] Door is now CLOSED"));
		}else{
			TERM_voidPutFlash((DOOR_u8GetState() == DOOR_OPEN) ? FLASH_STR("Door is already open.") : FLASH_STR("Door is already closed."));
		}
		break ;
	case UIREM_MSG_OK_KEYPAD:
		TERM_voidPutFlash((SEC_u8IsLocalAllowed() == 1) ? FLASH_STR("[OK] Keypad control is now ALLOWED") : FLASH_STR("[OK] Keypad control is now BLOCKED"));
		break ;
	case UIREM_MSG_OK_ADDED:
	case UIREM_MSG_OK_REMOVED:
		/* Global_u8Arg : the list */
		TERM_voidPutFlash((Global_u8Arg == USERDB_LIST_REMOTE) ? FLASH_STR("[OK] Remote user ") : FLASH_STR("[OK] Keypad user "));
		TERM_voidPutRam(Global_c8Target);
		TERM_voidPutFlash((Copy_u8Message == UIREM_MSG_OK_ADDED) ? FLASH_STR(" added.") : FLASH_STR(" removed."));
		break ;
	case UIREM_MSG_GOODBYE:
		TERM_voidPutFlash(FLASH_STR("Goodbye "));
		TERM_voidPutRam(Global_c8Name);
		TERM_voidPutChar('.');
		break ;
	case UIREM_MSG_AC:
		if(CLIMATE_u8IsTempValid() == 1){
			TERM_voidPutFlash(FLASH_STR("AC is "));
			UIREM_voidPutOnOff(CLIMATE_u8IsAcOn());
			TERM_voidPutFlash(FLASH_STR(". Room temperature "));
			TERM_voidPutNumber(CLIMATE_u8GetRoomTemp());
			TERM_voidPutFlash(FLASH_STR("C."));
		}else{
			TERM_voidPutFlash(FLASH_STR("AC: waiting for the first reading."));
		}
		break ;
	case UIREM_MSG_HEATER:
		TERM_voidPutFlash(FLASH_STR("Heater is "));
		UIREM_voidPutHeaterState();
		TERM_voidPutFlash(FLASH_STR(". Water "));
		TERM_voidPutNumber(HEATER_u8GetWaterTemp());
		TERM_voidPutFlash(FLASH_STR("C, set "));
		TERM_voidPutNumber(HEATER_u8GetSetTemp());
		TERM_voidPutFlash(FLASH_STR("C."));
		break ;
	case UIREM_MSG_TIMEOUT:
		UIREM_voidFreshLine();
		TERM_voidPutFlash(FLASH_STR("[INFO] No input for "));
		TERM_voidPutNumber(UIREM_IDLE_TIMEOUT_S);
		TERM_voidPutFlash(FLASH_STR(" s. Logged out."));
		break ;
	default:
		break ;
	}
	TERM_voidNewLine();
	return 1 ;
}

/************************** command handling **************************/
/* the command is finished : its reply , then the prompt (D-8 : the menu only if configured) */
static void UIREM_voidDone(u8 Copy_u8Message){
	UIREM_voidSetMasked(0);
	UIREM_voidQueue(Copy_u8Message);
#if UIREM_MENU_AFTER_COMMAND == 1
	UIREM_voidQueue(UIREM_MSG_MENU);
#endif
	UIREM_voidQueue(UIREM_MSG_PROMPT);
	Global_u8State = UIREM_STATE_PROMPT ;
}
/* REQ-RUI-04 : the input was rejected : a clear message , then the menu again */
static void UIREM_voidError(u8 Copy_u8Message){
	UIREM_voidSetMasked(0);
	UIREM_voidQueue(Copy_u8Message);
	UIREM_voidQueue(UIREM_MSG_MENU);
	UIREM_voidQueue(UIREM_MSG_PROMPT);
	Global_u8State = UIREM_STATE_PROMPT ;
}
/* end of the remote session : a last line , then banner and username prompt */
static void UIREM_voidEndSession(u8 Copy_u8Message){
	UIREM_voidSetMasked(0);
	SEC_voidLogout(SEC_SOURCE_REMOTE);
	UIREM_voidQueue(Copy_u8Message);
	UIREM_voidQueue(UIREM_MSG_BANNER);
	UIREM_voidQueue(UIREM_MSG_ASK_NAME);
	Global_u8State = UIREM_STATE_ASK_NAME ;
}

/* the argument of command 1 , 2 , 5 , 11 or 13 : typed on the command line ("1 3") or after the question.
 * The terminal's own change is answered with [OK] , so its event is muted (uart_protocol.md Section 9). */
static void UIREM_voidArgument(const c8 * Copy_pc8Text){
	u16 Local_u16Number = 0 ;
	u8 Local_u8IsNumber = 0 ;
	u8 Local_u8Before ;
	u8 Local_u8Ok ;
	u8 Local_u8List ;
	if(Copy_pc8Text[0] != '\0'){
		Local_u8IsNumber = FMT_u8TextToNumber(Copy_pc8Text,&Local_u16Number);
	}
	switch(Global_u8Command){
	case UIREM_CMD_LAMP:
		/* REQ-LGT-01 */
		if((Local_u8IsNumber == 0) || (Local_u16Number < 1) || (Local_u16Number > LAMP_COUNT)){
			UIREM_voidError(UIREM_MSG_ERR_LAMP);
		}else{
			Local_u8Before = LIGHT_u8GetLamp((u8)Local_u16Number);
			EVQ_voidSetMute(1);
			Local_u8Ok = (LIGHT_u8ToggleLamp((u8)Local_u16Number) != Local_u8Before) ? 1 : 0 ;
			EVQ_voidSetMute(0);
			if(Local_u8Ok == 1){
				Global_u8Arg = (u8)Local_u16Number ;
				UIREM_voidDone(UIREM_MSG_OK_LAMP);
			}else{
				UIREM_voidError(UIREM_MSG_ERR_DEVICE);         /* the lamp chip did not answer : the state is unchanged */
			}
		}
		break ;
	case UIREM_CMD_DIMMER:
		/* REQ-LGT-02 */
		if((Local_u8IsNumber == 0) || (Local_u16Number > LIGHT_DIMMER_MAX) || ((Local_u16Number % LIGHT_DIMMER_STEP) != 0)){
			UIREM_voidError(UIREM_MSG_ERR_DIMMER);
		}else{
			EVQ_voidSetMute(1);
			LIGHT_voidSetDimmer((u8)Local_u16Number);
			EVQ_voidSetMute(0);
			UIREM_voidDone(UIREM_MSG_OK_DIMMER);
		}
		break ;
	case UIREM_CMD_SET_TEMP:
		/* REQ-HTR-14 : HEATER checks 35..75 in steps of 5 and saves */
		Local_u8Ok = 0 ;
		if((Local_u8IsNumber == 1) && (Local_u16Number <= 255)){
			EVQ_voidSetMute(1);
			Local_u8Ok = HEATER_u8SetSetTemp((u8)Local_u16Number);
			EVQ_voidSetMute(0);
		}
		if(Local_u8Ok == 1){
			UIREM_voidDone(UIREM_MSG_OK_TEMP);
		}else{
			UIREM_voidError(UIREM_MSG_ERR_TEMP);
		}
		break ;
	case UIREM_CMD_DEL_REMOTE:
	case UIREM_CMD_DEL_KEYPAD:
		/* REQ-SEC-03 */
		Local_u8List = (Global_u8Command == UIREM_CMD_DEL_REMOTE) ? USERDB_LIST_REMOTE : USERDB_LIST_KEYPAD ;
		if(USERDB_u8RemoveUser(Local_u8List,Copy_pc8Text) == USERDB_OK){
			UIREM_voidCopy(Global_c8Target,Copy_pc8Text,USERDB_NAME_TEXT_SIZE);
			Global_u8Arg = Local_u8List ;
			UIREM_voidDone(UIREM_MSG_OK_REMOVED);
		}else{
			UIREM_voidError(UIREM_MSG_ERR_NOT_FOUND);
		}
		break ;
	default:
		UIREM_voidError(UIREM_MSG_ERR_UNKNOWN);
		break ;
	}
}

/* one line typed at the prompt : "<number>" , "<number> <argument>" , "?" or nothing */
static void UIREM_voidCommand(c8 * Copy_pc8Line){
	c8 * Local_pc8Arg = NULL ;
	u16 Local_u16Number = 0 ;
	u8 i = 0 ;
	/*1. cut the line at the first space : command , argument */
	while((Copy_pc8Line[i] != '\0') && (Copy_pc8Line[i] != ' ')){
		i++;
	}
	if(Copy_pc8Line[i] == ' '){
		Copy_pc8Line[i++] = '\0' ;
		while(Copy_pc8Line[i] == ' '){
			i++;
		}
		if(Copy_pc8Line[i] != '\0'){
			Local_pc8Arg = &Copy_pc8Line[i] ;
		}
	}
	/*2. '?' or an empty line : the menu */
	if((Copy_pc8Line[0] == '\0') || ((Copy_pc8Line[0] == '?') && (Copy_pc8Line[1] == '\0'))){
		UIREM_voidQueue(UIREM_MSG_MENU);
		UIREM_voidQueue(UIREM_MSG_PROMPT);
		return ;
	}
	/*3. REQ-RUI-04 */
	if((FMT_u8TextToNumber(Copy_pc8Line,&Local_u16Number) == 0) || (Local_u16Number > UIREM_CMD_LAST)){
		UIREM_voidError(UIREM_MSG_ERR_UNKNOWN);
		return ;
	}
	/*4. REQ-SEC-06 , REQ-SEC-07 : door , accounts and keypad control are for the admin */
	if((Local_u16Number >= UIREM_CMD_FIRST_ADMIN) && (SEC_u8GetRole(SEC_SOURCE_REMOTE) != SEC_ROLE_ADMIN)){
		UIREM_voidError(UIREM_MSG_ERR_ROLE);
		return ;
	}
	Global_u8Command = (u8)Local_u16Number ;
	switch(Global_u8Command){
	case UIREM_CMD_LOGOUT: UIREM_voidEndSession(UIREM_MSG_GOODBYE); break ;
	case UIREM_CMD_AC:     UIREM_voidDone(UIREM_MSG_AC);            break ;
	case UIREM_CMD_HEATER: UIREM_voidDone(UIREM_MSG_HEATER);        break ;
	case UIREM_CMD_STATUS: UIREM_voidDone(UIREM_MSG_STATUS);        break ;
	case UIREM_CMD_LIST:   UIREM_voidDone(UIREM_MSG_USERS);         break ;
	case UIREM_CMD_DOOR_OPEN:
		/* REQ-DOR-01 */
		Global_u8Arg = DOOR_u8SetState(DOOR_OPEN);
		UIREM_voidDone(UIREM_MSG_DOOR);
		break ;
	case UIREM_CMD_DOOR_CLOSE:
		Global_u8Arg = DOOR_u8SetState(DOOR_CLOSED);
		UIREM_voidDone(UIREM_MSG_DOOR);
		break ;
	case UIREM_CMD_KEYPAD:
		/* REQ-SEC-08 : toggles. Blocking ends a keypad session ; SEC reports that as an event */
		SEC_voidSetLocalAllowed((SEC_u8IsLocalAllowed() == 1) ? 0 : 1);
		UIREM_voidDone(UIREM_MSG_OK_KEYPAD);
		break ;
	case UIREM_CMD_ADD_REMOTE:
		UIREM_voidQueue(UIREM_MSG_ASK_REMOTE_NAME);
		Global_u8State = UIREM_STATE_ADD_NAME ;
		break ;
	case UIREM_CMD_ADD_KEYPAD:
		UIREM_voidQueue(UIREM_MSG_ASK_KEYPAD_ID);
		Global_u8State = UIREM_STATE_ADD_NAME ;
		break ;
	case UIREM_CMD_ADMIN_PASS:
		UIREM_voidSetMasked(1);
		UIREM_voidQueue(UIREM_MSG_ASK_ADMIN_PASS);
		Global_u8State = UIREM_STATE_NEW_PASS ;
		break ;
	case UIREM_CMD_RESET:
		UIREM_voidQueue(UIREM_MSG_ASK_CONFIRM);
		Global_u8State = UIREM_STATE_CONFIRM ;
		break ;
	default:
		/* 1 , 2 , 5 , 11 , 13 : one argument , on the same line (D-9) or asked for */
		if(Local_pc8Arg != NULL){
			UIREM_voidArgument(Local_pc8Arg);
		}else{
			switch(Global_u8Command){
			case UIREM_CMD_LAMP:       UIREM_voidQueue(UIREM_MSG_ASK_LAMP);       break ;
			case UIREM_CMD_DIMMER:     UIREM_voidQueue(UIREM_MSG_ASK_DIMMER);     break ;
			case UIREM_CMD_SET_TEMP:   UIREM_voidQueue(UIREM_MSG_ASK_TEMP);       break ;
			case UIREM_CMD_DEL_REMOTE: UIREM_voidQueue(UIREM_MSG_ASK_DEL_REMOTE); break ;
			default:                   UIREM_voidQueue(UIREM_MSG_ASK_DEL_KEYPAD); break ;
			}
			Global_u8State = UIREM_STATE_ARG ;
		}
		break ;
	}
}

/* REQ-SEC-01 , REQ-SEC-05 : the pair is checked once , after both lines */
static void UIREM_voidLogin(const c8 * Copy_pc8Pass){
	u8 Local_u8Result ;
	UIREM_voidSetMasked(0);
	Global_u8State = UIREM_STATE_ASK_NAME ;
	/* what happened while nobody was logged in is not reported */
	UIREM_voidDropEvents();
	Local_u8Result = SEC_u8Login(SEC_SOURCE_REMOTE,Global_c8Name,Copy_pc8Pass);
	if((Local_u8Result == SEC_LOGIN_ADMIN) || (Local_u8Result == SEC_LOGIN_USER)){
		Global_u8IdleSeconds = 0 ;
		UIREM_voidQueue(UIREM_MSG_WELCOME);
		/* a fault found at boot posted no event : tell it here (REQ-EEP-03 , test T-78) */
		if(ESTORE_u8GetStatus() == ESTORE_FAULT){
			UIREM_voidQueue(UIREM_MSG_WARN_STORAGE);
		}
		UIREM_voidQueue(UIREM_MSG_MENU);
		UIREM_voidQueue(UIREM_MSG_PROMPT);
		Global_u8State = UIREM_STATE_PROMPT ;
	}else if(Local_u8Result == SEC_LOGIN_FAILED){
		UIREM_voidQueue(UIREM_MSG_ERR_LOGIN);
		UIREM_voidQueue(UIREM_MSG_ASK_NAME);
	}else{
		/* locked : the next task call sees ALARM_u8IsActive and prints the alert */
	}
}

/* REQ-SEC-03 : the new name is checked before the password is asked */
static void UIREM_voidAddName(const c8 * Copy_pc8Line){
	u8 Local_u8List  = (Global_u8Command == UIREM_CMD_ADD_REMOTE) ? USERDB_LIST_REMOTE : USERDB_LIST_KEYPAD ;
	u8 Local_u8Slots = (Global_u8Command == UIREM_CMD_ADD_REMOTE) ? USERDB_REMOTE_SLOTS : USERDB_KEYPAD_SLOTS ;
	u8 Local_u8Length = UIREM_u8TextLength(Copy_pc8Line);
	if((Local_u8Length < USERDB_NAME_MIN) || (Local_u8Length > USERDB_NAME_MAX) || (UIREM_u8HasSpace(Copy_pc8Line) == 1)){
		UIREM_voidError(UIREM_MSG_ERR_NAME);
	}else if((Local_u8List == USERDB_LIST_KEYPAD) && (UIREM_u8IsDigits(Copy_pc8Line) == 0)){
		UIREM_voidError(UIREM_MSG_ERR_DIGITS);                 /* REQ-SEC-02 */
	}else if(UIREM_u8UserExists(Local_u8List,Local_u8Slots,Copy_pc8Line) == 1){
		UIREM_voidError(UIREM_MSG_ERR_EXISTS);
	}else if(UIREM_u8CountUsers(Local_u8List,Local_u8Slots) >= Local_u8Slots){
		UIREM_voidError(UIREM_MSG_ERR_FULL);
	}else{
		UIREM_voidCopy(Global_c8Target,Copy_pc8Line,USERDB_NAME_TEXT_SIZE);
		UIREM_voidSetMasked(1);
		UIREM_voidQueue((Local_u8List == USERDB_LIST_REMOTE) ? UIREM_MSG_ASK_REMOTE_PASS : UIREM_MSG_ASK_KEYPAD_PIN);
		Global_u8State = UIREM_STATE_ADD_PASS ;
	}
}
static void UIREM_voidAddPass(const c8 * Copy_pc8Line){
	u8 Local_u8List = (Global_u8Command == UIREM_CMD_ADD_REMOTE) ? USERDB_LIST_REMOTE : USERDB_LIST_KEYPAD ;
	u8 Local_u8Length = UIREM_u8TextLength(Copy_pc8Line);
	/* a keypad PIN of the right length with a letter in it : say what is really wrong */
	if((Local_u8List == USERDB_LIST_KEYPAD) && (Local_u8Length >= USERDB_PASS_MIN) && (Local_u8Length <= USERDB_PASS_MAX) && (UIREM_u8IsDigits(Copy_pc8Line) == 0)){
		UIREM_voidError(UIREM_MSG_ERR_DIGITS);
		return ;
	}
	/* USERDB checks everything again (a remote name equal to the admin's name is only found here) */
	switch(USERDB_u8AddUser(Local_u8List,Global_c8Target,Copy_pc8Line)){
	case USERDB_OK:
		Global_u8Arg = Local_u8List ;
		UIREM_voidDone(UIREM_MSG_OK_ADDED);
		break ;
	case USERDB_ERR_BAD_PASS: UIREM_voidError(UIREM_MSG_ERR_PASS);   break ;
	case USERDB_ERR_EXISTS:   UIREM_voidError(UIREM_MSG_ERR_EXISTS); break ;
	case USERDB_ERR_FULL:     UIREM_voidError(UIREM_MSG_ERR_FULL);   break ;
	default:                  UIREM_voidError(UIREM_MSG_ERR_NAME);   break ;
	}
}
/* D-9 : factory reset , only after "YES" */
static void UIREM_voidConfirm(const c8 * Copy_pc8Line){
	if((Copy_pc8Line[0] == 'Y') && (Copy_pc8Line[1] == 'E') && (Copy_pc8Line[2] == 'S') && (Copy_pc8Line[3] == '\0')){
		ESTORE_voidLoadDefaults();
		/* the heater works with its own copy of the set temperature : give it the default too */
		EVQ_voidSetMute(1);
		HEATER_u8SetSetTemp(HEATER_SET_DEFAULT);
		EVQ_voidSetMute(0);
		UIREM_voidEndSession(UIREM_MSG_OK_RESET);
	}else{
		UIREM_voidDone(UIREM_MSG_CANCELLED);
	}
}

/* REQ-RUI-02 : called when no line arrived */
static void UIREM_voidEvents(){
	if(SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_NONE){
		/* nobody is logged in : events are read and thrown away. The lockdown is not lost :
		 * the task prints its alert from ALARM_u8IsActive , whoever caused it */
		UIREM_voidDropEvents();
	}else if((Global_u8State == UIREM_STATE_PROMPT) && (TERM_u8IsLineEmpty() == 1)){
		/* only at the prompt with nothing typed , so an event never cuts into a half typed line */
		if((EVQ_u8Get(&Global_u8Event,&Global_u8EventArg) == 1) && (Global_u8Event != EVQ_LOCKDOWN)){
			UIREM_voidQueue(UIREM_MSG_EVENT);
			UIREM_voidQueue(UIREM_MSG_PROMPT);
		}
	}else{
		/* in the middle of a command : the events wait in EVQ */
	}
}

/****************************** public ******************************/
void UIREM_voidInit(){
	Global_u8State = UIREM_STATE_ASK_NAME ;
	Global_u8Command = 0 ;
	Global_u8IdleSeconds = 0 ;
	Global_u8AtPrompt = 0 ;
	Global_u8QueueCount = 0 ;
	Global_u8Step = 0 ;
	Global_c8Name[0] = '\0' ;
	Global_c8Target[0] = '\0' ;
	EVQ_voidSetMute(0);
	UIREM_voidSetMasked(0);
	UIREM_voidQueue(UIREM_MSG_BANNER);
	UIREM_voidQueue(UIREM_MSG_ASK_NAME);
}

void UIREM_voidTask5ms(){
	c8 Local_c8Line[TERM_LINE_MAX + 1] ;
	u8 Local_u8Status ;
	u8 i ;

	/*1. REQ-SEC-05 : lockdown , whoever caused it : the alert once , then nothing is answered */
	if((Global_u8State != UIREM_STATE_LOCKED) && (ALARM_u8IsActive() == 1)){
		Global_u8QueueCount = 0 ;
		Global_u8Step = 0 ;
		UIREM_voidQueue(UIREM_MSG_ALERT);
		TERM_voidSetEcho(TERM_ECHO_OFF);
		Global_u8State = UIREM_STATE_LOCKED ;
	}

	/*2. output rule (architecture.md 5.8) : nothing is done while the TX ring is fuller than the
	 * reserve , so a step always fits and the echo of a typed line is never dropped.
	 * Typed bytes wait in the RX ring meanwhile. */
	if(TERM_u8TxFree() < UIREM_TX_RESERVE){
		return ;
	}
	/*3. at most ONE print step per call */
	if(Global_u8QueueCount > 0){
		if(UIREM_u8PrintStep(Global_u8Queue[0],Global_u8Step) == 1){
			Global_u8QueueCount--;
			for(i = 0 ; i < Global_u8QueueCount ; i++){
				Global_u8Queue[i] = Global_u8Queue[i + 1] ;
			}
			Global_u8Step = 0 ;
		}else{
			Global_u8Step++;
		}
		return ;
	}

	/*4. input : only when everything is printed */
	Local_u8Status = TERM_u8GetLine(Local_c8Line);
	if(Global_u8State == UIREM_STATE_LOCKED){
		UIREM_voidDropEvents();                        /* received bytes and events are thrown away */
		return ;
	}
	if(Local_u8Status == TERM_LINE_NONE){
		UIREM_voidEvents();
		return ;
	}
	Global_u8AtPrompt = 0 ;                            /* Enter was echoed : the cursor is at a line start */
	if(Local_u8Status == TERM_LINE_TOO_LONG){
		if(Global_u8State <= UIREM_STATE_ASK_PASS){
			/* ASSUMPTION: a too long name or password is rejected at once and is not a login
			 * attempt (no pair was checked) ; the login starts again */
			UIREM_voidSetMasked(0);
			UIREM_voidQueue(UIREM_MSG_ERR_TOO_LONG);
			UIREM_voidQueue(UIREM_MSG_ASK_NAME);
			Global_u8State = UIREM_STATE_ASK_NAME ;
		}else{
			UIREM_voidError(UIREM_MSG_ERR_TOO_LONG);
		}
		return ;
	}
	switch(Global_u8State){
	case UIREM_STATE_ASK_NAME:
		if(Local_c8Line[0] == '\0'){
			/* ASSUMPTION: Enter alone at the username prompt asks again , it is not a login attempt */
			UIREM_voidQueue(UIREM_MSG_ASK_NAME);
		}else{
			UIREM_voidCopy(Global_c8Name,Local_c8Line,TERM_LINE_MAX + 1);
			UIREM_voidSetMasked(1);
			UIREM_voidQueue(UIREM_MSG_ASK_PASS);
			Global_u8State = UIREM_STATE_ASK_PASS ;
		}
		break ;
	case UIREM_STATE_ASK_PASS: UIREM_voidLogin(Local_c8Line);    break ;
	case UIREM_STATE_PROMPT:   UIREM_voidCommand(Local_c8Line);  break ;
	case UIREM_STATE_ARG:      UIREM_voidArgument(Local_c8Line); break ;
	case UIREM_STATE_ADD_NAME: UIREM_voidAddName(Local_c8Line);  break ;
	case UIREM_STATE_ADD_PASS: UIREM_voidAddPass(Local_c8Line);  break ;
	case UIREM_STATE_NEW_PASS:
		/* D-9 : USERDB checks the length (4..8) */
		if(USERDB_u8SetAdminPassword(Local_c8Line) == USERDB_OK){
			UIREM_voidDone(UIREM_MSG_OK_ADMIN_PASS);
		}else{
			UIREM_voidError(UIREM_MSG_ERR_PASS);
		}
		break ;
	case UIREM_STATE_CONFIRM:  UIREM_voidConfirm(Local_c8Line);  break ;
	default:
		break ;
	}
}

void UIREM_voidTask1s(){
	/* always read : the flag is cleared by reading it */
	u8 Local_u8Active = TERM_u8IsRxActive();
	if((Local_u8Active == 1) || (Global_u8State == UIREM_STATE_LOCKED) || (SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_NONE)){
		Global_u8IdleSeconds = 0 ;
		return ;
	}
	/* D-11 : a session left alone ends , so an admin who walks away does not block the keypad for ever.
	 * ASSUMPTION: characters typed before the timeout but not ended with Enter stay in the line editor */
	Global_u8IdleSeconds++;
	if(Global_u8IdleSeconds >= UIREM_IDLE_TIMEOUT_S){
		Global_u8IdleSeconds = 0 ;
		UIREM_voidEndSession(UIREM_MSG_TIMEOUT);
	}
}
