/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  z73_drv.c
 *
 *      \author  Christian.Schuster@men.de
 *
 *      \brief   Low-level driver for Z73 module
 *
 *     Required: OSS, DESC, DBG, ID libraries
 *
 *     \switches _ONE_NAMESPACE_PER_DRIVER_
 *
 *
 *---------------------------------------------------------------------------
 * Copyright 2010-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/

 /*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "z73_int.h"

#ifdef Z73_POSCNT_24
    #ifdef Z73_SW
        static const char IdentString[]=MENT_XSTR_SFX(MAK_REVISION,Z73_POSCNT_24 swapped);
    #else
        static const char IdentString[]=MENT_XSTR_SFX(MAK_REVISION,Z73_POSCNT_24 nonswapped);
    #endif
#else
    #ifdef Z73_SW
        static const char IdentString[]=MENT_XSTR_SFX(MAK_REVISION,Z73_POSCNT_16 swapped);
    #else
        static const char IdentString[]=MENT_XSTR_SFX(MAK_REVISION,Z73_POSCNT_16 nonswapped);
    #endif
#endif

/****************************** Z73_GetEntry ********************************/
/** Initialize driver's jump table
 *
 *  \param drvP     \OUT pointer to the initialized jump table structure
 */
extern void Z73_GetEntry( LL_ENTRY* drvP )
{
    drvP->init        = Z73_Init;
    drvP->exit        = Z73_Exit;
    drvP->read        = Z73_Read;
    drvP->write       = Z73_Write;
    drvP->blockRead   = Z73_BlockRead;
    drvP->blockWrite  = Z73_BlockWrite;
    drvP->setStat     = Z73_SetStat;
    drvP->getStat     = Z73_GetStat;
    drvP->irq         = Z73_Irq;
    drvP->info        = Z73_Info;
}

/******************************** Z73_Init **********************************/
/** Allocate and return low-level handle, initialize hardware
 *
 * The function initializes all channels with the definitions made
 * in the descriptor. The interrupt is disabled.
 *
 * The following descriptor keys are used:
 *
 * \code
 * Descriptor key        Default          Range
 * --------------------  ---------------  -------------
 * DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 * DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 * ID_CHECK              1                0..1
 * \endcode
 *
 *  \param descP      \IN  pointer to descriptor data
 *  \param osHdl      \IN  oss handle
 *  \param ma         \IN  hw access handle
 *  \param devSemHdl  \IN  device semaphore handle
 *  \param irqHdl     \IN  irq handle
 *  \param llHdlP     \OUT pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z73_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE *llHdl = NULL;
    u_int32 gotsize;
    int32 error;
    u_int32 value;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
    *llHdlP = NULL;     /* set low-level driver handle to NULL */

    /* alloc */
    if ((llHdl = (LL_HANDLE*)OSS_MemGet(
                    osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

    /* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

    /* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma         = *ma;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
    /* driver's ident function */
    llHdl->idFuncTbl.idCall[0].identCall = Ident;
    /* library's ident functions */
    llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
    llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
    /* terminator */
    llHdl->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
    DBG_MYLEVEL = OSS_DBG_DEFAULT;  /* set OS specific debug level */
    DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
    /* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
        return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
                                &value, "DEBUG_LEVEL_DESC")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    DESC_DbgLevelSet(llHdl->descHdl, value);    /* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
                                &llHdl->dbgLevel, "DEBUG_LEVEL")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    if ((error = DESC_GetUInt32(llHdl->descHdl, 0,
                                &value, "Z073_INT_PRS")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    llHdl->irqEn |= (value ? Z073_IRQ_EN_PRS : 0);

    if ((error = DESC_GetUInt32(llHdl->descHdl, 0,
                                &value, "Z073_INT_REL")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    llHdl->irqEn |= (value ? Z073_IRQ_EN_REL : 0);

    if ((error = DESC_GetUInt32(llHdl->descHdl, 0,
                                &value, "Z073_INT_UP")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    llHdl->irqEn |= (value ? Z073_IRQ_EN_UP : 0);

    if ((error = DESC_GetUInt32(llHdl->descHdl, 0,
                                &value, "Z073_INT_DWN")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    llHdl->irqEn |= (value ? Z073_IRQ_EN_DWN : 0);

    if ((error = DESC_GetUInt32(llHdl->descHdl, 10,
                                &value, "Z073_STATUSQ_SIZE")) &&
        error != ERR_DESC_KEY_NOTFOUND)
        return( Cleanup(llHdl,error) );

    llHdl->statusQDepth  = (value > 1) ? value : Z073_STATUSQ_SIZE_DEF;

    DBGWRT_1((DBH, "LL - Z73_Init base addr = 0x%08x\n", llHdl->ma));



    /*------------------------------+
    |  init status queue            |
    +------------------------------*/

    if ((llHdl->statusQ = (u_int32 *)OSS_MemGet(
                    osHdl, 4*llHdl->statusQDepth, &gotsize)) == NULL)
    {
        error = ERR_OSS_MEM_ALLOC;
        return( Cleanup(llHdl,error) );
    }
    llHdl->statusQSizeGot = gotsize;
    DBGWRT_1((DBH, "LL - Z73_Init statusQ=0x%08x size=%d\n",
            (int)llHdl->statusQ, (int)llHdl->statusQSizeGot));

    /* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl->statusQ, 0x00);

    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
    /* clear and enable interrupts and position status */
    MWRITE_D32( ma, Z073_IRQ_EN, 0x00 );
    MREAD_D32( ma, Z073_POS_CNT );
    MWRITE_D32( ma, Z073_IRQ, 0xFFFFFFFF );

    *llHdlP = llHdl;    /* set low-level driver handle */

    return(ERR_SUCCESS);
}

/****************************** Z73_Exit ************************************/
/** De-initialize hardware and clean up memory
 *
 *  The function deinitializes all channels by setting them to ???.
 *  The interrupt is disabled.
 *
 *  \param llHdlP      \IN  pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z73_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
    int32 error = 0;

    DBGWRT_1((DBH, "LL - Z73_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
    /* disable interrupts */
    MWRITE_D32( llHdl->ma, Z073_IRQ_EN, 0x00 );

    /*------------------------------+
    |  clean up memory              |
    +------------------------------*/
    *llHdlP = NULL;     /* set low-level driver handle to NULL */
    error = Cleanup(llHdl,error);

    return(error);
}

/****************************** Z73_Read ************************************/
/** Read a value from the device, not supported by this driver
 *
 *  \return           \c ERR_LL_ILL_FUNC
 */
static int32 Z73_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
    return(ERR_LL_ILL_FUNC);
}

/****************************** Z73_Write ***********************************/
/** Write a value to the device, not supported by this driver
 *
 *  \return           \c ERR_LL_ILL_FUNC
 */
static int32 Z73_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
    return(ERR_LL_ILL_FUNC);
}

/****************************** Z73_SetStat *********************************/
/** Set the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl            \IN  low-level handle
 *  \param code             \IN  \ref getstat_setstat_codes "status code"
 *  \param ch               \IN  current channel
 *  \param value32_or_64    \IN  data or
 *                               pointer to block data structure (M_SG_BLOCK) for
 *                               block status codes
 *  \return           \c 0 on success or error code
 */
static int32 Z73_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
    int32 value = (int32)value32_or_64;     /* 32bit value */
    /* INT32_OR_64 valueP = value32_or_64;    stores 32/64bit pointer */
    int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - Z73_SetStat: ch=%d code=0x%04x value=0x%x\n",
              ch,code,value));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  enable interrupts        |
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
            if( value ) /* enable interrupts ?*/
            {
                MREAD_D32( llHdl->ma, Z073_POS_CNT );
                MWRITE_D32( llHdl->ma, Z073_IRQ, 0xFFFFFFFF );
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
            } else       /* disable interrupts */
            {
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, 0 );
            }
            break;
        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            llHdl->irqCount = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            if( value != M_CH_OUT )
                error = ERR_LL_ILL_DIR;
            break;
        case Z073_SIG_PRS_REL:
            if( value ) /* install signal */
            {   /* signal already installed ? */
                if( llHdl->prsRelSig ) {
                    error = ERR_OSS_SIG_SET;
                    break;
                }

                error = OSS_SigCreate( llHdl->osHdl, value, &llHdl->prsRelSig );
            } else /* clear signal */
            {
                /* signal already installed ? */
                if( llHdl->prsRelSig == NULL ) {
                    error = ERR_OSS_SIG_CLR;
                    break;
                }

                error = OSS_SigRemove( llHdl->osHdl, &llHdl->prsRelSig );
            }
            break;
        case Z073_SIG_MOVE:
            if( value ) /* install signal */
            {   /* signal already installed ? */
                if( llHdl->upDwnSig ) {
                    error = ERR_OSS_SIG_SET;
                    break;
                }

                error = OSS_SigCreate( llHdl->osHdl, value, &llHdl->upDwnSig );
            } else /* clear signal */
            {
                /* signal already installed ? */
                if( llHdl->upDwnSig == NULL ) {
                    error = ERR_OSS_SIG_CLR;
                    break;
                }

                error = OSS_SigRemove( llHdl->osHdl, &llHdl->upDwnSig );
            }
            break;
        case Z073_INT_PRS:
            if( value && !(llHdl->irqEn & Z073_IRQ_EN_PRS) )
            {  /* enable interrupt */
                llHdl->irqEn |= Z073_IRQ_EN_PRS;
                if( (MREAD_D32( llHdl->ma, Z073_IRQ_EN ) & Z073_IRQ_EN_ALL) )
                { /* only enable irqs if already enabled */
                    MWRITE_D32( llHdl->ma, Z073_IRQ, Z073_IRQ_PRS );
                    MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
                }
            } else if( !value )  /* disable interrupt */
            {
                llHdl->irqEn &= ~Z073_IRQ_EN_PRS;
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
            }
            break;
        case Z073_INT_REL:
            if( value && !(llHdl->irqEn & Z073_IRQ_EN_REL) )
            {  /* enable interrupt */
                llHdl->irqEn |= Z073_IRQ_EN_REL;
                if( (MREAD_D32( llHdl->ma, Z073_IRQ_EN ) & Z073_IRQ_EN_ALL) )
                { /* only enable irqs if already enabled */
                    MWRITE_D32( llHdl->ma, Z073_IRQ, Z073_IRQ_REL );
                    MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
                }
            } else if( !value )  /* disable interrupt */
            {
                llHdl->irqEn &= ~Z073_IRQ_EN_REL;
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
            }
            break;
        case Z073_INT_UP:
            if( value && !(llHdl->irqEn & Z073_IRQ_EN_UP) )
            {  /* enable interrupt */
                llHdl->irqEn |= Z073_IRQ_EN_UP;
                if( (MREAD_D32( llHdl->ma, Z073_IRQ_EN ) & Z073_IRQ_EN_ALL) )
                { /* only enable irqs if already enabled */
                    MWRITE_D32( llHdl->ma, Z073_IRQ, Z073_IRQ_UP );
                    MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
                }
            } else if( !value )  /* disable interrupt */
            {
                llHdl->irqEn &= ~Z073_IRQ_EN_UP;
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
            }
            break;
        case Z073_INT_DWN:
            if( value && !(llHdl->irqEn & Z073_IRQ_EN_DWN) )
            {  /* enable interrupt */
                llHdl->irqEn |= Z073_IRQ_EN_DWN;
                if( (MREAD_D32( llHdl->ma, Z073_IRQ_EN ) & Z073_IRQ_EN_ALL) )
                { /* only enable irqs if already enabled */
                    MWRITE_D32( llHdl->ma, Z073_IRQ, Z073_IRQ_DWN );
                    MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
                }
            } else if( !value )  /* disable interrupt */
            {
                llHdl->irqEn &= ~Z073_IRQ_EN_DWN;
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, llHdl->irqEn );
            }
            break;
       /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

    return(error);
}

/****************************** Z73_GetStat *********************************/
/** Get the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl            \IN  low-level handle
 *  \param code             \IN  \ref getstat_setstat_codes "status code"
 *  \param ch               \IN  current channel
 *  \param value32_or_64P   \IN  pointer to block data structure (M_SG_BLOCK) for
 *                               block status codes
 *  \param value32_or_64P   \OUT data pointer or pointer to block data structure
 *                               (M_SG_BLOCK) for block status codes
 *
 *  \return           \c ERR_SUCCESS on success or error code
 */
static int32 Z73_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
    /* pointer to 32bit value  */
    int32       *valueP       = (int32*)value32_or_64P;
    /* stores 32/64bit pointer  */
    INT32_OR_64 *value64P     = value32_or_64P;
    /*   stores block struct pointer */
    /* M_SG_BLOCK   *blk          = (M_SG_BLOCK*)value32_or_64P;*/
    int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - Z73_GetStat: ch=%d code=0x%04x\n",
              ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_OUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 32;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_BINARY;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = llHdl->irqCount;
            break;
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
            *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
            break;
        /*--------------------------+
        |   get status              |
        +--------------------------*/
        case Z073_STATUS:
        {
            OSS_IRQ_STATE irqState;
            *valueP = 0; /* in case off error return value might be parsed */

            if( llHdl->error ) /* errors have higher priority than data */
            {
                error = llHdl->error;
                llHdl->error = 0;
                DBGWRT_ERR((DBH, "*** LL - Z73_GetStat(STATUS): 0x%04x\n", error));
                break;
            }

            if( llHdl->statusQIn == llHdl->statusQOut ) /* empty queue */
            {
                irqState = OSS_IrqMaskR( llHdl->osHdl, llHdl->irqHdl );
                error  = getStatus( llHdl );
                OSS_IrqRestore( llHdl->osHdl, llHdl->irqHdl, irqState );

                if( llHdl->statusQIn == llHdl->statusQOut ) /* still empty */
                {
                    error = Z073_ERR_NO_STATUS;
                    DBGWRT_3((DBH, "LL - Z73_GetStat(STATUS): no new events\n", error));
                    break;
                }
            }

            *valueP = (int32)llHdl->statusQ[llHdl->statusQOut++];

            if( llHdl->statusQOut == llHdl->statusQDepth )
                llHdl->statusQOut = 0;


            break;
        }
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

    return(error);
}

/******************************* Z73_BlockRead ******************************/
/** Read a data block from the device, not supported by this driver
 *
 *  \return            \c ERR_LL_ILL_FUNC
 */
static int32 Z73_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    DBGWRT_1((DBH, "LL - Z73_BlockRead: not supported \n"));

    /* return number of read bytes */
    *nbrRdBytesP = 0;

    return(ERR_LL_ILL_FUNC);
}

/****************************** Z73_BlockWrite *****************************/
/** Write a data block from the device, not supported by this driver
 *
 *  \return           \c ERR_LL_ILL_FUNC
 */
static int32 Z73_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
    /* return number of written bytes */
    *nbrWrBytesP = 0;

    return(ERR_LL_ILL_FUNC);
}


/****************************** Z73_Irq ************************************/
/** Interrupt service routine
 *
 *  The interrupt is triggered when ??? occurs.
 *
 *  If the driver can detect the interrupt's cause it returns
 *  LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *
 *  \param llHdl       \IN  low-level handle
 *  \return LL_IRQ_DEVICE   irq caused by device
 *          LL_IRQ_DEV_NOT  irq not caused by device
 *          LL_IRQ_UNKNOWN  unknown
 */
static int32 Z73_Irq(
   LL_HANDLE *llHdl
)
{
    u_int32 irqReg = 0;
    int32 getStatusError = ERR_SUCCESS;

    IDBGWRT_1((DBH, ">>> Z73_Irq\n"));

    irqReg = MREAD_D32( llHdl->ma, Z073_IRQ );

    irqReg &= llHdl->irqEn; /* only consider bits where irq enabled */
    if( irqReg  )
    {
        if( irqReg & (Z073_IRQ_PRS | Z073_IRQ_REL) )
        {
            if( (getStatusError = getStatus( llHdl )) )
            {
                llHdl->error = getStatusError;
                /* disable interrupts */
                MWRITE_D32( llHdl->ma, Z073_IRQ_EN, 0x00 );
                IDBGWRT_ERR((DBH, ">>>*** Z73_Irq: Queue Full, all interrupts disabled!!\n"));
            }

            /* if requested send signal to application */
            if( llHdl->prsRelSig ) {
                OSS_SigSend( llHdl->osHdl, llHdl->prsRelSig );
            }
            MWRITE_D32( llHdl->ma, Z073_IRQ, irqReg &
                                             (Z073_IRQ_PRS | Z073_IRQ_REL) );
        } else
        {
            /* if requested send signal to application */
            if( llHdl->upDwnSig ) {
                OSS_SigSend( llHdl->osHdl, llHdl->upDwnSig );
            }
            MWRITE_D32( llHdl->ma, Z073_IRQ, irqReg &
                                             (Z073_IRQ_UP | Z073_IRQ_DWN) );
        }
    } else
    {
        return( LL_IRQ_DEV_NOT );
    }

    llHdl->irqCount++;
    return(LL_IRQ_DEVICE);
}

/****************************** Z73_Info ***********************************/
/** Get information about hardware and driver requirements
 *
 *  The following info codes are supported:
 *
 * \code
 *  Code                      Description
 *  ------------------------  -----------------------------
 *  LL_INFO_HW_CHARACTER      hardware characteristics
 *  LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *  LL_INFO_ADDRSPACE         address space information
 *  LL_INFO_IRQ               interrupt required
 *  LL_INFO_LOCKMODE          process lock mode required
 * \endcode
 *
 *  The LL_INFO_HW_CHARACTER code returns all address and
 *  data modes (ORed) which are supported by the hardware
 *  (MDIS_MAxx, MDIS_MDxx).
 *
 *  The LL_INFO_ADDRSPACE_COUNT code returns the number
 *  of address spaces used by the driver.
 *
 *  The LL_INFO_ADDRSPACE code returns information about one
 *  specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *  data mode represents the widest hardware access used by
 *  the driver.
 *
 *  The LL_INFO_IRQ code returns whether the driver supports an
 *  interrupt routine (TRUE or FALSE).
 *
 *  The LL_INFO_LOCKMODE code returns which process locking
 *  mode the driver needs (LL_LOCK_xxx).
 *
 *  \param infoType    \IN  info code
 *  \param ...         \IN  argument(s)
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z73_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
        /*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
        {
            u_int32 *addrModeP = va_arg(argptr, u_int32*);
            u_int32 *dataModeP = va_arg(argptr, u_int32*);

            *addrModeP = MDIS_MA08;
            *dataModeP = MDIS_MD08 | MDIS_MD16;
            break;
        }
        /*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
        {
            u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

            *nbrOfAddrSpaceP = ADDRSPACE_COUNT;
            break;
        }
        /*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
        {
            u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
            u_int32 *addrModeP = va_arg(argptr, u_int32*);
            u_int32 *dataModeP = va_arg(argptr, u_int32*);
            u_int32 *addrSizeP = va_arg(argptr, u_int32*);

            if (addrSpaceIndex >= ADDRSPACE_COUNT)
                error = ERR_LL_ILL_PARAM;
            else {
                *addrModeP = MDIS_MA08;
                *dataModeP = MDIS_MD16;
                *addrSizeP = ADDRSPACE_SIZE;
            }

            break;
        }
        /*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
        {
            u_int32 *useIrqP = va_arg(argptr, u_int32*);

            *useIrqP = USE_IRQ;
            break;
        }
        /*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
        {
            u_int32 *lockModeP = va_arg(argptr, u_int32*);

            *lockModeP = LL_LOCK_CALL;
            break;
        }
        /*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ***********************************/
/** Return ident string
 *
 *  \return            pointer to ident string
 */
static char* Ident( void )
{
	return( (char*) IdentString );
}

/********************************* Cleanup *********************************/
/** Close all handles, free memory and return error code
 *
 *  \warning The low-level handle is invalid after this function is called.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param retCode    \IN  return value
 *
 *  \return           \IN   retCode
 */
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode
)
{
    DBGWRT_1((DBH, "Z73 Cleanup: statusQ*=0x%08x   size=%d llHdl*=0x%08p\n",
     (int)llHdl->statusQ, (int)llHdl->statusQSizeGot, llHdl));
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
    /* clean up desc */
    if (llHdl->descHdl)
        DESC_Exit(&llHdl->descHdl);

    /* clean up debug */
    DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free status queue */
    if( llHdl->statusQ )
        OSS_MemFree(llHdl->osHdl, (int8*)llHdl->statusQ, llHdl->statusQSizeGot);

    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
    return(retCode);
}

static int32 getStatus( LL_HANDLE* llHdl )
{
    u_int32 retVal = 0;
    OSS_IRQ_STATE irqState;
    u_int32 curPosStat, curIrqStat;
    int32 error = ERR_SUCCESS;

    irqState = OSS_IrqMaskR( llHdl->osHdl, llHdl->irqHdl );

    /* detect full queue and abort if necessary */
    if( ( (llHdl->statusQIn + 1) == llHdl->statusQOut) ||
        (((llHdl->statusQIn + 1) == llHdl->statusQDepth) &&
         (llHdl->statusQOut == 0)) )
    {
        error = Z073_ERR_STATUSQ_FULL;
        goto ERR_EXIT;
    }

    curPosStat = MREAD_D32( llHdl->ma, Z073_POS_CNT );
    curIrqStat = MREAD_D32( llHdl->ma, Z073_IRQ );

    DBGWRT_1((DBH, "Z73 getStatus: curPosStat=0x%08X\n",curPosStat));

    retVal |= (curIrqStat & Z073_IRQ_PRS) ? Z073_STATUS_PRS : 0;
    retVal |= (curIrqStat & Z073_IRQ_REL) ? Z073_STATUS_REL : 0;
    retVal |= (curPosStat & Z073_POS_CNT_STS) ? (Z073_STATUS_MOV |
                                                (curPosStat & Z073_POS_CNT_CNT))
                                              : 0;

    /* also get current status of inputs for debug purposes ? */
    retVal |= curIrqStat&(Z073_IRQ_STS_PRSREL|Z073_IRQ_STS_B|Z073_IRQ_STS_A)<<24;

    /* place in Q, one field will never get filled, otherwise more flags are
     * needed to detect full queue */
    if( retVal & (Z073_STATUS_PRS | Z073_STATUS_REL | Z073_STATUS_MOV) )
        llHdl->statusQ[llHdl->statusQIn++] = retVal;

    if( llHdl->statusQIn == llHdl->statusQDepth )
        llHdl->statusQIn = 0;

    /* status reported, clear bits */
    MWRITE_D32(llHdl->ma, Z073_IRQ, Z073_IRQ_PRS | Z073_IRQ_REL |
                                    Z073_IRQ_UP  | Z073_IRQ_UP );

ERR_EXIT:
    OSS_IrqRestore( llHdl->osHdl, llHdl->irqHdl, irqState );
    return( error );
}


