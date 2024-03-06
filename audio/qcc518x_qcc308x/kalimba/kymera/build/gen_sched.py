#!/usr/bin/env python
############################################################################
# Copyright (c) 2020 Qualcomm Technologies International, Ltd.
############################################################################
"""
Generate a set of files that describe the static tasks and background interrupts
used by Kymera based on the content of a list of XML files specified on the command line.

Each XML file should look like this:
<scheduler>
    <task>
        <name value="TASK_1"/>
        <init value="task1Init"/>
        <handler value="task1InitHandler"/>
        <queue value="TASK_1_QUEUE"/>
        <priority value="LOW_PRIORITY"/>
        <type value="my_type"/>
    </task>
    <task>
        <onlyif value="defined(SUPPORTS_SECOND_TASK)"/>
        <name value="TASK_2"/>
        <init value="task2Init"/>
        <handler value="task2InitHandler"/>
        <queue value="TASK_2_QUEUE"/>
    </task>
    <bgint>
        <name value="BGINT_1"/>
        <handler value="plTask1BgHandler"/>
        <priority value="LOW_PRIORITY"/>
    </bgint>
</scheduler>

Each XML file must contain one "scheduler" element containing at least
one "task" element or one "bgint" element. Both type of elements must contain
a "name" element and a "handler" element. Moreover "task" elements must contain
one "init" element and one "queue" element.

If a "priority" element is not present for a given task or background interrupt,
then the lowest priority will be used. Valid values are listed in the PRIORITY
enumeration from "sched_oxygen.h".

The "init" and "handler" elements contain the name of the function to be called
to initialise the task or background interrupt. Currently specifying the "init"
element of a task is not optional.

The optional "type" element can be used only in a "task" element. It is used
to indicate to ACAT the type of the pointer passed to the handler function. This
is needed because all handler functions have the same prototype and must cast
their argument to their internal type.

The optional "cores" element indicates which the cores a task or background
interrupt should run on. Possible values are: PROC_BIT_PROCESSOR_0, PROC_BIT_PROCESSOR_1
and PROC_BIT_PROCESSOR_ALL. Default is PROC_BIT_PROCESSOR_ALL.

The "onlyif" element allows making a task or background interrupt available based
on the value of a pre-preprocessor symbol. For example, the previous example
causes the tool to generate this definition:

/* Use one of the symbols in this enumeration when calling functions
   "put_message_with_routing" or "put_message". */
typedef enum
{
    TASK_1_QUEUE = 0x100,
#if defined(SUPPORTS_SECOND_TASK)
    TASK_2_QUEUE = 0x0,
#endif
    QUEUE_INVALID = -1
} QUEUE_IDS;

The tool generates one C file called "sched_static.c" that is build as part of
the "sched_oxygen" component, one header file called "sched_ids.h" included by
"sched_oxygen.h", one header file called "sched_count.h" included by the scheduler
and finally one header file by component supplying a XML file. The file name of each
of the per-component header files is created by replacing the extension of the XML
file by ".h". The files are stored in the directory specified on the command line.
"""

from __future__ import print_function
import argparse
import datetime
import os
import sys
from xml.dom import minidom

if sys.version_info.major:
    class FileNotFoundError(OSError): # pylint: disable=too-few-public-methods, redefined-builtin
        """
        Forward compatibility with Python 3.
        """

class InvalidSchedulerDeclaration(Exception): # pylint: disable=too-few-public-methods, redefined-builtin
    """
    Forward compatibility with Python 3.
    """

class IdFactory():
    """
    Class to be used by factory objects that generate unique scheduler id for
    both static tasks and static background interrupts.
    """

    PRIORITIES = {"LOWEST_PRIORITY" : 0,
                  "LOW_PRIORITY" : 1,
                  "MID_PRIORITY" : 2,
                  "HIGH_PRIORITY" : 3,
                  "HIGHEST_PRIORITY" : 4,
                  "DEFAULT_PRIORITY" : 0}
    _NUMBER_PRIORITIES = 5

    def __init__(self):
        """
        @brief Initialise a IdFactory object.
        """

        self.ids = [1] * self._NUMBER_PRIORITIES

    def generate_id(self, priority):
        """
        @brief Generate a unique id for a scheduler object running at the specified priority.
        @param[in] priority A string specifying the priority level.
        """

        if priority not in self.PRIORITIES:
            message = 'Unknown priority level "{0}"'.format(priority)
            raise InvalidSchedulerDeclaration(message)
        level = self.PRIORITIES[priority]

        result = self.ids[level]
        self.ids[level] += 1
        return result

    def get_queue_id(self, priority, identifier, is_bgint=False):
        """
        @brief Combine an id and priority into a a taskid.
        @param[in] priority A string specifying the priority level.
        @param[in] identifier A unique id as returned by generate_id.
        """

        if priority not in self.PRIORITIES:
            message = 'Unknown priority level "{0}"'.format(priority)
            raise InvalidSchedulerDeclaration(message)
        level = self.PRIORITIES[priority]

        result = (level << 8) + identifier
        if is_bgint:
            # BG_INT_FLAG_BIT must be set
            result |= (1 << 24)
        return result

class GeneratedFile():
    """
    A class representing a generated file.

    Data can be appended to the generated file like a normal string.
    When the object is destroyed it will check if th content stored in
    it matches the content of the output file. If so the output file
    will be left unmodified to avoid triggering make dependencies.
    Otherwise, the content will be used to overwrite the file.
    """
    def __init__(self, output_path):
        """
        @brief Initialise a GeneratedFile object.
        @param[in] output_path Path to the generated file.
        """

        self.content = ""
        self.previous_content = ""
        # The specified file will be overwritten so it is called
        # output_path even though the first step is to read its
        # content.
        self.output_path = output_path
        if os.path.isfile(output_path):
            with open(output_path, "r") as targetfile:
                self.previous_content = targetfile.read()

    def __enter__(self):
        """
        @brief Return the object itself when created using "with".
        """
        return self

    def __exit__(self, exception_type, value, traceback):
        """
        @brief Flush the content of a GeneratedFile object and destroy it.
        """
        if self.content != self.previous_content:
            with open(self.output_path, "w") as targetfile:
                targetfile.write(self.content)

    def __add__(self, line):
        """
        @brief Append a string to the file to be generated.
        @param[in] string Path to the generated file.
        """

        self.content += line
        return self

class SchedulerTasksDefinition(GeneratedFile): # pylint: disable=too-few-public-methods
    """
    A class representing a C file containing the definition of the scheduler's tasks array.
    """
    def __init__(self, output_path):
        """
        @brief Initialise a SchedulerTasksDefinition object.
        @param[in] output_path Path to the output C file.
        """

        GeneratedFile.__init__(self, output_path)
        now = datetime.datetime.now()
        self += "/****************************************************************************\n"
        self += " * Copyright (c) {0} Qualcomm Technologies International, Ltd.\n".format(now.year)
        self += "****************************************************************************/\n"
        self += "/**\n"
        self += " * \\file {0}\n".format(os.path.basename(output_path))
        self += " * \\ingroup sched_oxygen\n"
        self += " *\n"
        self += " * Definition of the scheduler's tasks array and scheduler's background\n"
        self += " * interrupts array.\n"
        self += " */\n"
        self += "\n"
        self += '#include "sched_oxygen/sched_oxygen_private.h"\n\n'
        self += "/* Define a macro to declare the mapping between a task handler function\n"
        self += "   and the type used by this function.\n"
        self += "   The information is stored in the ELF and used by ACAT. */\n"
        self += "#ifdef __KCC__\n"
        self += '#define GLOBAL_DEBUG_STRING_ATTR _Pragma("datasection GLOBAL_DEBUG_STRINGS")\n'
        self += "#define SCHED_MAP_HANDLER_DATA(HANDLER, HTYPE) GLOBAL_DEBUG_STRING_ATTR \\\n"
        self += '     const char ACAT_SCHED_TYPE_##HANDLER[] = "" #HTYPE "";\n'
        self += "#else /* __KCC__ */\n"
        self += "#define SCHED_MAP_HANDLER_DATA(HANDLER, HTYPE)\n"
        self += "#endif /* __KCC__ */\n\n"

class SchedulerCountHeader(GeneratedFile): # pylint: disable=too-few-public-methods
    """
    A class representing a header file containing the definition of the scheduler's tasks array.
    """
    def __init__(self, output_path):
        """
        @brief Initialise a SchedulerCountHeader object.
        @param[in] output_path Path to the output C file.
        """

        GeneratedFile.__init__(self, output_path)
        now = datetime.datetime.now()
        guard = os.path.basename(output_path).upper()
        guard = guard.replace('.', '_')
        self += "/****************************************************************************\n"
        self += " * Copyright (c) {0} Qualcomm Technologies International, Ltd.\n".format(now.year)
        self += "****************************************************************************/\n"
        self += "/**\n"
        self += " * \\file {0}\n".format(os.path.basename(output_path))
        self += " * \\ingroup sched_oxygen\n"
        self += " *\n"
        self += " * Declaration of the number of static tasks and static background interrupts\n"
        self += " * available in the firmware.\n"
        self += " */\n"
        self += "\n"
        self += "#ifndef {0}\n".format(guard)
        self += "#define {0}\n\n".format(guard)

    def __exit__(self, exception_type, value, traceback):
        """
        @brief Flush the content of a GeneratedFile object and destroy it.
        """
        self += "#endif\n"
        GeneratedFile.__exit__(self, exception_type, value, traceback)

class SchedulerIdsHeader(GeneratedFile): # pylint: disable=too-few-public-methods
    """
    A class representing a header file containing the definition of the number of static tasks
    and static background interrupts present in the firmware.
    """
    def __init__(self, output_path):
        """
        @brief Initialise a SchedulerIdsHeader object.
        @param[in] output_path Path to the output C file.
        """

        GeneratedFile.__init__(self, output_path)
        now = datetime.datetime.now()
        guard = os.path.basename(output_path).upper()
        guard = guard.replace('.', '_')
        self += "/****************************************************************************\n"
        self += " * Copyright (c) {0} Qualcomm Technologies International, Ltd.\n".format(now.year)
        self += "****************************************************************************/\n"
        self += "/**\n"
        self += " * \\file {0}\n".format(os.path.basename(output_path))
        self += " * \\ingroup sched_oxygen\n"
        self += " *\n"
        self += " * Definition of the scheduler's queue ids and background interrupts ids.\n"
        self += " */\n"
        self += "\n"
        self += "#ifndef {0}\n".format(guard)
        self += "#define {0}\n\n".format(guard)

    def __exit__(self, exception_type, value, traceback):
        """
        @brief Flush the content of a GeneratedFile object and destroy it.
        """
        self += "#endif\n"
        GeneratedFile.__exit__(self, exception_type, value, traceback)

class ComponentHeader(GeneratedFile): # pylint: disable=too-few-public-methods
    """
    A class representing a header file that a component has to include.
    """
    def __init__(self, output_path):
        """
        @brief Initialise a ComponentHeader object.
        @param[in] output_path Path to the output header file.
        """

        GeneratedFile.__init__(self, output_path)
        now = datetime.datetime.now()
        guard = os.path.basename(output_path).upper()
        guard = guard.replace('.', '_')
        self += "/****************************************************************************\n"
        self += " * Copyright (c) {0} Qualcomm Technologies International, Ltd.\n".format(now.year)
        self += "****************************************************************************/\n"
        self += "/**\n"
        self += " * \\file {0}\n".format(os.path.basename(output_path))
        self += " * \\ingroup sched_oxygen\n"
        self += " *\n"
        self += " * Declaration of a component's functions used by the scheduler.\n"
        self += " */\n"
        self += "\n"
        self += "#ifndef {0}\n".format(guard)
        self += "#define {0}\n\n".format(guard)

    def __exit__(self, exception_type, value, traceback):
        """
        @brief Flush the content of a GeneratedFile object and destroy it.
        """
        self += "#endif\n"
        GeneratedFile.__exit__(self, exception_type, value, traceback)


class ComponentDeclaration():
    """
    A class representing the declaration of the static tasks and static background interrupts
    used by a component.

    Each object is created from a XML file.
    """
    
    PROC_BIT_FIELD = {"PROC_BIT_PROCESSOR_0" : 1,
                      "PROC_BIT_PROCESSOR_1" : 2,
                      "PROC_BIT_PROCESSOR_ALL" : 3}

    def __init__(self, input_path, task_id_factory, bgint_id_factory):
        """
        @brief Initialise a ComponentDeclaration object.
        @param[in] input_path Path to the input XML file.
        @param[in] task_id_factory Instance of IdFactory.
        @param[in] bgint_id_factory Instance of IdFactory.
        """

        # Each component can have any number of tasks and background interrupts.
        self._tasks = []
        self._bgints = []
        self._task_id_factory = task_id_factory
        self._bgint_id_factory = bgint_id_factory
        if os.path.isfile(input_path):
            _, self.header_name = os.path.split(input_path)
            self.header_name, _ = os.path.splitext(self.header_name)
            self.header_name = self.header_name + ".h"
        else:
            message = "Component's XML file "
            message += '"{0}" not found'.format(input_path)
            raise FileNotFoundError(message)

        doc = minidom.parse(input_path)
        scheduler = doc.getElementsByTagName("scheduler")[0]

        message = ""
        try:
            tasks = scheduler.getElementsByTagName("task")
            for task in tasks:
                result = self.__process_task(task)
                if result is not None:
                    self._tasks.append(result)

            bgints = scheduler.getElementsByTagName("bgint")
            for bgint in bgints:
                result = self.__process_bgint(bgint)
                if result is not None:
                    self._bgints.append(result)
        except InvalidSchedulerDeclaration as error:
            message = str(error)
            message += ' in file "{0}".'.format(input_path)

        if message:
            raise InvalidSchedulerDeclaration(message)

    def __process_task(self, task):
        """
        @brief Extract a task's information from a XML element.
        @param[in] task Instance of xml.dom.minidom.Element.
        """

        attributes = {"name" : None,
                      "init" : None,
                      "handler" : None,
                      "queue" : None,
                      "onlyif" : None,
                      "priority" : "DEFAULT_PRIORITY",
                      "cores" : "PROC_BIT_PROCESSOR_ALL",
                      "type" : None}
        for key in attributes:
            element = task.getElementsByTagName(key)
            if len(element) == 1:
                attributes[key] = element[0].getAttribute("value")

        for element in ["name", "init", "handler", "queue"]:
            if not attributes[element]:
                message = 'Missing element "{0}" in task declaration'.format(element)
                raise InvalidSchedulerDeclaration(message)

        if attributes["cores"] not in self.PROC_BIT_FIELD:
                message = 'Invalid cores element "{0}" in task declaration'.format(attributes["cores"])
                raise InvalidSchedulerDeclaration(message)

        attributes["id"] = self._task_id_factory.generate_id(attributes["priority"])
        return attributes

    def __process_bgint(self, bgint):
        """
        @brief Extract a background interrupt's information from a XML element.
        @param[in] bgint Instance of xml.dom.minidom.Element.
        """
        attributes = {"name" : None,
                      "handler" : None,
                      "onlyif" : None,
                      "priority" : "DEFAULT_PRIORITY",
                      "cores" : "PROC_BIT_PROCESSOR_ALL"}

        for key in attributes:
            element = bgint.getElementsByTagName(key)
            if len(element) == 1:
                attributes[key] = element[0].getAttribute("value")

        for element in ["name", "handler"]:
            if not attributes[element]:
                message = 'Missing element "{0}" in bgint declaration'.format(element)
                raise InvalidSchedulerDeclaration(message)

        if attributes["cores"] not in self.PROC_BIT_FIELD:
                message = 'Invalid cores element "{0}" in task declaration'.format(attributes["cores"])
                raise InvalidSchedulerDeclaration(message)

        attributes["id"] = self._bgint_id_factory.generate_id(attributes["priority"])
        return attributes

    def get_filename(self):
        """
        @brief Extract a background interrupt's information from a XML element.
        @param[in] bgint Instance of xml.dom.minidom.Element.
        """

        return self.header_name

    def get_prototypes(self):
        """
        @brief Get a string containining the prototypes of functions used by the tasks and
        background interrupts of the component.
        """

        lines = ""
        for task in self._tasks:
            if task["onlyif"]:
                lines += "#if {0}\n".format(task["onlyif"])
            lines += "extern void {0}(void **);\n".format(task["init"])
            lines += "extern void {0}(void **);\n".format(task["handler"])
            if task["onlyif"]:
                lines += "#endif\n"

        for bgint in self._bgints:
            if bgint["onlyif"]:
                lines += "#if {0}\n".format(bgint["onlyif"])
            lines += "extern void {0}(void **);\n".format(bgint["handler"])
            if bgint["onlyif"]:
                lines += "#endif\n"

        return lines

    def get_task_count(self):
        """
        @brief Get a string containining a list used by the compiler
        to calculate the number of static scheduler tasks.
        """

        lines = ""
        for task in self._tasks:
            if task["onlyif"]:
                lines += "#if {0}\n".format(task["onlyif"])
            lines += "    sched_count_task_{0},\n".format(task["name"])
            if task["onlyif"]:
                lines += "#endif\n"

        return lines

    def get_queue_declaration(self):
        """
        @brief Get a string containining the declaration of a queue.
        """

        lines = ""
        for task in self._tasks:
            if task["onlyif"]:
                lines += "#if {0}\n".format(task["onlyif"])
            taskid = self._task_id_factory.get_queue_id(task["priority"],
                                                        task["id"])
            lines += "    {0} = {1},\n".format(task["queue"],
                                               hex(taskid))
            if task["onlyif"]:
                lines += "#endif\n"

        return lines

    def get_task_definition(self):
        """
        @brief Get a string containining the definition of a task.
        """

        lines = ""
        for task in self._tasks:
            if task["onlyif"]:
                lines += "#if {0}\n".format(task["onlyif"])
            lines += "    /* {0} */\n".format(task["name"])
            lines += "    {\n"
            lines += "        {0},\n".format(task["cores"])
            lines += "        {0},\n".format(task["priority"])
            lines += "        {0},\n".format(task["id"])
            lines += "        0,\n"
            lines += "        {0},\n".format(task["init"])
            lines += "        {0},\n".format(task["handler"])
            lines += "    },\n"
            if task["onlyif"]:
                lines += "#endif\n"
        return lines

    def get_task_types(self):
        """
        @brief Get a string declaring the type of the private pointer
        of a type to help ACAT analyze it.
        """
        lines = ""
        for task in self._tasks:
            if task["type"]:
                if task["onlyif"]:
                    lines += "#if {0}\n".format(task["onlyif"])
                lines += "SCHED_MAP_HANDLER_DATA({0}, {1})\n".format(task["handler"],
                                                                     task["type"])
                if task["onlyif"]:
                    lines += "#endif\n"
        return lines


    def get_bgint_count(self):
        """
        @brief Get a string containining a list used by the compiler
        to calculate the number of static background interrupts.
        """

        lines = ""
        for bgint in self._bgints:
            if bgint["onlyif"]:
                lines += "#if {0}\n".format(bgint["onlyif"])
            lines += "    sched_count_bgint_{0},\n".format(bgint["name"])
            if bgint["onlyif"]:
                lines += "#endif\n"
        return lines

    def get_bgint_declaration(self):
        """
        @brief Get a string containining the declaration of a background interrupt.
        """

        lines = ""
        for bgint in self._bgints:
            if bgint["onlyif"]:
                lines += "#if {0}\n".format(bgint["onlyif"])
            taskid = self._task_id_factory.get_queue_id(bgint["priority"],
                                                        bgint["id"],
                                                        True)
            lines += "    {0}_bg_int_id = {1},\n".format(bgint["name"], hex(taskid))
            if bgint["onlyif"]:
                lines += "#endif\n"
        return lines

    def get_bgint_definition(self):
        """
        @brief Get a string containining the definition of a background interrupt.
        """

        lines = ""
        for bgint in self._bgints:
            if bgint["onlyif"]:
                lines += "#if {0}\n".format(bgint["onlyif"])
            lines += "    /* {0} */\n".format(bgint["name"])
            lines += "    {\n"
            lines += "        {0},\n".format(bgint["cores"])
            lines += "        {0},\n".format(bgint["priority"])
            lines += "        {0},\n".format(bgint["id"])
            lines += "        0,\n"
            lines += "        {0},\n".format(bgint["handler"])
            lines += "    },\n"
            if bgint["onlyif"]:
                lines += "#endif\n"
        return lines

class OutputGenerator():
    """
    A class used to generate the files needed by the scheduler.
    """
    def __init__(self, output_path, components):
        """
        @brief Initialise a OutputGenerator object.
        @param[in] output_path Path to the directory to put the output files into.
        @param[in] components A list of ComponentDeclaration objects.
        """

        self._target_directory = output_path
        self._components = components

    def generate_header_file(self):
        """
        @brief Generate a header file used exclusively by the scheduler.
        """

        task_count = ""
        bgint_count = ""
        for component in self._components:
            task_count += component.get_task_count()
            bgint_count += component.get_bgint_count()

        filename = self._target_directory + os.sep + "sched_count.h"
        with SchedulerCountHeader(filename) as output:
            if task_count:
                output += "/* The only relevant symbol in this enumeration is N_TASKS. */\n"
                output += "typedef enum\n{\n"
                output += task_count
                output += "    N_TASKS\n"
                output += "} TASK_COUNT;\n\n"
            else:
                output += "typedef enum\n{\n"
                output += "    N_TASKS = 0\n"
                output += "} TASK_COUNT;\n\n"

            if bgint_count:
                output += "/* The only relevant symbol in this enumeration is N_BG_INTS. */\n"
                output += "typedef enum\n{\n"
                output += bgint_count
                output += "    N_BG_INTS\n"
                output += "} BGINT_COUNT;\n"
            else:
                output += "typedef enum\n{\n"
                output += "    N_BG_INTS = 0\n"
                output += "} BGINT_COUNT;\n"

    def generate_source_file(self):
        """
        @brief Generate a source file used by the scheduler component itself.
        """

        inclusions = ""
        task_definition = ""
        task_types = ""
        bgint_definition = ""
        for component in self._components:
            inclusions += '#include "sched_oxygen/{0}"\n'.format(component.get_filename())
            task_definition += component.get_task_definition()
            task_types += component.get_task_types()
            bgint_definition += component.get_bgint_definition()

        filename = self._target_directory + os.sep + "sched_static.c"
        with SchedulerTasksDefinition(filename) as output:
            output += inclusions + "\n"

            if task_definition:
                # The theoretical case of having all tasks disabled
                # by the pre-processor is not supported.
                output += "const STATIC_TASK static_tasks[] =\n{\n"
                output += task_definition
                output += "};\n\n"
                output += task_types + "\n"
            else:
                output += "const STATIC_TASK static_tasks[1];\n\n"
                output += "#define N_TASKS 0\n\n"

            if bgint_definition:
                # The theoretical case of having all background interrupts disabled
                # by the pre-processor is not supported.
                output += "const STATIC_BGINT static_bgints[] =\n{\n"
                output += bgint_definition
                output += "};\n"
            else:
                output += "const STATIC_BGINT static_bgints[1];\n\n"
                output += "#define N_BG_INTS 0\n\n"

    def generate_components_header(self):
        """
        @brief Generate header files included by the components that use either
        static tasks or static background interrupts.
        """

        for component in self._components:
            prototypes = component.get_prototypes()
            filename = self._target_directory + os.sep + component.get_filename()
            with ComponentHeader(filename) as output:
                output += prototypes

    def generate_id_header(self):
        """
        @brief Generate a header file that declare the types needed to send
        a message to a static task or to raise a background interrupt.
        """

        queue_declaration = ""
        bgint_declaration = ""
        for component in self._components:
            queue_declaration += component.get_queue_declaration()
            bgint_declaration += component.get_bgint_declaration()

        filename = self._target_directory + os.sep + "sched_ids.h"
        with SchedulerIdsHeader(filename) as output:
            output += '/* Use one of the symbols in this enumeration when calling functions\n'
            output += '   "put_message_with_routing" or "put_message". */\n'
            output += "typedef enum\n{\n"
            output += queue_declaration
            output += "    QUEUE_INVALID = -1\n"
            output += "} QUEUE_IDS;\n\n"

            output += '/* Use one of the symbols in this enumeration when calling functions\n'
            output += '   "put_message_with_routing" or "put_message". */\n'
            output += "typedef enum\n{\n"
            output += bgint_declaration
            output += "    BGINT_INVALID = -1\n"
            output += "} BGINT_IDS;\n"

def create_placeholders(target_directory):
    """
    @brief Create the minimum set of files expected by the build environment even though
    they contain no static tasks or background interrupts.
    @param[in] target_directory Path to the directory to store the files in.
    """

    filename = target_directory + os.sep + "sched_count.h"
    output = SchedulerCountHeader(filename)
    with SchedulerCountHeader(filename) as output:
        output += "typedef enum\n{\n"
        output += "    N_TASKS = 0\n"
        output += "} TASK_COUNT;\n\n"

        output += "typedef enum\n{\n"
        output += "    N_BG_INTS = 0\n"
        output += "} BGINT_COUNT;\n"

    filename = target_directory + os.sep + "sched_ids.h"
    with SchedulerIdsHeader(filename) as output:
        output += "typedef enum\n{\n"
        output += "    QUEUE_INVALID = -1\n"
        output += "} QUEUE_IDS;\n\n"

        output += "typedef enum\n{\n"
        output += "    BGINT_INVALID = -1\n"
        output += "} BGINT_IDS;\n"

    filename = target_directory + os.sep + "sched_static.c"
    with SchedulerTasksDefinition(filename) as output:
        output += "const STATIC_TASK static_tasks[1];\n\n"
        output += "#define N_TASKS 0\n\n"

        output += "const STATIC_BGINT static_bgints[1];\n\n"
        output += "#define N_BG_INTS 0\n\n"

if __name__ == '__main__':
    # Parse the command line arguments.
    PARSER = argparse.ArgumentParser(description="Generate a header file and a C file " +
                                     "containing the definition of the types and the " +
                                     "declaration of the variables that describe the static " +
                                     "scheduler tasks used by Kymera.")
    PARSER.add_argument("-o",
                        action="store", type=str, dest="target_directory", required=True,
                        help="The path of the directory to store the generated files in.")
    PARSER.add_argument(dest="declarations",
                        help="A list of XML files containing the declaration of static " +
                        "scheduler tasks.",
                        type=str, nargs='*')
    ARGUMENTS = PARSER.parse_args()

    # If there are no declarations, it means either that the scheduler is not included
    # or that the build is meant for a unit test that includes the scheduler but uses it
    # only with dynamic tasks or background interrupts.
    if not ARGUMENTS.declarations:
        create_placeholders(ARGUMENTS.target_directory)
        sys.exit(0)

    # Instanciate factory objects.
    TASK_ID_FACTORY = IdFactory()
    BGINT_ID_FACTORY = IdFactory()

    # Parse per-component XML files.
    COMPONENTS = []
    try:
        for xml_filename in ARGUMENTS.declarations:
            declaration = ComponentDeclaration(xml_filename, TASK_ID_FACTORY, BGINT_ID_FACTORY)
            COMPONENTS.append(declaration)
    except InvalidSchedulerDeclaration as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)

    # Generate the output files if needed.
    OUTPUT_GENERATOR = OutputGenerator(ARGUMENTS.target_directory, COMPONENTS)
    OUTPUT_GENERATOR.generate_id_header()
    OUTPUT_GENERATOR.generate_source_file()
    OUTPUT_GENERATOR.generate_header_file()
    OUTPUT_GENERATOR.generate_components_header()
