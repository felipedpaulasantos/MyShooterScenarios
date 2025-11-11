Param(
  [int]$TargetMax = 4096,
  [switch]$ReimportLarge,
  [string]$UnrealEditorExe = "$Env:ProgramFiles\Epic Games\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
  [string]$ProjectPath = (Resolve-Path "..\MY_SHOOTER.uproject").Path
)

if (!(Test-Path $UnrealEditorExe)) {
  Write-Host "UnrealEditor executable não encontrado em: $UnrealEditorExe" -ForegroundColor Red
  Write-Host "Ajuste o parâmetro -UnrealEditorExe para sua instalação (ex: C:\Program Files\Epic Games\UE_5.5)" -ForegroundColor Yellow
  exit 1
}

$env:TARGET_MAX_TEXTURE_SIZE = $TargetMax
$env:REIMPORT_LARGE = if ($ReimportLarge) { 'true' } else { 'false' }

$scriptPath = (Resolve-Path "ResizeLargeTextures.py").Path
Write-Host "Executando redimensionamento de texturas > $TargetMax (Reimport: $ReimportLarge)" -ForegroundColor Cyan

# Rodar editor em modo sem UI com script Python
& $UnrealEditorExe $ProjectPath -run=pythonscript -script="$scriptPath" -nop4 -nosplash -unattended -UTF8Output

if ($LASTEXITCODE -ne 0) {
  Write-Host "Processo terminou com código $LASTEXITCODE" -ForegroundColor Red
  exit $LASTEXITCODE
}

Write-Host "Concluído. Verifique o Output Log do Unreal para relatório (ResizeLargeTextures)." -ForegroundColor Green

