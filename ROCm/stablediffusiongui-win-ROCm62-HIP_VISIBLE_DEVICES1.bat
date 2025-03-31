@echo off

@REM set HIP_VISIBLE_DEVICES=0
set HIP_VISIBLE_DEVICES=1

set COMMANDLINE_ARGS=-hipblas

start stablediffusiongui.exe %COMMANDLINE_ARGS%

exit
