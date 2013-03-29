========================
uCore AutoTest Framework
========================

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 2 $

This document introduces the format of test specifications used in uCore.

.. contents::

Test Specification
==================
A test specification file (testspec) is composed of multiple lines each of which provides a setting or a constraint of the test.

Settings
--------
Lines starting with @ are regarded as setting lines. A setting line defines a variable matching the following pattern

^@<variable name>[:space:]<value>$

The variable name is composed of letters and digits. The supported variables are listed below.

program
  The path of the test case in the sfs image. This variable is expected to be defined in every testspec.

arch
  Architectures on which this test is supposed to run. The value of this variable should be a list of architectures seperated by spaces or commas, e.g. "i386 arm". If not defined, this test will be run on all architectures.

timeout
  How long the simulator instance can run in seconds before it is killed. The value of this variable should be a number. As most test cases may not cost much time to finish their work, the default timeout is 10s. If the test requires more time, define this variable.

sfs_force_rebuild
  Always rebuild sfs image before running this test if this variable is defined. The value of this variable is ignored. This is for tests on file system to make sure former tests can't pollute their test environment.

failin
  The test is expected to fail when executing in the architectures listed. This is for marking hard-to-debug cases and provide a clearer view on old and new bugs.

Constraints
-----------
The lines which doesn't match the pattern of settings lines are considered constraints. A constraint is some text quoted with single quotes and will be used as patterns to match the log of uCore (mostly from the serial port). Some modifiers can be added before the text to change how the match is carried out. Some examples are followed.

'abcd[efg]'
  This constraint requires a line exactly equal to "abcd[efg]" in the log.

\- 'abcd[efg]'
  This constraint requires a line exactly equal to "abcde" or "abcdf" or "abcdg". The hyphen before the pattern means regard this pattern as a regular expression.

! 'abcd[efg]'
  This constraint requires that no line should equal to "abcd[efg]". The exclamatory mark before the pattern means this pattern should not match any line in the log.

! - 'abcd[efg]'
  This constraint requires that no line should equal to "abcde" or "abcdf" or "abcdg".

How to Use AutoBuild System
=========
See https://github.com/chyh1990/autotester_v2

Known Bugs
==========
The parser of testspecs is ultra simple and doesn't have error-checking. Thus its behavior with illegal inputs is uncertain.
