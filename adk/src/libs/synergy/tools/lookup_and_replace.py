'''
NAME:
    lookup_and_replace.py

PURPOSE:
    Utility enum_generator.py generates enums based on macros found in synergy prim headers.
    Enums header files are generated and kept in this folder - MACRO_FILES_PATHS.
    The generated enum constants have same value as the original macros but their name have understandably changed.
    
    This script searches for the original macro usage in directories pointed to by variable REPLACE_FILES_PATHS
    and replaces them with new (generated) definitions.
    
 USAGE:
    Run the script from synergy folder
    $>python tools\lookup_and_replace.py
'''
import re
import os
from pathlib import Path
from colorama import Fore, init
WORKSPACE_BASE_PATH = "..\\..\\..\\..\\"
MACRO_FILES_PATHS = os.path.join(os.getcwd(),".\\bt\\inc")
REPLACE_FILES_PATHS = [\
    os.path.join(WORKSPACE_BASE_PATH,"adk\\src\\domains"),
    os.path.join(WORKSPACE_BASE_PATH,"adk\\src\\services"),
    os.path.join(WORKSPACE_BASE_PATH,"adk\\src\\topologies"),
    os.path.join(WORKSPACE_BASE_PATH,"earbud\\src")]

MACRO_FILE_SEARCH_PATTERN = '*_dbg.h'
GLOBAL_COUNT = 0
GLOBAL_REPLACED_COUNT = 0
GLOBAL_FILE_LIST = []
IGNORE_USER = False
CONTEXT_LEN = 200 #no. of characters printed along with the searched macro

ONE_OR_MORE_SPACES = '\s+'
NAME = '([a-zA-Z0-9_]+)'
EQUALS = '='
COMMA = ','

def meaningful_searchresult(string, macro):
    substrings = []
    start_index = string.find('\n')
    end_index = string.rfind('\n')
    macro_index = string.find(macro)
    macro_len = len(macro)
    if start_index != -1 and end_index != -1:
        substrings.append(string[start_index:macro_index])
        substrings.append(macro)
        substrings.append(string[macro_index + macro_len:end_index])
        return substrings
    return string

def search_and_replace_in_file(f, macros):
    global GLOBAL_COUNT
    global global_file_count
    global GLOBAL_REPLACED_COUNT
    global IGNORE_USER
    with open(f,encoding='utf8') as fr:
        user_response = 'n'
        content = fr.read()
        count = 0
        replaced_count = 0
        for macro in macros:
            index = content.find(macro)
            if index != -1:
                line = len(re.findall('\n',content[:index])) + 1
                print(f"macro \"{macro}\" found in {f} at {line}")
                sub_strings = meaningful_searchresult(content[index-CONTEXT_LEN:index+CONTEXT_LEN], macro)
                print(Fore.YELLOW + sub_strings[0] + \
                      Fore.GREEN + sub_strings[1] + \
                      Fore.YELLOW + sub_strings[2])
                print(Fore.WHITE)
                if user_response != 'Y' and user_response != 'N' and IGNORE_USER == False:
                    user_response = input(f"Update this with \"{macros[macro]}\"? y(yes)/n(no)/Y(YesAll)/N(NoAll)/I(Ignore all) ")
                if user_response == 'y' or user_response == 'Y':
                    content = re.sub(macro, macros[macro], content)
                    replaced_count = replaced_count + 1
                if user_response == 'I':
                    IGNORE_USER = True;
                count = count + 1
        if count > 0:
            GLOBAL_COUNT = GLOBAL_COUNT + count
            GLOBAL_REPLACED_COUNT = GLOBAL_REPLACED_COUNT + replaced_count
            GLOBAL_FILE_LIST.append(f) 
            fr.close()
            os.chmod(f,0o755)
            with open(f,'w') as fw:
                fw.write(content)
            
def get_list_of_macros_files():
    paths = []
    for path in Path(MACRO_FILES_PATHS).rglob(MACRO_FILE_SEARCH_PATTERN):
        paths.append(path)
    return paths

def get_list_of_replace_files():
    paths = []
    for dirs in REPLACE_FILES_PATHS:
        for path in Path(dirs).rglob('*.c'):
            paths.append(path)
    return paths

def get_list_of_macros(files):
    #search patterh = "BT_RESULT_CODE_AVRCP_SUCCESS = CSR_BT_RESULT_CODE_AVRCP_SUCCESS,"
    searchpattern = ONE_OR_MORE_SPACES + NAME + ONE_OR_MORE_SPACES + EQUALS + ONE_OR_MORE_SPACES + NAME + COMMA
    macros = {}
    for f in files:
        with open(f,'r') as openf:
            lines = openf.readlines()
            for l in lines:
                findings = re.search(searchpattern, l)
                if findings:
                    macros[findings.group(2)] = findings.group(1)
    return macros                        

init()#colorama init
macro_files = get_list_of_macros_files()
macros = get_list_of_macros(macro_files)
print(f'No. of Macros to replace: {len(macros)}')
replace_files = get_list_of_replace_files()
print(f'No. of replace files {len(replace_files)}')
for f in replace_files:
    search_and_replace_in_file(f, macros)
print(f'{GLOBAL_COUNT} matches found in {len(GLOBAL_FILE_LIST)} files')
print(f'{GLOBAL_REPLACED_COUNT} matches updated')
print(*GLOBAL_FILE_LIST, sep='\n')
