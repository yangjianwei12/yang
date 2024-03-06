import os
from os import path

# P4 Environment variables for accessing server
P4_ENV_VAR = {'port': "banunxp4p01.asia.root.pri:1666",
	'user': "ss83",                                 # ToDo: Put your own perforce user here, this user is the one querying the perforce
	'client': "ss83_qbl_main_syn_main_partial2"}                         # ToDo: Change the client name

BRANCH_VIEW_FILE = "Syn2VmBranchView"
BRANCH_VIEW_BOILERPLATE = "Branch:  " + BRANCH_VIEW_FILE + "\n\nOwner:  hr03\n\n\n\nOptions:    unlocked\n\nView:"
TARGET_FOLDER = os.getcwd()                         # Copy into current working directory