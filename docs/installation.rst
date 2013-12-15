============
Installation
============

You should define BOOST_PATH before install. For example::

    $ BOOST_PATH=/usr/local/boost_1_55_0
    $ export BOOST_PATH

Or, on Windows::

    $ SET BOOST_PATH=C:\boost_1_55_0

And then install with command line::

    $ easy_install bcse.collections

Or, if you have virtualenvwrapper installed::

    $ mkvirtualenv bcse.collections
    $ pip install bcse.collections
