@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
echo VCVARS DONE: errorlevel=%errorlevel%
where msbuild
where cl
echo ----
msbuild D:\GameDev1\Project1\Project1.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
exit /b %ERRORLEVEL%