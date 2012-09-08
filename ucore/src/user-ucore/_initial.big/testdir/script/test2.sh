cd test
mkdir pp
cd pp
ls /bin | cat > qq
link qq pp
ls
cd ..
link pp/pp orz
unlink pp/pp pp/qq
rename pp qq
unlink qq
unlink orz
echo test2.sh end.

