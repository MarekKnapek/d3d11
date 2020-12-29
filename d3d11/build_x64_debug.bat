CALL "c:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsx86_amd64.bat"

cd "%~dp0"
msbuild.exe "%~dp0d3d11.sln" /property:Platform=x64 /property:Configuration=Debug
