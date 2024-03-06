1. XML Schema:

Here we will discuss about the currently proposed XML schema to generate various reports. Note that this schema is subjected to evolve based on new data structures being identified in the implementation of other modules. Current schema is written based on the example structures of CM and HF modules.

1.1 Elements

In this section we will discuss about the elements which are used to generate reports.

Element: <REPORT>
Description: This is used to represent individual module report inside synergy.	
Attributes:
	- group: gives unique name to the report, can be used inside the parsing script.
	- global_name: main global variable which is used to generate the report. This acts as a base for all the other names present inside the child elements of <REPORT>.
	- title: title of the individual report, this will be printed in the beginning of the report.

Element: <COLUMN>
Description: This is used to provide contents to the table format in case of overview report. This has no usage for detailed report.	
Attributes:
	- title: title of the individual column in the table.
	- data_type: Represents the preprocessing to be applied to the contents of this column. Refer to table in 1.2.2 "Column Data Types for Preprocessing".
	- name: This provides the path of the variable which is used to fill the contents with this column. This path comes from the  attribute global_name of <REPORT> element.

Element: <TABLE>
Description: This has duel usage for both overview and detailed report. In case of overview report, it acts as a normal table where multiple attributes like data_type, title etc. can be set for the overview table. In case of detailed report, this creates a section, with the title as value of attribute title and populates it with the variable provided in attribute name.	
Attributes:
	- title: title of the individual table in overview and section inside detailed report.
	- name: provides the variable reference to populate the table in overview and section in detailed report.
	- data_type: Suggests the type of variable mentioned by attribute name. Refer to table in 1.2.1 "Table Data Types"

1.2 Data Types

This section talks about various data types which are used for elements as well as individual column contents.

1.2.1 Table Data Types

Table data types are used to indicate to the parsing script whether a given table is a static or an iterable object. Following are the data types which are used in this template. Any change in the table data types shall be reflected in the following table.

Element: regular
Description: This indicates the contents of this table is not an iterable object and hence the variable mentioned by attribute name of the table needs to be accessed as a normal variable (e.g. structure). This applies to both overview and detailed reports. This is equivalent to do a simple printing of variable mentioned by attribute name.
Attributes: N/A

Element: array
Description: This indicates that the contents of this table is filled with an array where the base of the array is represented by attribute array_ptr and the attribute length represents the number of elements in the array.	
Attributes:
	- array_ptr: provides the base pointer of the array to be parsed.
	- length: number of elements present in the array.

Element: Instanced_CsrCmnListSimple_t
Description: This is a representation of CsrCmnListSimple_t present in the synergy library where each element of this list contains a pointer to a main structure from where the details need to be taken. 	
Attributes:
	- name: this provides the base pointer of the list to be iterated.
	- lvl1_element_type: this provides the type of element inside the list.
	- lvl2_element_ptr: this is the structure pointer which is present inside each element that contains the data to be printed.
	- lvl2_element_type: this is the data type of the pointer which is used to fill the content of the table.

Element: CsrCmnListSimple_t
Description: This is a representation of CsrCmnListSimple_t present in the synergy library, here the element information is directly used to fill the table contents.
Attributes:
	- name: this provides the base pointer of the list to be iterated.
	- next_ptr: provides information on how to fetch the next element of the list.
	- lvl1_element_type: this provides the type of element inside the list.

Element: linked_list_ptr
Description: This is a representation of regular linked list with only one difference that the base of the linked list is a pointer to the head pointer (double pointer) of the linked list.	
Attributes:
	- name: this provides the base pointer of the list to be iterated. This in itself is a pointer pointing to the base of the list.
	- next_ptr: provides information on how to fetch the next element of the list.
	- lvl1_element_type: this provides the type of element inside the list.

Element: CsrCmnListSimple_t_ptr
Description: This is same as CsrCmnListSimple_t with the only difference of the base of the linked list is a pointer to the head pointer of the linked list.	
Attributes: Same as in CsrCmnListSimple_t.

Element: linked_list
Description: This is a representation of a generic linked list where the head pointer is directly provided.	
Attributes:
	- name: this provides the base pointer of the list to be iterated.
	- next_ptr: provides information on how to fetch the next element of the list.
	- lvl1_element_type: this provides the type of element inside the list.

1.2.2 Column Data Types for Preprocessing

The script-schema also allows you to add preprocessing for a given data of a column. Note that, this is used in overview reports only. In case of detailed report all the prints are raw. An example of a preprocessing is to convert a <nap, uap, lap> bd address into a single value. Following are some of the data types added for preprocessing. Any change in the preprocessing functionality shall be reflected in the following table.

DataType: TaskHandler
Description: This fetches the callback handler function from the given Application Handle value (appHandle).

DataType: CsrBtDeviceAddr	
Description: This reformats the CsrBtDeviceAddr into a simple easily readable form.

DataType: CsrBtCmStateL2cap
Description: This fetches the string value of the defines representing CsrBtCmStateL2cap values.

DataType: CsrBtCmStateRfc
Description: This fetches the string value of the defines representing CsrBtCmStateRfc values.

2. Script Details

Location: <vm_synergy_root>\adk\src\libs\synergy\tools\synergy_report_tool\report_gen.py

2.1 Loading

Loading the script at the time of attaching the device (live session):

Command: py .\pydbg.py -f apps1:"<path_to_earbud_elf>" <path_to_script>

Example: py .\pydbg.py -f apps1:"C:\Work\Convergence\test\earbud.elf" C:\Work\Convergence\test\vm_synergy\adk\src\libs\synergy\tools\synergy_report_tool\report_gen.py

The script can also be used at the time of loading coredump.

Command: py .\pydbg.py -d xcd3:"<path_to_xcd>" -f apps1:"<path_to_earbud_elf>"

Example: py .\pydbg.py -d xcd3:"C:\Work\Convergence\test\dump.xcd" -f apps1:"C:\Work\Convergence\test\earbud.elf" C:\Work\Convergence\test\vm_synergy\adk\src\libs\synergy\tools\synergy_report_tool\report_gen.py

2.2 Usage

This script can be used to generate overview and detailed reports.

Overview Report: 

>>> report_gen.generate_overview_report()

Detailed report:

>>> report_gen.generate_detailed_report()