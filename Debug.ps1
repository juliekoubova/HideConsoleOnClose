$DllName = "HideConsoleOnClose"

if ([IntPtr]::Size -eq 8) {
	$DllName += '64'
}

$HideConsoleOnClose = Add-Type `
	-PassThru `
	-Name HideConsoleOnClose `
	-MemberDefinition @"

		[DllImport("$DllName")]
		public static extern Boolean EnableForWindow(IntPtr hwnd);
"@

$Process = [Diagnostics.Process]::Start('cmd.exe')

Start-Sleep -Milliseconds 500

$HideConsoleOnClose::EnableForWindow($Process.MainWindowHandle)
