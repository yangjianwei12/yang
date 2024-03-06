/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_noise_id.c
\brief     ANC Noise Id: Seamless filter switch to the reported noise category
*/

#ifndef ANC_NOISE_ID_H
#define ANC_NOISE_ID_H

/* Compiler error checks for macros that are supported under ANC V2 only */
#ifndef INCLUDE_ANC_V2
    #if defined(ENABLE_ANC_NOISE_ID)
        #error ENABLE_ANC_NOISE_ID  can be used along with INCLUDE_ANC_V2 only
    #endif
#endif

/*! \brief Tunable ANC Noise ID category */
typedef enum
{    
    ANC_NOISE_ID_CATEGORY_A, /*Mapped to KYMERA_NOISE_ID_CATEGORY_0*/
    ANC_NOISE_ID_CATEGORY_B, /*Mapped to KYMERA_NOISE_ID_CATEGORY_1*/
    ANC_NOISE_ID_CATEGORY_NA = 0xFF
        
} anc_noise_id_category_t;

typedef struct
{
    uint8 new_category;
}ANC_NOISE_ID_CATEGORY_CHANGE_IND_T;

/*!
    \brief API to init ANC Noise Id
    \param None
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncNoiseId_Init(void);
#else
#define AncNoiseId_Init() ((void)(0))
#endif


/*!
    \brief API to identify noise Id feature support
    \param None
    \return TRUE for feature supported otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool AncNoiseId_IsFeatureSupported(void);
#else
#define AncNoiseId_IsFeatureSupported() (FALSE)
#endif


/*!
    \brief API to identify if noise Id feature is enabled by user or not
    \param None
    \return TRUE for feature enabled otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool AncNoiseId_IsFeatureEnabled(void);
#else
#define AncNoiseId_IsFeatureEnabled() (FALSE)
#endif


/*!
    \brief API to enable or disable the noise Id feature
    \param Enable/Disable TRUE for feature enabled 
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncNoiseId_Enable(bool enable);
#else
#define AncNoiseId_Enable(enable) (UNUSED(enable))
#endif

/*!
    \brief API to action the mode change based on requested noise category
    \param None 
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncNoiseId_TriggerModeChange(void);
#else
#define AncNoiseId_TriggerModeChange(void)  ((void)(0))
#endif


/*!
\brief API to handle category change
\param None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncNoiseId_HandleCategoryChange(anc_noise_id_category_t requested_noise_category);
#else
#define AncNoiseId_HandleCategoryChange(x) ((void)(0))
#endif

/*!
    \brief API to check if mode change is requested due to noise ID change
    \param None 
    \return None
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool AncNoiseId_IsModeChangeRequested(void);
#else
#define AncNoiseId_IsModeChangeRequested() (FALSE)
#endif

/*!
    \brief API to check if a given Noise ID category is valid or not
    \param Noise ID Category
    \return TRUE if category is valid otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool AncNoiseId_IsValidCategory(anc_noise_id_category_t requested_noise_category);
#else
#define AncNoiseId_IsValidCategory(requested_noise_category) (FALSE)
#endif

/*!
    \brief API to check if noise ID can be enabled
    \param None
    \return TRUE if can be enabled otherwise FALSE
*/
#ifdef ENABLE_ADAPTIVE_ANC
bool AncNoiseId_CanProcess(void);
#else
#define AncNoiseId_CanProcess(void) ((void) (0))
#endif

/*!
    \brief Updates the task data with the session data stored in PS.
           If PS is empty, uses default values in session_data
    \param Noise ID Enable
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncNoiseId_UpdateTaskData(bool noise_id_enabled);
#else
#define AncNoiseId_UpdateTaskData(x) ((void)(0*x))
#endif

/*!
    \brief Stores Noise ID session data in PS.
    \param Noise ID config data to be stored in PS
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncNoiseId_UpdateSessionData(bool *noise_id_enabled);
#else
#define AncNoiseId_UpdateSessionData(x) ((void)(0*x))
#endif


#endif /* ANC_NOISE_ID_H */
