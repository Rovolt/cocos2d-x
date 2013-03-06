@echo off
cd Assets


rem for /f %%f in ('dir /s/b Assets') do echo %%f
setlocal EnableDelayedExpansion
for /L %%n in (1 1 500) do if "!__cd__:~%%n,1!" neq "" set /a "len=%%n+1"
setlocal DisableDelayedExpansion
for /r . %%g in (*) do (
  set "absPath=%%g"
  setlocal EnableDelayedExpansion
  set "relPath=!absPath:~%len%!"
  echo(^<Image Include^=^"Assets^\!relPath!^" ^/^>
  endlocal
)
