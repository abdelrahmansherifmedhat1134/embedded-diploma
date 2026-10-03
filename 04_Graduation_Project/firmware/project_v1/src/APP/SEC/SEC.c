/*
 * SEC.c
 *
 *  Created on: Oct 3, 2026
 *      Author: eslam
 */
#include "../../../lib/Service/std_types.h"
#include "../../../lib/Service/EVQ/EVQ.h"
#include "../../../lib/Service/USERDB/USERDB.h"
#include "../ALARM/ALARM.h"
#include "SEC.h"
#include "SEC_cfg.h"

/* one session and one fail counter per login source (architecture.md 5.1) */
static u8 Global_u8Role[SEC_SOURCE_COUNT] = {SEC_ROLE_NONE,SEC_ROLE_NONE} ;
static u8 Global_u8FailCount[SEC_SOURCE_COUNT] = {0,0} ;
static u8 Global_u8LocalAllowed = 0 ;                  /* set by the admin , only looked at while the admin is logged in */

/* ends the session of one source , if there is one */
static void SEC_voidEndSession(u8 Copy_u8Source){
	if(Global_u8Role[Copy_u8Source] != SEC_ROLE_NONE){
		if(Global_u8Role[Copy_u8Source] == SEC_ROLE_ADMIN){
			USERDB_voidSetWriteAccess(0);              /* REQ-SEC-07 : the accounts are read-only again */
			Global_u8LocalAllowed = 0 ;
		}
		if(Copy_u8Source == SEC_SOURCE_LOCAL){
			EVQ_voidPost(EVQ_LOCAL_SESSION,0);
		}
		Global_u8Role[Copy_u8Source] = SEC_ROLE_NONE ;
	}
}

void SEC_voidInit(){
	Global_u8Role[SEC_SOURCE_REMOTE] = SEC_ROLE_NONE ;
	Global_u8Role[SEC_SOURCE_LOCAL]  = SEC_ROLE_NONE ;
	Global_u8FailCount[SEC_SOURCE_REMOTE] = 0 ;
	Global_u8FailCount[SEC_SOURCE_LOCAL]  = 0 ;
	Global_u8LocalAllowed = 0 ;
	USERDB_voidSetWriteAccess(0);
}

u8 SEC_u8Login(u8 Copy_u8Source, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass){
	u8 Local_u8Result = SEC_LOGIN_FAILED ;
	if(Copy_u8Source >= SEC_SOURCE_COUNT){
		//error
		return SEC_LOGIN_FAILED ;
	}
	/* REQ-SEC-05 : locked until reset , nothing is checked any more */
	if(ALARM_u8IsActive() == 1){
		return SEC_LOGIN_LOCKED ;
	}
	/* ASSUMPTION: a login on a source that is still logged in ends that session first
	 * (the UIs only offer a login while logged out) */
	SEC_voidEndSession(Copy_u8Source);

	/* one attempt = name and password checked together : the result never tells which half was wrong */
	if(Copy_u8Source == SEC_SOURCE_REMOTE){
		/* REQ-SEC-01 : the admin record is only checked for the remote source */
		if(USERDB_u8CheckAdmin(Copy_pc8Name,Copy_pc8Pass) == 1){
			Local_u8Result = SEC_LOGIN_ADMIN ;
		}else if(USERDB_u8CheckUser(USERDB_LIST_REMOTE,Copy_pc8Name,Copy_pc8Pass) == 1){
			Local_u8Result = SEC_LOGIN_USER ;          /* REQ-SEC-02 : remote list */
		}else{
		}
	}else{
		/* REQ-SEC-02 : keypad list ; REQ-SEC-08 : not while the admin blocks the keypad */
		if((SEC_u8IsLocalAllowed() == 1) && (USERDB_u8CheckUser(USERDB_LIST_KEYPAD,Copy_pc8Name,Copy_pc8Pass) == 1)){
			Local_u8Result = SEC_LOGIN_USER ;
		}
	}

	switch(Local_u8Result){
	case SEC_LOGIN_ADMIN:
		Global_u8FailCount[SEC_SOURCE_REMOTE] = 0 ;
		Global_u8Role[SEC_SOURCE_REMOTE] = SEC_ROLE_ADMIN ;
		USERDB_voidSetWriteAccess(1);                  /* REQ-SEC-07 */
		/* REQ-SEC-08 (Section 12 #1) : an admin login ends the keypad session , keypad blocked until allowed */
		Global_u8LocalAllowed = 0 ;
		SEC_voidEndSession(SEC_SOURCE_LOCAL);
		break ;
	case SEC_LOGIN_USER:
		Global_u8FailCount[Copy_u8Source] = 0 ;
		Global_u8Role[Copy_u8Source] = SEC_ROLE_USER ;
		if(Copy_u8Source == SEC_SOURCE_LOCAL){
			EVQ_voidPost(EVQ_LOCAL_SESSION,1);
		}
		break ;
	default:
		/* REQ-SEC-05 : the 3rd failure in a row on this source locks the system */
		Global_u8FailCount[Copy_u8Source]++;
		if(Global_u8FailCount[Copy_u8Source] >= SEC_MAX_FAILED_LOGINS){
			ALARM_voidTrigger();
			Local_u8Result = SEC_LOGIN_LOCKED ;
		}
		break ;
	}
	return Local_u8Result ;
}

void SEC_voidLogout(u8 Copy_u8Source){
	if(Copy_u8Source < SEC_SOURCE_COUNT){
		SEC_voidEndSession(Copy_u8Source);
	}else{
		//error
	}
}

/* REQ-SEC-06 : the UIs decide what a role may do (door and user management = admin only) */
u8 SEC_u8GetRole(u8 Copy_u8Source){
	u8 Local_u8Role = SEC_ROLE_NONE ;
	if(Copy_u8Source < SEC_SOURCE_COUNT){
		Local_u8Role = Global_u8Role[Copy_u8Source] ;
	}else{
		//error
	}
	return Local_u8Role ;
}

u8 SEC_u8GetAttemptsLeft(u8 Copy_u8Source){
	u8 Local_u8Left = 0 ;
	if((Copy_u8Source < SEC_SOURCE_COUNT) && (Global_u8FailCount[Copy_u8Source] < SEC_MAX_FAILED_LOGINS)){
		Local_u8Left = SEC_MAX_FAILED_LOGINS - Global_u8FailCount[Copy_u8Source] ;
	}
	return Local_u8Left ;
}

/* REQ-SEC-08 : not locked , and not (admin logged in and keypad not allowed) */
u8 SEC_u8IsLocalAllowed(){
	if(ALARM_u8IsActive() == 1){
		return 0 ;
	}
	if((Global_u8Role[SEC_SOURCE_REMOTE] == SEC_ROLE_ADMIN) && (Global_u8LocalAllowed == 0)){
		return 0 ;
	}
	return 1 ;
}

void SEC_voidSetLocalAllowed(u8 Copy_u8State){
	if(Global_u8Role[SEC_SOURCE_REMOTE] == SEC_ROLE_ADMIN){
		if(Copy_u8State == 0){
			/* blocking the keypad also ends a keypad session */
			Global_u8LocalAllowed = 0 ;
			SEC_voidEndSession(SEC_SOURCE_LOCAL);
		}else{
			Global_u8LocalAllowed = 1 ;
		}
	}else{
		/* only the admin decides this */
	}
}
