import sys,os, re

p = re.compile('^(.+)\s+sys_(.+?)\((.+?)\);')
p1 = re.compile('(.+[\s*])([a-z0-9]+)')

f = open('syscall.h', 'r')
cnt = 0
while(True):
  line = f.readline()
  if not line: break
  cnt = cnt + 1
  m = p.match(line)
  if not m:
    print "ERR", line
    continue
  args = m.group(3).split(',')
  if (len(args)==1 and args[0]=='void') or len(args)==0:
    print '_syscall0('+m.group(1)+','+ m.group(2)+');'
  else:
    mm = p1.match(args[0])
    s = '_syscall'+str(len(args))+'('+m.group(1)+', '+m.group(2)
    for i in xrange(0,len(args)):
      mm = p1.match(args[i])
      s = s+','+mm.group(1)+', '+mm.group(2)
    s = s+');'
    print s

print "cnt", cnt


