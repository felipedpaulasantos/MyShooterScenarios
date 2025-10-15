@echo off
echo ================================================================
echo Fix Deprecated FTopLevelAssetPath Format - UE 5.5 Upgrade
echo ================================================================
echo.
echo This will resave all assets to update deprecated path formats.
echo This may take several minutes depending on project size.
echo.
pause

set UE_EDITOR="C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJECT_FILE="%~dp0MY_SHOOTER.uproject"

echo.
echo Starting asset resave process...
echo.

%UE_EDITOR% %PROJECT_FILE% -run=ResavePackages -autocheckout -progressonly -unattended -NoShaderCompile -SkipDeveloperFolders

echo.
echo ================================================================
echo Resave Complete!
echo ================================================================
echo.
echo The deprecated FTopLevelAssetPath warnings should now be fixed.
echo Restart the editor to verify.
echo.
pause

