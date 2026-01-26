@echo off
echo Transcompiling grammars...
taulc compile sigs Yama\grammars\sigs.taul Yama\internal\codegen\sigs-codegen.h -i=taul -t
echo Done!
pause