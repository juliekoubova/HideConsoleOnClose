using System;
using System.Runtime.InteropServices;

namespace HideConsoleOnCloseManaged
{
    public static class HideConsoleOnClose
    {
        [DllImport("kernel32")]
        private static extern IntPtr GetConsoleWindow();

        [DllImport(
            "HideConsoleOnClose",
            EntryPoint = "EnableForWindow",
            SetLastError = true
        )]
        private static extern Boolean EnableForWindow32(IntPtr hWnd);

        [DllImport(
            "HideConsoleOnClose64",
            EntryPoint = "EnableForWindow",
            SetLastError = true
        )]
        private static extern Boolean EnableForWindow64(IntPtr hWnd);

        public static void EnableForWindow(IntPtr hWnd)
        {
            var success = false;
            if (IntPtr.Size == 4)
            {
                success = EnableForWindow32(hWnd);
            }
            else
            {
                success = EnableForWindow64(hWnd);
            }

            if (!success)
            {
                Marshal.ThrowExceptionForHR(
                    Marshal.GetHRForLastWin32Error()
                );
            }
        }

        public static void Enable()
        {
            var hWnd = GetConsoleWindow();

            if (hWnd == IntPtr.Zero)
            {
                throw new InvalidOperationException("The current process has no console window.");
            }

            EnableForWindow(hWnd);
        }
    }
}
