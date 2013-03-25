#!/bin/sh
find . -type f -name "*.[chS]" >all.cscope.files
cat all.cscope.files |awk -F'/' '$2 != "package" && $3 !="glue-kern" && $4 !="module"{print($0)}'|awk -F'/'  '$0 !~ /arch\/nios2\// && $0 !~ /arch\/arm\// && $0 !~ /arch\/mips\// && $0 !~ /arch\/i386\// && $0 !~ /or32\// && $0 !~ /arch\/um\// && $0 !~ /yaff/ && $0 !~ /fatfs/{print($0)}' > cscope.files
rm all.cscope.files
cscope -bq
ctags -L cscope.files
