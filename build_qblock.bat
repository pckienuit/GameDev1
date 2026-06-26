@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\vsdevcmd\ext\vcvars.bat" x64
echo VCVARS DONE: errorlevel=%errorlevel%
where msbuild
where cl
echo ----
msbuild D:\GameDev1\Project1\Project1.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
exit /b %ERRORLEVEL%