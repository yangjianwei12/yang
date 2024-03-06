"""
Copyright (c) Qualcomm Technologies International, Ltd.
"""

from pathlib import Path
from filecmp import cmp
from difflib import diff_bytes

import pytest

from srec2dfu import convert_file, main


def test_generation(tmpdir: Path):
    """Check against a supplied srec and bin file
    """
    srec = Path(__file__).parent / 'case_dfu.srec'
    orig_bin = Path(__file__).parent / 'case_dfu.bin'
    gen_bin = tmpdir / 'case_dfu.bin'
    result = convert_file(str(srec), str(gen_bin), 1, 1)
    assert result == 0
    assert cmp(str(orig_bin), str(gen_bin))


def test_help(capsys):
    """Check that the --help command line is right.
    """
    # a "srec2dfu --help" should raise a SystemExit(0) error
    with pytest.raises(SystemExit) as e_info:
        main(['--help'])

    # does the script want to exit with code 0
    assert e_info.value.code == 0
    # grab the stdout of this script
    output = str(capsys.readouterr().out)
    # check that some of the options should be listed
    assert ('[-h]' in output)
    assert ('[-M MAJOR]' in output)
    assert ('[-m MINOR]' in output)
    assert ('[-o OUT]' in output)
