/*
 *****************************************************************************
 *
 *        Copyright 1989, Xylogics, Inc.  ALL RIGHTS RESERVED.
 *
 * ALL RIGHTS RESERVED. Licensed Material - Property of Xylogics, Inc.
 * This software is made available solely pursuant to the terms of a
 * software license agreement which governs its use.
 * Unauthorized duplication, distribution or sale are strictly prohibited.
 *
 * Include file description:
 *
 *	Include SRPC layer information (Secure Remote Procedure Call)
 *
 * Original Author: Dave Harris		Created on: April 4, 1988
 *
 * Revision Control Information:
 *
 * $Header: /p/xinu/cvs/ConsoleTools/reboot/libannex/srpc.h,v 1.1 2002/01/14 19:24:24 brylow Exp $
 *
 * This file created by RCS from:
 *
 * $Source: /p/xinu/cvs/ConsoleTools/reboot/libannex/srpc.h,v $
 *
 * Revision History:
 *
 * $Locker:  $
 *
 *  DATE:	$Date: 2002/01/14 19:24:24 $
 *  REVISION:	$Revision: 1.1 $
 *
 ****************************************************************************
 */


/*
 *  Define nature of encryption keys and related tables
 */

#define TABSZ 256
#define KEYSZ 16
#define MASK 0xff

typedef char KEY[KEYSZ];

typedef	struct
	{
	KEY	password;		/* password for this session */
	char	table_a[TABSZ];		/* first translation table */
	char	table_b[TABSZ];		/* second translation table */
	char	table_c[TABSZ];		/* third translation table */
	
	} KEYDATA;

	/*  define SRPC state structure  */

typedef struct srpc_state
{
	unsigned long	srpc_id;	/*  handle - per session  */
	KEYDATA		*rcv_key;	/*  key - for reception */
	KEYDATA		*xmt_key;	/*  key - for transmission */
	unsigned long	rcv_seq;	/*  sequence number - increments  */
	unsigned long	xmt_seq;	/*  sequence number, transmit side */

}	SRPC;

	/*  values for Srpc->retcd  */

#define S_SUCCESS	0
#define S_TIMEDOUT	-1
#define S_REJECTED	-2
#define S_ABORTED	-3
#define S_FRAGMENT	-4

	/*  define miscellaneous SRPC information  */

typedef struct srpc_header
{
	unsigned long	srpc_id;	/*  SRPC handle - same per session  */
	unsigned long	sequence;	/*  SRPC seq nr - increment/match  */

}	SHDR;

typedef	struct	srpc_open
{
	SHDR		open_hdr;
	KEY		open_key;

}	SRPC_OPEN;

typedef	struct	srpc_retn
{
	SHDR		retn_hdr;
	KEY		retn_key;

}	SRPC_RETN;

#define SHDRSIZE	(sizeof(SHDR))
#define SRPC_MAX	1280

#define SRPC_DELAY	2
#define SRPC_TIMEOUT	4

extern	KEYDATA	*make_table();
