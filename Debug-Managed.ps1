$null = [Reflection.Assembly]::LoadFrom(
	(Resolve-Path .\HideConsoleOnCloseManaged.dll).Path
)

$Process = [Diagnostics.Process]::Start('cmd.exe')
$Process.EnableRaisingEvents = $true

$null = Register-ObjectEvent -InputObject $Process -EventName Exited -Action {
	Write-Host
	Write-Host 'cmd.exe has exited'
}

# wait for process to create the console window
Start-Sleep -Milliseconds 500

[HideConsoleOnCloseManaged.HideConsoleOnClose]::EnableForWindow($Process.MainWindowHandle)
