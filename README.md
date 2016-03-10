libreactor_couch
================

CouchDB libreactor module

Installation
------------

    ./autogen.sh
    ./configure
    make
    sudo make install

Dependencies
------------

* libreactor_http
* libreactor_net
* libreactor_core
* jansson
        
Tests
-----

Requires cmocka (http://cmocka.org/) to be installed, as well as valgrind (http://valgrind.org/) for memory tests.

    make check
