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

		[DllImport("kernel32")]
		public static extern IntPtr GetConsoleWindow();
"@

$ConsoleWindow = $HideConsoleOnClose::GetConsoleWindow()
$HideConsoleOnClose::EnableForWindow($ConsoleWindow)
