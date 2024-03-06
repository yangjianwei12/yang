# -*- python -*-
import os
from os import path
import stat
import shutil
import sys
from P4 import P4,P4Exception
import syn_integration_config as SynConfig
import xml.etree.ElementTree as ET

# Pre-requisite: 	1. Python 3.x
# 					2. Install p4python plugin: >pip install p4python
# Create Synergy BT partial branching workspace.
# Build Synergy BT.
# Run this script: >branch_mapping.py <synergy_build_path> <files_list>

CMAKE_CACHE = "CMakeCache.txt"

Files_Read_From_XML = []
Paths = {'<SYN_BUILD>': "", '<SYN_FRW>': "", '<SYN_BT>': ""}
PathFolders = {'<SYN_BUILD>': "config", '<SYN_FRW>': "frw", '<SYN_BT>': "bt"}
Sources = {'<SYN_BUILD>': [], '<SYN_FRW>': [], '<SYN_BT>': []}


# Deletes the target file/folder
def RemoveFile(target):
	if path.exists(target) and path.isfile(target):
		os.chmod(target, stat.S_IWUSR)
		os.remove(target)


# Obtains target folder for a source folder.
# Also creates sub-folders as required in the
# target path based on reference include folder
def FolderCreate(filePath):
	subfolder = SynConfig.TARGET_FOLDER
	splitPath = filePath.split("\\")

	for dir in splitPath[0:-1]:
		subfolder = path.join(subfolder, dir)
		if not path.isdir(subfolder):
			os.mkdir(subfolder, 0o666)

# Creates branch view for integration
def BranchViewCreate(p4):
	try:
		#branchViewFile = open(BRANCH_VIEW_FILE, "w")
		#branchViewFile.write(SynConfig.BRANCH_VIEW_BOILERPLATE)
		view = []
		targetDepot = p4.run("where", SynConfig.TARGET_FOLDER)
		print("targetDepot", targetDepot)

		for key in Sources:
			for file in Sources[key]:
				#print("path, key", Paths[key], file)
				clientFile = path.join(Paths[key], file)

				print("clientFile", clientFile)

				try:
					whereOutput = p4.run("where", clientFile)
					#print("whereOutput", whereOutput)

					depotFilePath = whereOutput[0]['depotFile']
					for m in whereOutput:
						if 'unmap' not in m: # Replace the mapping which is unmapped
							depotFilePath = m['depotFile']

					fileMapping = "\n\t" + depotFilePath + " " + targetDepot[0]['depotFile'] + "/" + PathFolders[key] + "/" + file
					fileMapping = fileMapping.replace("\\", "/")
					#print(fileMapping)
					view.append(fileMapping)
					#branchViewFile.write(fileMapping)

				except P4Exception:
					for e in p4.errors:
						print(e)

					targetFile = path.join(SynConfig.TARGET_FOLDER, PathFolders[key], file)
					print("Copy file", clientFile, targetFile)
					RemoveFile(targetFile)
					FolderCreate(path.join(PathFolders[key], file))
					shutil.copy(clientFile, targetFile)

		#branchViewFile.close()

		branch = p4.fetch_branch("-o", SynConfig.BRANCH_VIEW_FILE)
		branch['View'] = view
		print(branch)

		p4.save_branch(branch)


	except P4Exception:
		for e in p4.errors:
			print(e)

# Creates integration changelist
def IntegrateFiles():
	try:
		p4 = P4()
		p4.port = SynConfig.P4_ENV_VAR['port']
		p4.user = SynConfig.P4_ENV_VAR['user']
		p4.client = SynConfig.P4_ENV_VAR['client']
		p4.connect()
		info = p4.run("info")

		BranchViewCreate(p4)

		p4.run("integrate", "-b", SynConfig.BRANCH_VIEW_FILE)

		p4.disconnect()

	except P4Exception:
		for e in p4.errors:
			print(e)

# Arranges the files in the dictionary according to their origins
def OrganizeFilesBasedOnOrigin(synBuild, synFrw, synBt, fileList):
	print("synBuild = " + synBuild)
	print("synFrw = " + synFrw)
	print("synBt = " + synBt)
	print("\n")

	Paths['<SYN_BUILD>'] = synBuild
	Paths['<SYN_FRW>'] = synFrw
	Paths['<SYN_BT>'] = synBt

	for line in fileList:
		line = line.rstrip()
		source = line
		source = source.replace("/", "\\")
		if source.startswith('frw'):
			source = source.replace("frw\\", "")
			Sources['<SYN_FRW>'].append(source)
		else:
			source = source.replace('bt\\', "")
			Sources['<SYN_BT>'].append(source)

def WritePathList(f, key):
	#add name and opening brace
	s = "\n\n" + key + " = ["
	
	#add the file paths
	for path in Paths[key]:
		f.write(s + "\n")
		unixPath = path.replace("\\", "/")
		s = "\t" + "\"" + unixPath + "\","		
	#remove last ',' from the last file path in the list
	s = s[:-1]
	
	#add closing brace
	f.write(s + "]")
	
# Extracts Synergy Frw and Bt paths from CMake build
def ExtractPaths(buildPath):
	cache = path.join(buildPath, CMAKE_CACHE)
	synBt = ""
	synFrw = ""
	with open(cache, "r") as file:
		lines = file.readlines()
		for line in lines:
			if "CSR_BT_ROOT:PATH=" in line:
				tmp, synBt = line.split('=', 1)
			elif "CSR_FRW_ROOT:PATH=" in line:
				tmp, synFrw = line.split('=', 1)

			if synBt and synFrw:
				break
	file.close()
	return synFrw.rstrip(), synBt.rstrip()
	
def StoreFilePaths(root):
	key = root.attrib["group"]
	for files in root.iter():
		path = files.get("path", default="None")
		if (path != 'None'):
			Files_Read_From_XML.append(path)

def ReadFilesListsFromXml(file):
	tree = ET.parse(file)
	root = tree.getroot()
	for child in root:
		if(child.tag == 'FILES'):			
			StoreFilePaths(child)

args = sys.argv[1:]
if len(args) == 2 and path.isdir(args[0]) and path.isfile(args[1]):
	synFrw, synBt = ExtractPaths(args[0])
	ReadFilesListsFromXml(args[1])
	OrganizeFilesBasedOnOrigin(args[0], synFrw, synBt, Files_Read_From_XML)
	IntegrateFiles()
else:
	print("Template: \n\tbranch_mapping.py <SYN_BUILD_PATH> <XML FILE>")

