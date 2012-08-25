cat script/test2.sh > test/tmp.sh
ls test
sh < test/tmp.sh
unlink test/tmp.sh
echo test3.sh end.

