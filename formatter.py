#!/usr/bin/env python

import os, sys
import re

if len(sys.argv) < 5:
    print 'Usage: formatter.py <section name> <result-dir> <repo> <tid>'
    sys.exit()

tid_regex = re.compile('([a-z0-9]+)-.*')
arch_regex = re.compile('=+ ([\w]+) =+')
result_regex = re.compile('(<.*> )([\w]+)( *)(\[.*\])')
result_formats = {
    '[  PASS  ]':'line-ok',
    '[! PASS !]':'line-warning',
    '[  FAIL  ]':'line-warning',
    '[! FAIL !]':'line-error',
    '[!BROKEN!]':'line-error'
}

section = sys.argv[1]
result_dir = sys.argv[2]
repo = sys.argv[3]
tid = sys.argv[4]
m = tid_regex.match(tid)
if not m:
    print 'Invalid tid'
    sys.exit()
commit = m.group(1)

if section == 'AutoTest':
    sys.stdout.write('Legend:<br>')
    sys.stdout.write('<span class="line-ok">[  PASS  ]</span>:\tThe test terminates successfully with correct output<br>')
    sys.stdout.write('<span class="line-warning">[! PASS !]</span>:\tThe test passes, but it is expected to fail in previous builds<br>')
    sys.stdout.write('<span class="line-warning">[  FAIL  ]</span>:\tSomething wrong happened in the test, but it is expected and has a corresponding issue on github<br>')
    sys.stdout.write('<span class="line-error">[! FAIL !]</span>:\tThe test fails and it is not expected so<br>')
    sys.stdout.write('<span class="line-error">[!BROKEN!]</span>:\tThe test is not executed normally, usually due to disk out of space or bugs in the test script<br><br>')
    sys.stdout.write('<center>')

arch = ''
while True:
    l = sys.stdin.readline()
    if not l:
        break
    line = l.rstrip('\n')
    m = result_regex.match(line)
    if not m:
        m = arch_regex.match(line)
        if m:
            arch = m.group(1)
        sys.stdout.write(line + '<br>')
        continue
    testsuite = m.group(1)
    testcase = m.group(2)
    whitespaces = m.group(3)
    result = m.group(4)
    
    output = testsuite.replace('<', '&lt;').replace('>', '&gt;')
    test_result = os.path.join(result_dir, repo, commit, arch, testcase + '.error')
    if os.path.exists(test_result):
        output += '<a href="/repo/' + repo + '/' + commit + '/' + arch + '/' + testcase + '">' + testcase + '</a>'
    else:
        output += testcase
    output += whitespaces
    if result_formats.has_key(result):
        output += '<span class="' + result_formats[result] + '">' + result + '</span>'
    else:
        output += result
    sys.stdout.write(output + '<br>')

if section == 'AutoTest':
    sys.stdout.write('</center>')
sys.stdout.flush()
