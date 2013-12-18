#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys


try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension


BOOST_PATH = os.environ.get('BOOST_PATH', '.')


if sys.argv[-1] == 'publish':
    os.system('python setup.py sdist upload')
    sys.exit()

readme = open('README.rst').read()
history = open('HISTORY.rst').read().replace('.. :changelog:', '')

setup(
    name='bcse.collections',
    version='0.1.0',
    description='My containers library.',
    long_description=readme + '\n\n' + history,
    author='Grey Lee',
    author_email='bcse@bcse.tw',
    url='https://github.com/bcse/collections',
    packages=['bcse'],
    package_dir={'bcse': 'bcse'},
    ext_modules=[
        Extension('bcse.collections',
            sources=['src/collectionsmodule.cc', 'src/orderedsetobject.cc'],
            depends=['src/orderedsetobject.h'],
            include_dirs=[BOOST_PATH]),
    ],
    include_package_data=True,
    install_requires=[
    ],
    license="BSD",
    zip_safe=False,
    keywords='collections orderedset',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Operating System :: OS Independent',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Natural Language :: English',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.5',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.1',
        'Programming Language :: Python :: 3.2',
        'Programming Language :: Python :: 3.3',
        'Topic :: Software Development :: Libraries :: Python Modules'
    ],
    platforms=['any'],
    test_suite='tests',
)
