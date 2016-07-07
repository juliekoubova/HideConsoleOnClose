using System;
using System.Runtime.InteropServices;

namespace HideConsoleOnCloseManaged
{
    public static class HideConsoleOnClose
    {
        [DllImport("kernel32")]
        private static extern IntPtr GetConsoleWindow();

        [DllImport("HideConsoleOnClose", EntryPoint = "EnableForWindow")]
        private static extern Boolean EnableForWindow32(IntPtr hWnd);

        [DllImport("HideConsoleOnClose64", EntryPoint = "EnableForWindow")]
        private static extern Boolean EnableForWindow64(IntPtr hWnd);

        public static Boolean EnableForWindow(IntPtr hWnd)
        {
            if (IntPtr.Size == 4)
            {
                return EnableForWindow32(hWnd);
            }

            return EnableForWindow64(hWnd);
        }

        public static Boolean Enable()
        {
            var hWnd = GetConsoleWindow();

            if (hWnd != IntPtr.Zero)
            {
                return EnableForWindow(hWnd);
            }

            return false;
        }
    }
}
