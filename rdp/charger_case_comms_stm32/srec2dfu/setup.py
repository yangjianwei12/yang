"""
Copyright (c) Qualcomm Technologies International, Ltd.
"""

from setuptools import setup

setup(
    name='srec2dfu',
    version='1.0.0',
    license='Proprietary License',
    author='Qualcomm Techonlogies International, Ltd',
    description='A tool to convert SREC DFU file to SREC BIN file',
    long_description=open('README.rst', encoding='utf-8').read(),
    zip_safe=False,
    packages=['srec2dfu'],
    entry_points = {
        'console_scripts': [
            'srec2dfu = srec2dfu:main',
        ]
    },
    install_requires=[
    ],
    classifiers=[
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
)
