$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$pythonExe = (Get-Command pythonw.exe -ErrorAction Stop).Source
$desktopPath = [Environment]::GetFolderPath('Desktop')
$shortcutPath = Join-Path $desktopPath 'FNC Light.lnk'
$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $pythonExe
$shortcut.Arguments = '"' + (Join-Path $PSScriptRoot 'light_viewer.py') + '"'
$shortcut.WorkingDirectory = $projectRoot
$shortcut.IconLocation = (Join-Path $projectRoot 'assets\light-aligned.ico') + ',0'
$shortcut.Description = 'KinCony light controllers on the local network'
$shortcut.Save()
Write-Output $shortcutPath

$previousShortcut = Join-Path $desktopPath 'FNC Licht.lnk'
if (Test-Path -LiteralPath $previousShortcut) { Remove-Item -LiteralPath $previousShortcut }
