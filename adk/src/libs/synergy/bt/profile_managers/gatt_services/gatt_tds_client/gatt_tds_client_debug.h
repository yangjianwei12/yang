/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#ifndef GATT_TDS_CLIENT_DEBUG_H_
#define GATT_TDS_CLIENT_DEBUG_H_


#ifdef LEA_TDS_LOG_ENABLE  /* Logging enable is not complete */

#define GATT_TDS_CLIENT_INFO(...) CSR_LOG_TEXT_INFO((CsrBtMcsLto, 0, __VA_ARGS__))
#define GATT_TDS_CLIENT_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtMcsLto, 0, __VA_ARGS__))
#define GATT_TDS_CLIENT_ERROR(...)  CSR_LOG_TEXT_ERROR((CsrBtMcsLto, 0, __VA_ARGS__))
#define GATT_TDS_CLIENT_PANIC(...)  {CSR_LOG_TEXT_ERROR((CsrBtMcsLto, 0, __VA_ARGS__)); CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#else

#define GATT_TDS_CLIENT_INFO(...)
#define GATT_TDS_CLIENT_WARNING(...)
#define GATT_TDS_CLIENT_ERROR(...)
#define GATT_TDS_CLIENT_PANIC(...)  {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#endif

#endif /* GATT_TDS_CLIENT_DEBUG_H_ */
