[CmdletBinding()]
param (
	[Parameter()]
	[Int32]
	$ThreadId
)

$DllName = "HideConsoleOnClose$([IntPtr]::Size * 8)"
$HideConsoleOnClose = Add-Type `
	-PassThru `
	-Name HideConsoleOnClose `
	-MemberDefinition @"

		[DllImport("$DllName")]
		public static extern Boolean EnableForThread(Int32 threadId);

		[DllImport("kernel32")]
		public static extern IntPtr GetConsoleWindow();

		[DllImport("user32")]
		public static extern Int32 GetWindowThreadProcessId(
			IntPtr hWnd, 
			IntPtr lpdwProcessId
		);
"@

if (-not $ThreadId) {
	$ConsoleWindow = $HideConsoleOnClose::GetConsoleWindow()
	$ThreadId = $HideConsoleOnClose::GetWindowThreadProcessId(
		$ConsoleWindow,
		[IntPtr]::Zero
	)
}

$HideConsoleOnClose::EnableForThread($ThreadId)
