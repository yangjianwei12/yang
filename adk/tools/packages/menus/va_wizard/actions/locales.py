# Copyright (c) 2022 Qualcomm Technologies International, Ltd.

import os
import shutil
import logging
import json
import zipfile
import re
import fnmatch

from menus.addon_importer import AddonUtils
addon_utils = AddonUtils()

from .action import VaAction

from menus.va_wizard.actions.defines import Defines
from menus.va_wizard.actions.utils import (
    get_audio_bin_dir,
    get_capability_project_path
)


class LocalesActionError(RuntimeError):
    pass


class Locales(VaAction):
    def __init__(self, workspace, vendor_package_file=None, available_locales=None, project_file=None, selected_locales=None):
        super(Locales, self).__init__()
        self.workspace = workspace
        self.vendor_package_file = vendor_package_file
        self.available_locales = available_locales
        self.project_file = project_file
        self.selected_locales = selected_locales if selected_locales is not None else list()
        self.available_locales_filtered_by_selection = set()

        self.locales_map = dict()
        self.extracted_model_dir = None
        self.va_fs_project = os.path.join(os.path.dirname(workspace), 'filesystems', 'va_fs.x2p')
        self.va_files_dir = os.path.join(os.path.dirname(workspace), 'filesystems', 'va')
        self.model_files_dir = os.path.join(self.va_files_dir, 'models')
        self.prompts_dir = os.path.join(self.va_files_dir, 'prompts')

    def add_preloaded_model_files(self):
        map = self._get_extracted_locale_to_model_map()
        if map:
            model_files = self._copy_extracted_model_files(map)
        else:
            model_files = self._extract_model_files()
        self.log.info("add_preloaded_model_files {}".format(model_files))
        self.add_files_to_project(model_files)

    def add_files_to_project(self, files_list):
        template = '<file path="{}"/>'
        patch = [template.format(os.path.relpath(f, os.path.dirname(self.project_file))) for f in files_list]

        addon_utils.patchFile('<project>{}</project>'.format('\n'.join(patch)), self.project_file)

    def _get_extracted_locale_to_model_map(self):
        map = None

        if self.extracted_model_dir:
            file_path = os.path.join(self.extracted_model_dir, "WakewordModelMapping.json")
            if os.path.isfile(file_path):
                with open(file_path) as f:
                    map = json.load(f)
        return map

    def _copy_extracted_model_files(self, model_map):
        src_dir = self.extracted_model_dir
        dst_dir = self.model_files_dir

        if not os.path.isdir(dst_dir):
            self.log.info("makedirs {}".format(dst_dir))
            os.makedirs(dst_dir)

        models = set()
        selected_model_files = set()
        for locale in model_map:
            model = model_map[locale]
            src = os.path.join(src_dir, model)
            dst = os.path.join(dst_dir, model)
            if model not in models:
                models.add(model)
                self.log.info("copy {} to {}".format(src, dst))
                shutil.copyfile(src, dst)
            if locale in self.selected_locales:
                selected_model_files.add(os.path.normpath(dst))

        if self.selected_locales and len(selected_model_files) == 0:
            self.log.error("No model files found for selected Locales {}".format(self.selected_locales))
            LocalesActionError("No model files found for selected Locales {}".format(self.selected_locales))

        return selected_model_files

    def _extract_model_files(self):
        zipObj = zipfile.ZipFile(self.vendor_package_file)

        if not os.path.isdir(self.model_files_dir):
            self.log.info("makedirs {}".format(self.model_files_dir))
            os.makedirs(self.model_files_dir)

        selected_model_files = set()
        for locale in self.available_locales:
            locale_file = self.locales_map[locale]['file']
            zip_path = "{}/{}".format(self.model_file_prefix, locale_file)
            extracted_path = os.path.normpath(os.path.join(self.model_files_dir, self.locales_map[locale]['code']))

            if not os.path.isdir(os.path.dirname(extracted_path)):
                self.log.info("makedir {}".format(extracted_path))
                os.mkdir(os.path.dirname(extracted_path))

            with zipObj.open(zip_path) as src, open(extracted_path, 'wb') as dst:
                self.log.info("copy {} to {}".format(src, dst))
                shutil.copyfileobj(src, dst)

            if locale in self.selected_locales:
                selected_model_files.add(extracted_path)

        return selected_model_files

    def get_locales_from_vendor_package(self, vendor_package_file):
        raise NotImplementedError("Children must implement this method")


class GaaLocales(Locales):
    def add_preloaded_model_files(self):
        super(GaaLocales, self).add_preloaded_model_files()
        self.__add_project_to_workspace()

    def __add_project_to_workspace(self):
        patch = (
            '<workspace>'
            '<project default="" name="gaa" path=""><dependencies><project name="va_fs"/></dependencies></project>'
            '<project name="va_fs" path="{}" default="no"/>'
            '</workspace>'
        ).format(self.project_file)

        self.log.info("Adding project: {} to workspace: {}".format(self.project_file, self.workspace))
        addon_utils.patchFile(patch, self.workspace)

    def get_locales_from_vendor_package(self, vendor_package_file):
        zipObj = zipfile.ZipFile(vendor_package_file)
        for f in zipObj.namelist():
            if f.startswith("Hotword_Models") and fnmatch.fnmatch(f, '*kalimba_*_map.txt'):
                model_map = zipObj.read(f).decode()
                matches = re.finditer(r".*'(300.)': '((ok|x)\/[a-zA-Z_]+\.ota)'", model_map)

                for match in matches:
                    display_locale = match.group(0)
                    model_code = match.group(1)
                    model_file = match.group(2)

                    self.model_file_prefix = os.path.dirname(f)

                    self.locales_map[display_locale] = {
                        'file': model_file,
                        'code': model_code
                    }
                    yield display_locale


class AmaLocales(Locales):
    PROMPTS_PER_LOCALE = 2

    def __init__(self, workspace, chip_type, **kwargs):
        super(AmaLocales, self).__init__(workspace, **kwargs)
        self._ama_locale_to_models_override = dict()
        self._ama_model_to_all_supported_locales = dict()
        self._prompts_to_add = list()
        self._locales_with_missing_prompt_files = list()
        self.locale_prompts_src_dir = None
        self.default_model = None
        self.chip_type = chip_type
        if chip_type == 'qcc518x_qcc308x' or chip_type == 'qcc519x_qcc309x':
            self.chip_type = 'qcc517x_qcc307x'
        self.app_project = os.path.splitext(os.path.normpath(workspace))[0] + '.x2p'
        capability_project_path = get_capability_project_path(workspace, chip_type, 'download_apva.x2p')
        self.extracted_model_dir = os.path.join(get_audio_bin_dir(capability_project_path), "models", "apva")

    
    @property
    def prompts_to_add(self):
        if not self._prompts_to_add:
            self.__compose_prompt_and_locales_from_user_selection()
        return self._prompts_to_add

    @property
    def locales_with_missing_prompt_files(self):
        if not self._locales_with_missing_prompt_files:
            self.__compose_prompt_and_locales_from_user_selection()
        return self._locales_with_missing_prompt_files

    def __compose_prompt_and_locales_from_user_selection(self):
        if self.vendor_package_file is not None:
            self.__compose_prompt_and_locales_using_vendor_package_file()
        else:
            self.__compose_prompt_and_locales_without_vendor_package_file()


    def __compose_prompt_and_locales_using_vendor_package_file(self):
        locales_with_prompt_files = self.get_locales_from_prompts()
        for locale in self.selected_locales:
            for group in self._ama_model_to_all_supported_locales.values():
                if locale in group:
                    self.available_locales_filtered_by_selection.add(locale)

                    # The set intersection of all locales that have prompts and 
                    # locales in the group gives the common locale/s in both
                    locale_group_members_with_prompt_file = list(set(group).intersection(locales_with_prompt_files))

                    if not locale_group_members_with_prompt_file:
                        self.log.warn("No prompts for locale {}".format(locale))
                        locale_to_use_for_prompt_file = locale
                    elif len(locale_group_members_with_prompt_file) > 1:
                        # The locale group has one model file but two or more members have
                        # prompts file of the their own. For example: the locale en-CA which belongs
                        # to the group of [en-CA, en-US, en-NZ, en-MX]. This set is intersected with
                        # the set of all locales that have prompt files e.g. [en-GB, en-CA, en-US, en-AU, en-IN]
                        #  will give the resulting set [en-CA, en-US]
                        self.log.warn("More than 1 set of prompts {} for locale {}".format(locale_group_members_with_prompt_file, locale))
                        if locale in locale_group_members_with_prompt_file:
                            locale_to_use_for_prompt_file = locale
                        else:
                            locale_to_use_for_prompt_file = locale_group_members_with_prompt_file[0]
                    else:
                        locale_to_use_for_prompt_file = locale_group_members_with_prompt_file[0]

                    if locale != locale_to_use_for_prompt_file and \
                            self._ama_locale_to_models_override[locale] not in locale_group_members_with_prompt_file:
                        # the group members has a different locale other than the one with the prompt file
                        # as the override locale. Change all members to point to locale with prompt file.
                        self.log.info("Updaing the override locale for group {} to {}".format(group, locale_to_use_for_prompt_file))
                        for l in group:
                            self._ama_locale_to_models_override[l] = locale_to_use_for_prompt_file

                    self.__copy_prompt_files(locale_to_use_for_prompt_file)
                    break

    def __compose_prompt_and_locales_without_vendor_package_file(self):
        for locale in self.selected_locales:
            self.available_locales_filtered_by_selection.add(locale)
            self.__copy_prompt_files(locale)

    def __copy_prompt_files(self, locale):
        prompts_src_dir = self.locale_prompts_src_dir
        prompts_dst_dir = self.prompts_dir
        prompts_added = list()
        if not os.path.isdir(prompts_dst_dir):
            os.makedirs(prompts_dst_dir)
        for localized_prompt in os.listdir(prompts_src_dir):
            if locale in localized_prompt:
                src = os.path.join(prompts_src_dir, localized_prompt)
                dst = os.path.join(self.prompts_dir, localized_prompt)
                shutil.copy(src, dst)
                prompts_added.append(localized_prompt)
                self._prompts_to_add.append(dst)
        
        if len(prompts_added) < self.PROMPTS_PER_LOCALE:
            self._locales_with_missing_prompt_files.append(locale)
        else:
            self.log.info("Prompts that are added for Locale {} are {}".format(locale, prompts_added))


    def add_prompts_to_project(self):
        return super(AmaLocales, self).add_files_to_project(self.prompts_to_add)

    def add_model_defs_to_app_project(self):
        if not self.default_model:
            return

        available_locales = ','.join('\\"{}\\"'.format(l) for l in self.available_locales_filtered_by_selection)

        defines = [
            'AMA_DEFAULT_LOCALE=\\"{}\\"'.format(self.default_model),
            'AMA_AVAILABLE_LOCALES={}'.format(available_locales)
        ]

        overrides = []
        for locale, override in self._ama_locale_to_models_override.items():
            if locale in self.available_locales_filtered_by_selection:
                overrides.append('{{\\"{}\\",\\"{}\\"}}'.format(locale, override))

        if overrides:
            defines.append('AMA_LOCALE_TO_MODEL_OVERRIDES={}'.format(",".join(overrides)))

        Defines().add_to_project(defines, self.app_project)

    def get_locales_from_vendor_package(self, vendor_package_file):
        zipObj = zipfile.ZipFile(vendor_package_file)
        chip_alias_in_filename = self.chip_type[0:7:1]
        locale_to_models = dict()
        mapfile = ""
        models_folder = ""

        for f in zipObj.namelist():
            if ("kalimba" in f) and (chip_alias_in_filename in f): 
                if (f.endswith("localeToModels.json")):
                    models_folder = "models"
                    mapfile = f;
                    break;
                elif (f.endswith("WakewordModelMapping.json")):
                    models_folder = "wakeword-models"
                    mapfile = f;
                    break;

        if mapfile != "":
            data = json.loads(zipObj.read(mapfile))['alexa']
            for locale, models in data.items():
                #Only 50k models for locale are currently supported
                model_50k = list(s for s in models if '_50k' in s)
                if not model_50k:
                    self.log.warn("Dropping locale {} from available locale list as no 50k model file found".format(locale))
                    continue
                elif len(model_50k) > 1:
                    self.log.warning("More than one model found for: locale - {}, Models - {}".format(locale, model_50k))
                self.model_file_prefix, model_file = model_50k[0].split('/')
                self.model_file_prefix = "{}/{}".format(models_folder, self.model_file_prefix)
                model_file = model_file + ".bin"
                model_file_code = self.__get_locale_from_file(model_file)
                self.locales_map[locale] = {
                    'file': model_file,
                    'code': model_file_code
                }
                locale_to_models[locale] = model_file_code
                if locale != model_file_code:
                    self._ama_locale_to_models_override[locale] = model_file_code

                try:
                    self._ama_model_to_all_supported_locales[model_file_code].append(locale)
                except KeyError:
                    self._ama_model_to_all_supported_locales[model_file_code] = [locale]
    
        for locale in locale_to_models:
            yield locale

    def __get_locale_from_file(self, model_file):
        m = re.match(r".*([a-z]{2}-[A-Z]{2})", model_file)
        locale = m.groups()[0]
        return "{}".format(locale)

    def __get_prompt_extension(self):
        if "INCLUDE_AAC_PROMPTS" in addon_utils.readAppProjectProperty("DEFS", self.app_project):
            return "aac"
        else:
            return "sbc"

    def get_locales_from_prompts(self):
        prompts_dir = self.locale_prompts_src_dir
        found_locales = set()
        for localized_prompt in os.listdir(prompts_dir):
            if localized_prompt.endswith("." + self.__get_prompt_extension()):
                found_locales.add(self.__get_locale_from_file(localized_prompt))

        if not found_locales:
            raise LocalesActionError("No {} prompts found in folder: {}".format(self.__get_prompt_extension(), prompts_dir))

        return found_locales
