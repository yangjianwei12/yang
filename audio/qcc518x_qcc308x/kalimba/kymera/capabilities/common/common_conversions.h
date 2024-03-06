/****************************************************************************
 * Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/

#ifndef COMMON_CONVERSIONS_H
#define COMMON_CONVERSIONS_H

/*****************************************************************************
Public Constant Declarations
*/

/* The common OPMSG to set sample rate uses a falue of fs/25
 * This allows all standard rates to be defined while still fitting in 16 bits 
 */
#define CONVERSION_SAMPLE_RATE_TO_HZ   (25)

/* Macro to extract the relevant message field and convert to Hz */
#define SAMPLE_RATE_FROM_COMMON_OPMSG(message_data) (CONVERSION_SAMPLE_RATE_TO_HZ * OPMSG_FIELD_GET(message_data, OPMSG_COMMON_MSG_SET_SAMPLE_RATE, SAMPLE_RATE))

/*****************************************************************************
Public Function Declarations
*/
/** Converts 60ths of a dB to a linear fractional gain */
extern unsigned dB60toLinearFraction(int db);

/** Converts 60ths of a dB to a linear Q5.N fractional gain */
extern unsigned dB60toLinearQ5(int db);

/** Converts a linear fractional gain to 60ths of a dB*/
extern unsigned gain_linear2dB60(int db);

#endif /* COMMON_CONVERSIONS_H */
