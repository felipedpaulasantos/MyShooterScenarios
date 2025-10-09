@echo off
echo ==========================================
echo Unreal Engine Project Quick Cleanup
echo ==========================================
echo.
echo This will DELETE the following folders (they regenerate):
echo   - Binaries
echo   - Intermediate  
echo   - Saved
echo   - DerivedDataCache
echo   - .vs
echo.
echo These folders are SAFE to delete and will save 5-20 GB
echo They will be regenerated when you rebuild the project
echo.
pause
echo.

cd /d "F:\UnrealProjects\MyShooterScenarios"

if exist "Binaries" (
    echo Deleting Binaries...
    rmdir /s /q "Binaries"
    echo Done.
)

if exist "Intermediate" (
    echo Deleting Intermediate...
    rmdir /s /q "Intermediate"
    echo Done.
)

if exist "Saved" (
    echo Deleting Saved...
    rmdir /s /q "Saved"
    echo Done.
)

if exist "DerivedDataCache" (
    echo Deleting DerivedDataCache...
    rmdir /s /q "DerivedDataCache"
    echo Done.
)

if exist ".vs" (
    echo Deleting .vs...
    rmdir /s /q ".vs"
    echo Done.
)

echo.
echo ==========================================
echo Cleanup Complete!
echo ==========================================
echo.
echo Next steps:
echo 1. Right-click MY_SHOOTER.uproject
echo 2. Select "Generate Visual Studio project files"
echo 3. Open and rebuild the solution
echo.
pause

