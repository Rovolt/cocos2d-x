@echo on
set SRC_DIR=..\Resource
set DST_DIR=%1

xcopy /E /Y %SRC_DIR% %DST_DIR%
