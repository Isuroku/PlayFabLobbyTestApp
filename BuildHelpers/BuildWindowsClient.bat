set BasePath=%~dp0

set PathToRunUAT=d:\UnrealEngine427Source\Engine\Build\BatchFiles\

set Configuration=Development

set ProjPath=%BASEPATH%..\PlayFabLobbyTestApp.uproject

call %PathToRunUAT%RunUAT.bat BuildCookRun -project="%ProjPath%" -platform=Win64 -clientconfig=%Configuration% -build -cook -stage -crashreporter