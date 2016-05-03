#************************** MDIS5 device descriptor *************************
#
#        Author: Christian.Schuster@men.de
#         $Date: 2010/04/21 15:48:20 $
#     $Revision: 1.2 $
#
#   Description: Metadescriptor for Z73, all available keys
#
#****************************************************************************

qdec_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1     # descriptor type (1=device)
    HW_TYPE          = STRING   Z73   # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   chameleon_pcitbl_1    # device name of baseboard
    DEVICE_SLOT      = U_INT32  0                # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	debug levels (optional)
	#   this keys have only effect on debug drivers
	#------------------------------------------------------------------------
    DEBUG_LEVEL      = U_INT32  0xc0008000  # LL driver
    DEBUG_LEVEL_MK   = U_INT32  0xc0008000  # MDIS kernel
    DEBUG_LEVEL_OSS  = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC = U_INT32  0xc0008000  # DESC calls
    DEBUG_LEVEL_MBUF = U_INT32  0xc0008000  # MBUF calls

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------

	# Press push button interrupt
    # 0 := disabled
    # 1 := enabled
    Z073_INT_PRS = U_INT32 0

    # Release push button interrupt
    # 0 := disabled
    # 1 := enabled
    Z073_INT_REL = U_INT32 0

    # Button move up interrupt
    # 0 := disabled
    # 1 := enabled
    Z073_INT_UP = U_INT32 0

    # Button move down interrupt
    # 0 := disabled
    # 1 := enabled
    Z073_INT_DWN = U_INT32 0

    # Size of status queue,
    # mainly relevant on none realtime operating systems
    Z073_STATUSQ_SIZE = U_INT32 5
}
