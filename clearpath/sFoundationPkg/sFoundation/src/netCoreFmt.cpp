/****************************************************************************
 * $Archive: /ClearPath SC/User Driver/src/netCoreFmt.cpp $
 *  $Date: 01/19/2017 17:39 $
 *
 * !NAME!
 *		netCoreFmt
 *
 * DESCRIPTION:
 *		This module defines a set of utilities that allow the creation of
 *		<packetbuf> objects for the core command set. These form the
 *		basis of the Set and Get Parameter commands	utilized by all
 *		node devices. 
 *		
 * CREATION DATE:
 *		June 23, 1998  18:34:12 12:22:30
 *																	  	 !-8!
 * COPYRIGHT NOTICE:
 *		(C)Copyright 1998  Teknic, Inc.  All rights reserved.
 *
 *		This copyright notice must be reproduced in any copy, modification, 
 *		or portion thereof merged into another program. A copy of the 
 *		copyright notice must be included in the object library of a user
 *		program.
 *
 * !end!																    *
 ****************************************************************************/



/****************************************************************************
 * !NAME!																    *
 * 	netCore.c headers
 *		
																!0! */
	 #include "mnParamDefs.h"
	 #include "meridianHdrs.h"
	 #include "netCmdPrivate.h"
	 #include "netCmdAPI.h"
	 #include "sFoundResource.h"
/* 																	   !end!*
 ****************************************************************************/




/****************************************************************************
 * !NAME!																	*
 * 	netCore.c constants
 *
 * !PATHS!
 *		constants\all
 *		constants\netCore
 * !0!																		*/

/* !end!																	*
 ****************************************************************************/



/****************************************************************************
 * !NAME!																	*
 * 	netCore.c static variables
 *
 * !PATHS!
 *		statics\all
 *		statics\netCore
 * !0!																		*/
/* !end!																	*
 ****************************************************************************/



/*****************************************************************************
 *	!NAME!
 *		netGetParameterFmt
 *		
 *	DESCRIPTION:
 *		This function will format the <frameBuf> with the get parameter
 *		command for device <theAddr> at parameter index <theParam>.
 *
 *	RETURNS:
 *		cnErrCode error code
 *		
 *	SYNOPSIS: 															    */
cnErrCode MN_DECL netGetParameterFmt(
		packetbuf *theCmd,			// ptr to the frame buffer to format
		nodeaddr theNode, 			// device address
		nodeparam theParamNum)		// parameter index
		
{
	appNodeParam param;
	param.bits = theParamNum;		// Init the field cracker

	// Stuff the frame with this command
	switch(param.fld.bank) {
		case 0:
			theCmd->Byte.Buffer[CMD_LOC] = MN_CMD_GET_PARAM0;
			break;
		case 1:
			theCmd->Byte.Buffer[CMD_LOC] = MN_CMD_GET_PARAM1;
			break;
		case 2:
			theCmd->Byte.Buffer[CMD_LOC] = MN_CMD_GET_PARAM2;
			break;
		default:
			return (MN_ERR_PARAM_RANGE);
	}
	theCmd->Fld.Addr = theNode;
	theCmd->Fld.PktLen = 2;
	theCmd->Byte.Buffer[CMD_LOC+1] = (nodechar)(0xff & theParamNum);
	return(MN_OK);
}
/* 																 !skip end! */
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		netGetParameterExtract
 *		
 *	DESCRIPTION:
 *		This function will extract returned parameter from the <theResp>
 *		into the raw area of the parameter database, stripping it of the
 *		response header.
 *
 *	RETURNS:
 *		cnErrCode error code
 *		
 *	SYNOPSIS: 															    */
cnErrCode MN_DECL netGetParameterExtract(
		packetbuf *theResp,			// ptr to the frame buffer to format
		packetbuf *paramRaw)		// ptr to length and data itself
		
{
	// Make copy of parameter to return
	if (theResp->Byte.BufferSize > MN_API_PACKET_MAX)
		return(MN_ERR_FRAME);
	paramRaw->Byte.BufferSize = theResp->Byte.BufferSize - MN_API_PACKET_HDR_LEN;
	cpymem((nodeuchar *)&theResp->Byte.Buffer[RESP_LOC],
		   (nodeuchar *)&paramRaw->Byte.Buffer[0],
		   paramRaw->Byte.BufferSize);
	return MN_OK;
}
/* 																 !skip end! */
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		netSetParameterFmt
 *		
 *	DESCRIPTION:
 *		This function will format the <theCmd> with the set parameter
 *		command for device <theNode> at parameter index <theParam>. The
 *		parameter value is defined in the <paramRaw> area and the <paramSize>
 *		field defines the actual size.
 *
 *	RETURNS:
 *		cnErrCode error code
 *		
 *	SYNOPSIS: 															    */
cnErrCode MN_DECL netSetParameterFmt(
		packetbuf *theCmd,			// ptr to the frame buffer to format
		nodeaddr theNode, 			// node address
		nodeparam theParamNum, 		// parameter index
		packetbuf *paramRaw)		// ptr to new parameter data
{
	appNodeParam param;
	// Stuff the frame with this command if room for cmd and parameter #
	if (paramRaw->Byte.BufferSize > MN_API_PAYLOAD_MAX-2) {
		return (MN_ERR_PARAM_RANGE);
	}
		
	param.bits = theParamNum;		// Init the field cracker

	// Stuff the frame with this command
	switch(param.fld.bank) {
		case 0:
			theCmd->Byte.Buffer[CMD_LOC] = MN_CMD_SET_PARAM0;
			break;
		case 1:
			theCmd->Byte.Buffer[CMD_LOC] = MN_CMD_SET_PARAM1;
			break;
		case 2:
			theCmd->Byte.Buffer[CMD_LOC] = MN_CMD_SET_PARAM2;
			break;
		default:
			return (MN_ERR_PARAM_RANGE);
	}
	theCmd->Fld.Addr = theNode;
	theCmd->Fld.PktLen = 2+paramRaw->Byte.BufferSize;
	theCmd->Byte.Buffer[CMD_LOC+1] = (nodechar)(0xff&theParamNum);
	// Copy info into command frame
	cpymem((nodeuchar *)&paramRaw->Byte.Buffer[0],
		   (nodeuchar *)&theCmd->Byte.Buffer[CMD_LOC+2],
		   paramRaw->Byte.BufferSize); 
	return (MN_OK);
}
/* 																 !skip end! */
/*																 !end!		*/
/****************************************************************************/


///*****************************************************************************
// *	!NAME!
// *		coreGoFmt
// *
// *	DESCRIPTION:
// *		Format a Go control frame.
// *		
// *		NOTE: this command must be sent via the low-level transmission
// *		function to preserve the cc_err field.
// *
// *	RETURNS:
// *		standard error code
// *
// *	SYNOPSIS: 															    */
//MN_EXPORT cnErrCode MN_DECL coreGoFmt(
//		packetbuf *theCmd,			// ptr to a command buffer
//		nodeaddr theNode) 			// node address
//{
//	theCmd->Fld.Src = MN_SRC_HOST;
//	theCmd->Fld.PktType = MN_PKT_TYPE_TRIGGER;
//	theCmd->Fld.Addr = theNode;
//	theCmd->Fld.PktLen = 0;
//	theCmd->Fld.Mode = 0;					// Fill in unused with 0
//	theCmd->bufferSize = 2;						// This is always length 2
//	return(MN_OK);
//}
///* 																 !skip end! */
///*																 !end!		*/
///****************************************************************************/
//


///*****************************************************************************
// *	!NAME!
// *		coreNetFlushFmt
// *
// *	DESCRIPTION:
// *		Format the command buffer with the command that flushes all regular
// *		
// *
// *		NOTE: this command must be sent via the low-level transmission
// *		function to preserve the cc_err field.
// *
// *	RETURNS:
// *		standard error return
// *		
// *	SYNOPSIS: 															    */
//MN_EXPORT cnErrCode MN_DECL coreNetFlushFmt(
//		packetbuf *theCmd,			// ptr to a command buffer
//		nodeaddr theNode) 			// node address
//{
//	theCmd->Fld.Src = MN_SRC_HOST;
//	theCmd->Fld.PktType = MN_PKT_TYPE_EXTEND_HIGH;
//	theCmd->Fld.Addr = theNode;
//	theCmd->Fld.PktLen = 1;
//	theCmd->u.fld.unused1 = 0;					// Fill in unused with 0
//	theCmd->Byte.Buffer[2] = ND_CTL_EXT_NET_FLUSH;
//	theCmd->bufferSize = 3;						// This is always length 3
//	return(MN_OK);
//}
///* 																 !skip end! */
///*																 !end!		*/
///****************************************************************************/



/*========================================================================== 
	END OF FILE netCoreFmt.c
  ==========================================================================*/
