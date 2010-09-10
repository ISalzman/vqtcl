#! /usr/bin/env tclkit85
# V9 wrapper to run all tests suites
# 2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
# $Id$
   
source [file join [file dirname [info script]] setup.tcl]

runAllTests
