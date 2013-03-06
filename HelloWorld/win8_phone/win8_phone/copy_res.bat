@echo on
set SRC_DIR=..\Resources
set SRC_DIR_WIN=..\Resources_windows
set DST_DIR=%1
echo %DST_DIR%
xcopy /E /Y %SRC_DIR% %DST_DIR%
xcopy /E /Y %SRC_DIR_WIN% %DST_DIR%

