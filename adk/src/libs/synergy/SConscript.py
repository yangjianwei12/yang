"""
Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.

Synergy Library build script
"""

from __future__ import print_function
import os
import sys
import logging
import SCons.Builder

Import('p_env')

# Builder for database files
p_env.Tool('gattdbgen')

# Generate source lists
from tools.gen_build_files import Parse_xml, Paths

Parse_xml('source_lists.xml')
Paths["SYNERGY_INCLUDE_PATHS"] = ["#adk/src/libs/synergy/"+p for p in Paths["SYNERGY_INCLUDE_PATHS"]]
Paths["SYNERGY_INCLUDE_PATHS"] = [p.replace("#adk/src/libs/synergy/QBL_LIB_PATH","#/apps_platform/"+p_env['CHIP_TYPE']+"/fw/bt/qbluestack") for p in Paths["SYNERGY_INCLUDE_PATHS"]]
Paths["SYNERGY_INCLUDE_PATHS"].append("#/apps_platform/"+p_env['CHIP_TYPE']+"/fw/bt/common/interface")
Paths["CORESTACK_LIB_SOURCES"] = [p.replace("QBL_LIB_PATH","qbluestack") for p in Paths["CORESTACK_LIB_SOURCES"]]

#Generate Enum Headers and Source
from tools.enum_generator import Generate_enums, generated_files
Generate_enums()

#For pta apps use pta config file
if "_pta" in p_env["VARIANT_APP"].lower() :
    conf_file = 'config/SConscript.PTA'
    p_env.SConscript(conf_file, exports='Paths', must_exist=True)

# Assign default values where required
p_env.SetDefault(BUILD_TYPE = 'release')

# It is obvious for customers to add DEFS to the x2p file.
# So we want to merge these DEFS.
defs = set(Split(p_env.get('DEFS', '')))
defs |= {'CAA',
         #GATT_DBI_LIB The scons tool defines it( adk\src\site_scons\site_tools\gattdbifgen.py )
         'HCI_CIS_MAX="((uint8_t)0x08)"',
         'HCI_ULP_BROADCAST_CODE_SIZE=16', 'BLUESTACK_LIB'}
if 'DEFINES' in p_env:
    defs |= set(Split(p_env['DEFINES']))
p_env['DEFS'] = sorted(defs)

# Initialise folder locations
PROJECT_ROOT = os.getcwd().replace('\\', '/')
logging.debug("PROJECT_ROOT is %s", PROJECT_ROOT)

try:
    TOOLS_ROOT = p_env['TOOLS_ROOT']
except KeyError:
    print("Variable TOOLS_ROOT has not been defined. It should be set to "
          "the location of the tools folder.")
    Exit(1)
logging.debug("TOOLS_ROOT is %s", TOOLS_ROOT)

try:
    ADK_ROOT = p_env['ADK_ROOT']
except KeyError:
    print("Variable ADK_ROOT has not been defined. It should be set to "
          "the location of the ADK folder.")
    Exit(1)
logging.debug("ADK_ROOT is %s", ADK_ROOT)

# Setup paths for the build
SRC_DIR = os.getcwd()

LIB_DIR = p_env['inst_dir'] + '/lib/' + p_env['LIBRARY_VERSION'] + '/native'
SYNERGY_INTERFACE_DIR = p_env['profiles_inc_dir']
SYNERGY_PLATFORM_INTERFACE_DIR = SYNERGY_INTERFACE_DIR + '/platform'
SYNERGY_LE_AUDIO_DIR = 'bt/profile_managers/le_audio'
SYNERGY_SERVICE_DIR = SYNERGY_LE_AUDIO_DIR + '/services'
SYNERGY_LE_DIR = 'bt/profile_managers/gatt_services'

# Expand ellipses in LIBPATHS and INCPATHS to include subdirectories recursively
for f in p_env.get('INCPATHS', '').split():
    if f.endswith('...'):
        for root, dir, file in os.walk(f[:-3]):
            p_env.AppendUnique(CPPPATH=root) #  <== CPPATH was replaced later on
    else:
        p_env.AppendUnique(CPPPATH=f)

p_env.SetDefault(LIBPATH=[])
for f in p_env.get('LIBPATHS', '').split():
    if f.endswith('...'):
        for root, dir, file in os.walk(f[:-3]):
            p_env.AppendUnique(LIBPATH=root)
    else:
        p_env.AppendUnique(LIBPATH=f)

# Release package will not contain LEA code. Temporarily relying on availability
# of code for building. Later we need to expose a build parameter (similar to
# CHIP_TYPE) via lib builder utilities.
LEA_ENABLED = os.path.exists(SYNERGY_LE_AUDIO_DIR)
if LEA_ENABLED:
    p_env.Append(DEFS = ['CSR_ENABLE_LEA'])
#CDA"_TODO remove this once xpan defines are cleared out of synergy
p_env.Append(DEFS = ['EXCLUDE_CSR_BT_XPAN_SERVER_MODULE', 'EXCLUDE_CSR_BT_XPAN_CLIENT_MODULE','BLUESTACK_BREAK_IF_PANIC_RETURNS="break;"', 'CSR_LOG_ENABLE'])

logging.debug("BUILDING FOR %s", p_env['CHIP_TYPE'])

# Additional project specific warning options
if p_env["VARIANT_CTOOLCHAIN"] == 'llvm':
    p_env.AppendUnique(WARNING_OPTS =
                       ['-Wno-parentheses-equality',
                        '-Wno-format',
                        '-Wno-tautological-pointer-compare',
                        '-Wsign-compare',
                        '-Wextra',
                        '-Wno-gnu-include-next',
                        '-Wno-keyword-macro',
                        '-Wno-bitfield-constant-conversion', # bap_client_ase_fsm_definitions.h 4 bit value, with at least 5 bits going in
                        '-Wno-newline-eof'])

WARNING_OPTS = p_env.get('WARNING_FLAGS', p_env['WARNING_OPTS'])

if not p_env.get('EXTRA_WARNINGS'):
    WARNING_OPTS += p_env['EXTRA_WARNING_OPTS']

# Treat warnings as errors
WARNING_OPTS += ['-Werror']

COMPILE_FLAGS = WARNING_OPTS

# Translate options variable names into SCons versions
p_env.Append(CFLAGS=COMPILE_FLAGS)
p_env.Append(CFLAGS=["-includebuild_defs.h", "-includechip_macros.h"])  # LIBR_TODO

p_env.Append(CPPDEFINES=p_env['DEFS'])
# TODO Order of paths changed for CDA2 ?
p_env.PrependUnique(CPPPATH = Paths['SYNERGY_INCLUDE_PATHS'] + [SYNERGY_INTERFACE_DIR], delete_existing=True)

# Header rules
for f in Paths['BT_PUBLIC_HEADERS']:
    p_env.Command(SYNERGY_INTERFACE_DIR + '/' + os.path.basename(f), f,
                  [Copy('$TARGET', '$SOURCE'), Chmod('$TARGET', 0o666)])

for f in generated_files:
    p_env.Command(SYNERGY_INTERFACE_DIR + '/' + os.path.basename(f), f,
                  [Copy('$TARGET', '$SOURCE'), Chmod('$TARGET', 0o666)])

for f in Paths['PLATFORM_PUBLIC_HEADERS']:
    p_env.Command(SYNERGY_PLATFORM_INTERFACE_DIR + '/' + os.path.basename(f), f,
                  [Copy('$TARGET', '$SOURCE'), Chmod('$TARGET', 0o666)])

for f in Paths['LE_DBI_FILES']:
    p_env.Command(SYNERGY_INTERFACE_DIR + '/' + os.path.basename(f), f,
                  [Copy('$TARGET', '$SOURCE'), Chmod('$TARGET', 0o666)])

if LEA_ENABLED:
    for f in Paths['BT_LEA_PUBLIC_HEADERS']:
        p_env.Command(SYNERGY_INTERFACE_DIR + '/' + os.path.basename(f), f,
                      [Copy('$TARGET', '$SOURCE'), Chmod('$TARGET', 0o666)])

    for f in Paths['LEA_DBI_FILES']:
        p_env.Command(SYNERGY_INTERFACE_DIR + '/' + os.path.basename(f), f,
                      [Copy('$TARGET', '$SOURCE'), Chmod('$TARGET', 0o666)])
        p_env.DbiObject(SYNERGY_INTERFACE_DIR + '/' + os.path.basename(f).replace('.dbi', '.h'), f)

# Setup paths for the build
path = ['default', p_env['LIBRARY_VERSION'], 'native']
if p_env['BUILDOUTPUT_PATH']:
    path = [p_env['BUILDOUTPUT_PATH'], 'synergy'] + path
BUILD_DIR = '/'.join(path)

# Ensure object files are created separately from source files, and
# don't create copies of the source files in the build folder
p_env.VariantDir(BUILD_DIR, SRC_DIR, duplicate=0)
p_env.VariantDir(BUILD_DIR+'/qbluestack', SRC_DIR + "/../../../../apps_platform/" + p_env['CHIP_TYPE'] + "/fw/bt/qbluestack",  duplicate=1)
Paths['FRAMEWORK_LIB_SOURCES'] = [BUILD_DIR + '/' + s for s in Paths['FRAMEWORK_LIB_SOURCES']]
Paths['SERVICE_LIB_SOURCES'] = [BUILD_DIR + '/' + s for s in Paths['SERVICE_LIB_SOURCES']]
Paths['PROFILES_LIB_SOURCES'] = [BUILD_DIR + '/' + s for s in Paths['PROFILES_LIB_SOURCES']]
Paths['CORESTACK_LIB_SOURCES'] = [BUILD_DIR + '/' + s for s in Paths['CORESTACK_LIB_SOURCES']]
if LEA_ENABLED:
    Paths['LEA_LIB_SOURCES'] = [BUILD_DIR + '/' + s for s in Paths['LEA_LIB_SOURCES']]

# Special handling for Service sources
# LIBR_TODO Path now same as other files (but did chnage the order there)
CPPPATH_SERVICE = p_env["CPPPATH"]#Paths['OS_INCLUDE_PATHS'] + Paths['SYNERGY_INCLUDE_PATHS'] + p_env["CPPPATH"]
SYNERGY_OBJECT_FRW = [p_env.Object(src, CPPPATH=CPPPATH_SERVICE)
                                for src in Paths['FRAMEWORK_LIB_SOURCES']]
SYNERGY_SERVICE_OBJECT = [p_env.Object(src, CPPPATH=CPPPATH_SERVICE)
                                for src in Paths['SERVICE_LIB_SOURCES']]
# (remaining sources can use default C builder)

# Special handling for csr_bt_corestack_libs because the number of
# objects exceeds the command line length limit

# CDA2_TODO
# Duplicate symbols were found by including this file. Killing it here removes from library (and compilation?)
duplicate_symbols_f = "native/bt/profile_managers/core_stack/common/bluestack/common/common.c"
[Paths['CORESTACK_LIB_SOURCES'].remove(file) for file in Paths['CORESTACK_LIB_SOURCES'] if duplicate_symbols_f in file]

CORESTACK_OBJS = [p_env.Object(src)[0]
                                for src in Paths['CORESTACK_LIB_SOURCES']]
CORESTACK_OBJS_FILE = p_env.File(BUILD_DIR + '/csr_bt_corestack_libs.txt')
p_env.Textfile(CORESTACK_OBJS_FILE, [f.path.replace('\\', '/') for f in CORESTACK_OBJS])
p_env.Depends(CORESTACK_OBJS_FILE, CORESTACK_OBJS)

if sys.platform == 'win32':
    import SCons.Platform.win32

    if p_env["VARIANT_CTOOLCHAIN"] == 'gcc':
        ARCMD= '$AR $ARFLAGS ${TEMPFILE("$TARGET $SOURCES.posix")}'
    else:
        ARCMD= '$AR $ARFLAGS ${TEMPFILE("$TARGET $SOURCES")}'

# Library component rules
SYNERGY_FRW_LIB       = p_env.Library(target = LIB_DIR + '/csr_frw',
                                      source = SYNERGY_OBJECT_FRW),
SYNERGY_SERVICE_LIB   = p_env.Library(target = LIB_DIR + '/synergy_service',
                                      source = SYNERGY_SERVICE_OBJECT),
if sys.platform == 'win32':
    SYNERGY_PROFILES_LIB = p_env.Library(target = LIB_DIR + '/csr_bt_profiles',
                                     source = Paths['PROFILES_LIB_SOURCES'],
                                     ARCOM = ARCMD,
                                     TEMPFILEARGJOIN='\n', # Makes the temp file neater (but gets deleted if works)
                                     SPAWN=SCons.Platform.win32.spawn)
else:
    SYNERGY_PROFILES_LIB  = p_env.Library(target = LIB_DIR + '/csr_bt_profiles',
                                      source = Paths['PROFILES_LIB_SOURCES']),

SYNERGY_CORESTACK_LIB = p_env.Library(target = LIB_DIR + '/csr_bt_corestack_libs',
                                      source = CORESTACK_OBJS_FILE,
                                      ARCOM = '$AR $ARFLAGS $TARGET @$SOURCE')
LIBRARIES = [SYNERGY_FRW_LIB, SYNERGY_SERVICE_LIB, SYNERGY_PROFILES_LIB,
             SYNERGY_CORESTACK_LIB]


if LEA_ENABLED:
    if sys.platform == 'win32':
        SYNERGY_LE_AUDIO_LIB = p_env.Library(target = LIB_DIR + '/csr_bt_le_audio',
                                         source = Paths['LEA_LIB_SOURCES'],
                                         ARCOM = ARCMD,
                                         TEMPFILEARGJOIN='\n', # Makes the temp file neater (but gets deleted if works)
                                         SPAWN=SCons.Platform.win32.spawn)
    else:
        SYNERGY_LE_AUDIO_LIB = p_env.Library(target = LIB_DIR + '/csr_bt_le_audio',
                                         source = Paths['LEA_LIB_SOURCES'])

    LIBRARIES.append(SYNERGY_LE_AUDIO_LIB)

# Deliverable library
p_env.Library(target = LIB_DIR + '/csr_bt', source = LIBRARIES)
