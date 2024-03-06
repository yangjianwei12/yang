#--------------------------------------------------------------------------------------------------
# Copyright (c) 2022 Qualcomm Technologies International, Ltd
#
# This utlity parses a set of headers and generates enum declarations based on the
# macros in the files.
# 
# This is how the script works
# =============================
# 1. Parse file and look for following pattern
#     "define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE ((CsrBtCmEventMask) 0x00000000)"
# 2. Extract CsrBtCmEventMask as an enum and CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE as a macro
# 3. Save these enums and corresponding macros in a dictionary called tokens,
#      tokens = {'enum1':[macro1, macro2], 'enum2':[macro3, macro4, macro5]}
# 4. Tokens dictionary is then used to generate header files with enum definitions, in following format,
#    enum <enum name> {
#        new_macro1 = <macro1>,
#        new_macro2 = <macro2>,
#    };
#
# Running the script
# ===================
# Script is located at adk/src/libs/synergy/tools/enum_generator.py
# It is automatically triggered during build. We can manually run it from libs/synergy folder as,
# $>python tools\enum_generator.p
#
# It assumes following - 
#  1. List of headers required for enum generation are provided by this file,
#      "adk/src/libs/synergy/tools/interface_list.xml"
#     Blacklisted interfaces are also defined in this file.
#     (variable INTERFACE_LIST holds location of this file in the script)
#  2. Generated headers with enum definitions are placed in the same directory as the interface header.
#  3. Generated headers have this nomenclature currently,
#     <interface header name>_enum_dbg.h
#     (this will probably get changed in the course of review)
#
# Blacklisting : 
# ===============
# We may not want all interfaces to be enumerized. In such cases we can add them in
# a blacklist so that they are ignored by the script.
#
# Nomenclature of generated enum constants
# =========================================
# all generated enum constants are prefixed with "_". Variable ENUM_CONST_PREFIX can be modified to 
# change the prefix.
#
# Preserve Macros
# ================
# In order to use the newly defined enum constants in debug logs, we need these enums to be preserved 
# in the elf file.
# This is done using this macro,
# LOGGING_PRESERVE_MESSAGE_ENUM(<enum name>)
#
# This script generates a source file (pointed by variable PRESERVE_SOURCE_FILE) which contains calls to 
# this macros for each generated enumeration. 
#--------------------------------------------------------------------------------------------------
import os
import io
import re
import pprint
import filecmp
import xml.etree.ElementTree as ET
import datetime
from os.path import exists

#Configuration
INTERFACE_LIST = './/tools//interface_list.xml'
DEBUG_ENABLED = False
ENUM_CONST_PREFIX = '_'
ENUM_FILE_NAME_SUFFIX = "_enum_dbg"
PUBLIC_HEADERS_DIR_PATH = './/bt//inc'
LOGFILE = 'tmp_enum_gen_output'
TMP_HEADER = 'temp_enum_hdr'
TMP_SOURCE = 'temp_preserve_src'
PRESERVE_SOURCE_FILE = './/bt//profile_managers//common//code//util_preserve_gen_enums.c'

#Global Data
blacklisted_tokens = []         #Blacklisted macros. These should not be generated
dropped_enum_tokens = set([])   #Enums which were not generated
dropped_macro_tokens = set([])  #Macros which were not generated
generated_files = []            #Generated files
output = open(LOGFILE, 'w')

#Regular Expression search strings
#Function to extract the string from a regular expression match
def extract(match):
    return "("+match+")"

DOT = '\.'
UNDERSCORE = '_'
HASHDEFINE = "#define"
ONE_OR_MORE_SPACES = "\s+"
NAME = "[a-zA-Z_0-9]+" #enclosed within () as we want to retrieve this name after pattern match
BRACKET_OPEN = "\("
BRACKET_CLOSE = "\)"
ONE_OR_MORE_CHARS = ".+"

#SEARCH_PATTERN =  "define <name> ((<typename>) value)"
#e.g."define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE ((CsrBtCmEventMask) 0x00000000)"
SEARCH_PATTERN = HASHDEFINE + ONE_OR_MORE_SPACES + extract(NAME) + ONE_OR_MORE_SPACES + BRACKET_OPEN + BRACKET_OPEN +\
  extract(NAME) + BRACKET_CLOSE + ONE_OR_MORE_CHARS + BRACKET_CLOSE

#--------------------------------------------------------------------------------------------
# General utilities
#--------------------------------------------------------------------------------------------
def DEBUG(s):
    if DEBUG_ENABLED == True:
        PRINT_LOGFILE(s)

def PRINT_LOGFILE(s):
    global output
    output.write(s)

#Print all enums stored in tokens dictionary
def print_enums(tokens):
    for t in tokens:
        enum_print = generate_enum(t, tokens[t])
        PRINT_LOGFILE(enum_print)

#Get current year
def get_current_year():
    current_date = datetime.datetime.now()
    date = current_date.date()
    year = date.strftime("%Y")
    return year

#Store key, value in dictionary
def store_in_dictionary(dic, key, value):
    if key in dic.keys():
        dic[key].append(value)
    else:
        dic[key] = [value]

#--------------------------------------------------------------------------------------------
# Header Parsing utilities
#--------------------------------------------------------------------------------------------
#Macro to enum constant name conversion
def rename(name):
    return ENUM_CONST_PREFIX + name

#Extract enum name from typename. Handle any exceptions
def get_enum_name(group, token):
    if token.find('CsrBtResultCode') != -1:
        #Convert CsrBtResultCode to CsrBt<module>ResultCode. e.g. CsrBtAvResultCode, CsrBtHfResultCode
        new_token = token[:5] + group.title() + token[5:]
        return new_token
    if token in blacklisted_tokens:
        dropped_enum_tokens.add(token)
        return ""
    return token

#Extract macro name. Handle any exceptions
def get_macro_name(group, token):
    #No macro filtering yet. So simply retun the token back
    if True:
        return token
    else:
        dropped_macro_tokens.add(token)
        return ""

#Parse a line and extract typename and macro name if it exist. These
#are stored as in tokens dictionary.
def search_tokens(group, lines, tokens):
    defines  = []
    enumeration = ""
    for line in lines:
        findings = re.search(SEARCH_PATTERN, line)
        if findings:
            enumeration = get_enum_name(group, findings.group(2))
            define_name = get_macro_name(group, findings.group(1))
            if enumeration != "" and define_name != "": 
                DEBUG('Enum: ' +enumeration+', Define: '+define_name)
                definition = rename(define_name) + ' = ' + define_name
                store_in_dictionary(tokens, enumeration, definition)

#Find all tokens(enums and macros) from a header file.
def get_tokens_from_header(g, f):
    tokens = {}
    with io.open(f,encoding='utf8') as header:
        lines = header.readlines()
        search_tokens(g, lines, tokens)
    return tokens

#--------------------------------------------------------------------------------------------
# Code Generation utilities
#--------------------------------------------------------------------------------------------
#Get a prim header file name from an enum header file name
def enum_filename_to_prim_filename(enum_filename):
    #remove _prim_enum_dbg from the file name
    prim_filename = enum_filename[:-11] + '.h'
    return prim_filename

#Get an enum header file name from a prim header file name
def prim_filename_to_enum_filename(prim_filename):
    # add _enum_dbg.h in the end
    return prim_filename[:len(prim_filename) - len('.h')] + ENUM_FILE_NAME_SUFFIX + ".h"

#Get an preserve macro source file name from a prim header file name
#This is currently unused as we are not generating preserve macro file for every enum header.
#Instead a single preserve macro source (pointed to by PRESERVE_SOURCE_FILE) is generated.
def prim_filename_to_preserve_filename(prim_filename):
    # add _enum_dbg.h in the end
    return prim_filename[:len(prim_filename) - len('.h')] + ENUM_FILE_NAME_SUFFIX + ".c"

#Generate and return a C Enumeration
def generate_enum(enum, defines):
    definition = "enum " + enum + "{\n"
    for d in defines:
        definition+= "    " + d + ",\n" 
    definition+= "};\n"
    return definition

#Generate and return a preserve macro declaration
def generate_preserve_macros(enum):
    return "UTIL_PRESERVE_GENERATED_ENUM("+enum+")\n"

#Generate a file comment header block for a .h file
def generate_fileheader_for_header_file(filename):
    macro = filename.upper() + UNDERSCORE + UNDERSCORE
    macro = re.sub(DOT, UNDERSCORE, macro)
    header = '#ifndef ' + macro + '\n'
    header+= '#define ' + macro + '\n'
    header+= '/*******************************************************************************\n'
    header+= ' * File: ' + filename + '\n'
    header+= ' * Copyright (c) 2022-'+get_current_year()+' Qualcomm Technologies International, Ltd.\n'
    header+= ' * All Rights Reserved.\n'
    header+= ' * Qualcomm Technologies International, Ltd. Confidential and Proprietary.\n'
    header+= ' *\n'
    header+= ' * DO NOT EDIT\n'
    header+= ' * Generated from ' + enum_filename_to_prim_filename(filename) + ' by enum_generator.py \n'
    header+= ' ******************************************************************************/\n'
    header+= '#include "'+enum_filename_to_prim_filename(filename)+'"\n' 
    header+= '\n'
    return header

#Generate a file comment header block for preserve macro source file
#Note - currently we are defining the preserve macro utilities in csr_synergy.h,
#       so we are including "csr_synergy.h". This will be updated as we start using 
#       macros.h ADK header.
def generate_fileheader_for_source_file(filename):
    header = '/*******************************************************************************\n'
    header+= ' * File: ' + filename + '\n'
    header+= ' * Copyright (c) 2022-'+get_current_year()+' Qualcomm Technologies International, Ltd.\n'
    header+= ' * All Rights Reserved.\n'
    header+= ' * Qualcomm Technologies International, Ltd. Confidential and Proprietary.\n'
    header+= ' *\n'
    header+= ' * Generated file - DO NOT EDIT\n'
    header+= ' * This file contains the macros to preserve the generated enums in elf file.\n'
    header+= ' ******************************************************************************/\n'
    header+= '#include "csr_synergy.h"\n'
    header+= '\n'
    header+= '#define CSR_STRINGIFY(x) #x\n'
    header+= '#define UTIL_PRESERVE_TYPE_FOR_DEBUGGING_(typename, section_name, tag) _Pragma(CSR_STRINGIFY(datasection section_name)) tag typename preserve_##tag##_##typename##_for_debugging;\n'
    header+= '#define UTIL_PRESERVE_TYPE_IN_SECTION(typename, section_name) UTIL_PRESERVE_TYPE_FOR_DEBUGGING_(typename, section_name, enum)\n'
    header+= '#define UTIL_PRESERVE_GENERATED_ENUM(typename) UTIL_PRESERVE_TYPE_IN_SECTION(typename, MSG_ENUMS)\n'
    return header

#Generate file footer for header file
def generate_filefooter_for_header_file(filename):
    macro = filename.upper() + UNDERSCORE + UNDERSCORE
    macro = re.sub(DOT, UNDERSCORE, macro)
    header = '#endif /*' + macro + '*/\n'
    return header

#Generate dummy function for preserve enum source file
#Without adding a dummy function, the source file is not getting added as 
#part of synergy library. We also need to call this dummy function from existing 
#synergy function. Need to find a better way to do this. 
def generate_dummy_function():
    code = "\nvoid util_preserve_gen_enums(void);\n"
    code+= "void util_preserve_gen_enums(void)\n"
    code+= "{\n"
    code+= "    /*Dummy Code added to ensure inclusion of object file in library.*/\n"
    code+= "}\n"
    return code

#Write file header for preserve macro file
def write_header_to_source_file(filename):
    with open(filename, 'w+') as t:
        t.write(generate_fileheader_for_source_file(os.path.basename(PRESERVE_SOURCE_FILE)))
        t.close()

#Write dummy code in preserve macro file
def write_dummy_source_code(filename):
    with open(filename, 'a') as t:
        t.write(generate_dummy_function())
        t.close()

#Write preserve macros in preserve macro file
def write_preserve_macros_to_file(macro_filename, prim_filename, tokens):
    with open(macro_filename, 'a+') as f:
        f.write("\n/* Enums from "+ os.path.basename(prim_filename) + " */\n")
        f.write("#include \""+ os.path.basename(prim_filename) + "\"\n")
        for t in tokens:
            f.write(generate_preserve_macros(t))
        f.close()

#Compare generated file with existing file and replace only if changed.
def replace_file_if_changed(original, new):
    with open(new) as n:
        content = n.read()
        n.close()
        if exists(original) and filecmp.cmp(original, new, shallow=False) != 0:
            #existing file matches the generated file
            PRINT_LOGFILE('\nNo change in '+original);
        else:
            with open(original, 'w+') as org:
                PRINT_LOGFILE('\nWriting file '+original)
                org.seek(0)
                org.write(content)
                org.truncate()
                org.close()

#Write enums to generated header files
def write_enums_to_file(filename, tokens):
    print(filename.replace('\\','/') + " ")
    with open(TMP_HEADER, 'w+') as f:
        f.write(generate_fileheader_for_header_file(os.path.basename(filename)))
        for t in tokens:
            enum_print = generate_enum(t, tokens[t])
            f.write(enum_print)
            f.write("\n")
        f.write(generate_filefooter_for_header_file(os.path.basename(filename)))
        f.seek(0)
        content = f.read()
        f.close()
        replace_file_if_changed(filename, TMP_HEADER)

# Reads INTERFACE_LIST and returns files marked for conversion as files[].
# Also updates blacklisted_tokens global array.
def get_public_headers_and_blacklist():
    files = {}
    with open(INTERFACE_LIST) as f:
        tree = ET.parse(f)
        root = tree.getroot()
        for child in root:
            if child.tag == 'FILES':
                for entry in child:
                    group = entry.get('group', default='')
                    path = os.path.join(PUBLIC_HEADERS_DIR_PATH,entry.get('path', default=''))
                    if group in files.keys():
                        files[group].append(path)
                    else:
                        files[group] = [path]
            elif child.tag == 'BLACKLIST':
                for entry in child:
                    typename = entry.get('name', default='')
                    blacklisted_tokens.append(typename)
        f.close()
    return files

#--------------------------------------------------------------------------------------------
# Main
#--------------------------------------------------------------------------------------------
def Generate_enums():
    interface_files = get_public_headers_and_blacklist()
    PRINT_LOGFILE('\nInterfaces :\n ' + str(interface_files))
    PRINT_LOGFILE('\nBlacklisted types :\n ' + ' '.join(blacklisted_tokens))
    write_header_to_source_file(TMP_SOURCE)
    if len(interface_files):
        for group in interface_files:
            PRINT_LOGFILE('\nConverting files for '+group)
            for f in interface_files[group]:
                tokens = get_tokens_from_header(group, f)
                write_enums_to_file(prim_filename_to_enum_filename(f), tokens)
                generated_files.append(prim_filename_to_enum_filename(f))
                write_preserve_macros_to_file(TMP_SOURCE, prim_filename_to_enum_filename(f), tokens)
        write_dummy_source_code(TMP_SOURCE)
        #Preserve source file generation is currently disabled. Uncomment below line to re-enable it.
        #replace_file_if_changed(PRESERVE_SOURCE_FILE, TMP_SOURCE)
        PRINT_LOGFILE('\nDROPPED ENUMS:\n ' + ' '.join(dropped_enum_tokens))
        PRINT_LOGFILE('\nDROPPED MACROS:\n ' + ' '.join(dropped_macro_tokens))
        
        if os.path.exists(TMP_HEADER):
            os.remove(TMP_HEADER)
        if os.path.exists(TMP_SOURCE):
            os.remove(TMP_SOURCE)
            
if __name__ == '__main__':
    Generate_enums()
