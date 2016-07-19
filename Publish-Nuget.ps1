[CmdletBinding()]
param (
	[Parameter(Mandatory)]
	$ApiKey
)

Get-ChildItem "$PSScriptRoot\Managed\bin\Release\HideConsoleOnClose.*.nupkg" `
| Sort-Object   -Property LastWriteTimeUtc -Descending `
| Select-Object -First 1 `
| ForEach-Object {
	& "$PSScriptRoot\nuget.exe" push `
		$_.FullName `
		-Source nuget.org `
		-ApiKey $ApiKey
}