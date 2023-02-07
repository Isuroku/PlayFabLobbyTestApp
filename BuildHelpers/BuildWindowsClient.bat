set BasePath=%~dp0

REM cd ..\Engine\Build\BatchFiles
cd d:\UnrealEngine427Source\Engine\Build\BatchFiles\

set PathToRunUAT=d:\UnrealEngine427Source\Engine\Build\BatchFiles\
set Configuration=Development

call %PathToRunUAT%RunUAT.bat BuildCookRun -project="e:\Work\UE4_27\PlayfabTest\PlayfabTest.uproject" -platform=Win64 -clientconfig=%Configuration% -build -cook -stage -crashreporter

cd %BASEPATH%