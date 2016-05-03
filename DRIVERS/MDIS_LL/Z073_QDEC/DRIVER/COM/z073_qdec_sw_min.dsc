#************************** MDIS5 device descriptor *************************
#
#        Author: Christian.Schuster@men.de
#         $Date: 2010/04/21 15:48:27 $
#     $Revision: 1.2 $
#
#   Description: Metadescriptor for Z73, minimum, swapped variant
#
#****************************************************************************

qdec_sw_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1          # descriptor type (1=device)
    HW_TYPE          = STRING   Z73_SW     # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   chameleon_pcitbl_1    # device name of baseboard
    DEVICE_SLOT      = U_INT32  0                # used slot on baseboard (0..n)
}
