
srec2dfu Command Line Tool
==========================

Installation
------------

To install this package into an activated virtualenv, run::

    py setup.py install

or run the following if you wish to develop this package further::

    py setup.py develop

This command creates a "link" to the source of this package in the
virtualenv's site-packages.


Running
-------

The command to run this script when in an activated shell is::

    srec2dfu --help

which produces the following help details::

    usage: srec2dfu [-h] [-M MAJOR] [-m MINOR] [-o OUT] file

    Convert srec file to custom upgade file format for dfu file

    positional arguments:
      file                  Path to srec file

    optional arguments:
      -h, --help            show this help message and exit
      -M MAJOR, --major MAJOR
                            Set the major version of the software
      -m MINOR, --minor MINOR
                            Set the minor version of the software
      -o OUT, --out OUT     Path to output file, defaults to same folder as srec
                            file

Testing
-------

To run the very small set of test, install pytest::

    pip install pytest

and then run::

    pytest

In the ``test`` folder there is an example srec and a bin file known to
be correct. A pytest test checks that supplied srec file generates the
equivalent correct bin file.

.. note::
   Pytest is much more powerful test framework than any other
   Python test framework, so it's worth understanding it even for a
   small number of tests. For example one of these tests uses the
   *fixture* concept, in this case the ``tmpdir`` fixture.
