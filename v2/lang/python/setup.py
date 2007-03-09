#! /usr/bin/env python
# setup.py -=- Configuration settings for Python's "distutils"

from distutils.core import setup, Extension

setup(name         = "thrive",
      version      = "2.23",
      url          = "http://www.vlerq.org/",
      author       = "Jean-Claude Wippler",
      author_email = "jcw@equi4.com",
      ext_modules  = [Extension("thrive",["thrive.c"])])
