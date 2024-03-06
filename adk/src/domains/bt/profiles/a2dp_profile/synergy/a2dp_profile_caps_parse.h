/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_profile_caps_parse.h
    
DESCRIPTION
	
*/

#include "av_lib.h"
#include "a2dp_profile_caps.h"

#ifndef A2DP_CAPS_PARSE_H_
#define A2DP_CAPS_PARSE_H_


/****************************************************************************
NAME	
	appA2dpConvertUint8ToUint32

DESCRIPTION
	Make a uint32 out of consecutive array of uint8 values.

*/
uint32 appA2dpConvertUint8ToUint32(const uint8 *ptr);


/****************************************************************************
NAME	
	appA2dpConvertUint8ToUint16

DESCRIPTION
	Make a uint16 out of consecutive array of uint8 values.

*/
uint16 appA2dpConvertUint8ToUint16(const uint8 *ptr);


/****************************************************************************
NAME	
	appA2dpFindStdEmbeddedCodecCaps

DESCRIPTION
	Locates the start of the specified standard codec specific information, which may be embedded 
    within the supplied codec caps.

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool appA2dpFindStdEmbeddedCodecCaps(const uint8 **codec_caps, uint8 embedded_codec);


/****************************************************************************
NAME	
	a2dpFindNonStdEmbeddedCodecCaps

DESCRIPTION
	Locates the start of the specified non-standard codec specific information, which may be embedded 
    within the supplied codec caps.

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool a2dpFindNonStdEmbeddedCodecCaps(const uint8 **codec_caps, uint32 embedded_vendor, uint16 embedded_codec);

/****************************************************************************
NAME
    a2dpFindMatchingCodecSpecificInformation

DESCRIPTION
    Returns pointer to start of codec specific information if
    the local and remote codecs are compatible.

    IMPORTANT: It assumes the basic structure of the caps is valid. Call
    gavdpValidateServiceCaps() first to make sure.

*/
const uint8* appA2dpFindMatchingCodecSpecificInformation(const uint8 *local_caps,
                                                    const uint8 *remote_caps, bool initiating,
                                                    uint8 *error_code);


/****************************************************************************
NAME
    appA2dpValidateServiceCaps

DESCRIPTION
    Attempts to validate that a support Service Capabilities list can
    be parsed and contains reasonable values.

    This function should allow all valid values, even if the local hardware/software
    does not support them.

    It is also used to validate the caps returned by the remote device, so should not
    be dependent on local settings.

    The 'reconfigure' flag is used to adjust the validation rules depending on if
    the Capabilities supplied are complete, or part of a reconfiguration.

    When 'only_check_structure' is TRUE this function only tests that the structure
    is correct, it does not verify if mandatory entries are present.

    When 'ignore_bad_serv_category' is TRUE this function does not return an error for
    service categories that are out of spec.

    When the function returns FALSE, it will write the service category with the error
    and the AVDTP error code to error_category and error_code parameters.

*/
bool appA2dpValidateServiceCaps(const uint8 *caps, uint16 caps_size,
                                bool reconfigure, bool only_check_structure,
                                bool ignore_bad_serv_category, uint8 *error_category,
                                uint8 *error_code);


/****************************************************************************
NAME	
	a2dpFindCodecSpecificInformation

DESCRIPTION
	Finds the next codec block in a list of caps.
	Passed pointer and size are updated to point to the search result.
	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool appA2dpFindCodecSpecificInformation(const uint8 **caps, uint16 *caps_size);


/****************************************************************************
NAME	
	appA2dpAreServicesCategoriesCompatible

DESCRIPTION
    Checks the Services requested in a SET_CONFIG or RECONFIGURE command
    are supported by the local SEP.  It only checks for the Service entry
    and DOES NOT validate the actual service capabilities - that should
    be done by other functions e.g. gavdpFindMatchingCodecSpecificInformation

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool appA2dpAreServicesCategoriesCompatible(const uint8 *local_caps, uint16 local_caps_size,
                                        const uint8 *config_caps, uint16 config_caps_size,
                                        uint8 *unsupported_service);


/****************************************************************************
NAME	
	appA2dpGetContentProtection

DESCRIPTION
	Call this function to determine if Content Protection has been selected by searching
	through the configured service capabilities.
	
	
*/
a2dpContentProtection appA2dpGetContentProtection(const uint8 *ptr, const uint16 size_ptr, const uint8 **returned_caps);


/****************************************************************************
NAME	
	appA2dpIsServiceSupported

DESCRIPTION
	Call this function to determine if the specified service has been selected by searching
	through the configured service capabilities.
	
	
*/
bool appA2dpIsServiceSupported (uint8 service, const uint8 *caps, const uint16 size_caps);


#endif /* A2DP_CAPS_PARSE_H_ */
