import os
from os import path
import sys
import shutil
import stat

# Go to the target folder
# >syn_rel.py <synergy_build_path> <files_list>

# Deletes the target file/folder
def Remove(target):
	if path.exists(target):
		if path.isfile(target):
			os.chmod(target, stat.S_IWUSR)
			os.remove(target)
		elif path.isdir(target):
			for root, dirs, files in os.walk(target, topdown = False):
				for file in files:
					filepath = path.join(root, file)
					os.chmod(filepath, stat.S_IWUSR)
					os.remove(filepath)
				for dir in dirs:
					Remove(path.join(root, dir))
			os.rmdir(target)

# Remove subfolders, if any
def RemoveSubfolders(folder):
	for root, dirs, files in os.walk(folder, topdown = False):
		for dir in dirs:
			Remove(path.join(root, dir))

# Obtains target folder for a source folder.
# Also creates sub-folders as required in the
# target path based on reference include folder
def CreateTargetFolder(targetFilePath):
	subfolder = TargetFolder
	splitPath = targetFilePath.split("\\")

	for dir in splitPath[0:-1]:
		subfolder = path.join(subfolder, dir)
		if not path.isdir(subfolder):
			os.mkdir(subfolder, 0o666)


def CopyFiles(synBuild, synFrw, synBt, fileList):
	print("fileList = " + fileList)
	print("synBuild = " + synBuild)
	print("synFrw = " + synFrw)
	print("synBt = " + synBt)
	print("\n")

	#RemoveSubfolders(TargetFolder)

	with open(fileList, "r") as file:
		lines = file.readlines()
		for line in lines:
			line = line.rstrip()

			source = line
			source = source.replace("<SYN_BUILD>", synBuild)
			source = source.replace("<SYN_FRW>", synFrw)
			source = source.replace("<SYN_BT>", synBt)

			target = line
			target = target.replace("<SYN_BUILD>", "config")
			target = target.replace("<SYN_FRW>", "frw")
			target = target.replace("<SYN_BT>", "bt")

			print("source = " + source)
			print("target = " + target)

			Remove(target)
			CreateTargetFolder(target)
			shutil.copy(source, target)
	file.close()

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


CMAKE_CACHE = "CMakeCache.txt"
TargetFolder = os.getcwd() # Copy into current working directory

#source_folders = {"inc", "profile_managers", "porting",

args = sys.argv[1:]
if len(args) == 2 and path.isdir(args[0]) and path.isfile(args[1]):
	synFrw, synBt = ExtractPaths(args[0])
	CopyFiles(args[0], synFrw, synBt, args[1])
else:
	print("Template: \n\tsyn_rel.py <SYN_BUILD_PATH> <LIST_FILE>")

