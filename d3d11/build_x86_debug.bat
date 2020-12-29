CALL "c:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

cd "%~dp0"
msbuild.exe "%~dp0d3d11.sln" /property:Platform=x86 /property:Configuration=Debug
