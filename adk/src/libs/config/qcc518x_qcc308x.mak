# Pull in the Kymera build configuration
-include config/hydracore.mak

CONFIG_DIRS_FILTER := $(CONFIG_DIRS_FILTER) nfc_api nfc_cl 

TEMP := $(CONFIG_DIRS_FILTER)
CONFIG_DIRS_FILTER = $(filter-out anc, $(TEMP))

CONFIG_FEATURES+=CONFIG_QCC518X_QCC308x CONFIG_KEY_MANIPULATION CONFIG_HANDOVER

-include config/synergy.mak
