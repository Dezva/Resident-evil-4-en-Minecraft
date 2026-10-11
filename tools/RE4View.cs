using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace RE4BridgeView {
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct Point { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct Placement {
        public int Length, Flags, Show; public Point Min, Max; public Rect Normal;
    }
    public sealed class Snapshot {
        public IntPtr Handle; public uint Pid; public long Style, Extended; public Placement Placement; public Rect Bounds;
    }
    public interface Desktop {
        bool SameWindow(IntPtr window, uint pid);
        IntPtr Foreground();
        Snapshot Capture(IntPtr window, uint pid);
        void Present(Snapshot state, Rect screen);
        void Restore(Snapshot state);
        bool Focus(IntPtr window);
        bool Stop(uint pid);
    }
    // The mode changes only one verified game window; it never sends desktop keys.
    public sealed class Session {
        readonly Desktop desktop;
        readonly Snapshot game;
        readonly IntPtr minecraft;
        readonly uint minecraftPid;
        public bool Active { get; private set; }
        public bool StopConfirmed { get; private set; }
        public bool FocusConfirmed { get; private set; }
        public Session(Desktop api, IntPtr re4, uint re4Pid, IntPtr mc, uint mcPid) {
            desktop = api; minecraft = mc; minecraftPid = mcPid;
            if (!api.SameWindow(re4,re4Pid) || !api.SameWindow(mc,mcPid))
                throw new InvalidOperationException("Las ventanas cambiaron. Reinicia VIEW-RE4.");
            game = api.Capture(re4,re4Pid);
        }
        public void Enter(Rect screen) {
            if (Active) return;
            if (!desktop.SameWindow(game.Handle,game.Pid) || !desktop.SameWindow(minecraft,minecraftPid))
                throw new InvalidOperationException("Se cerro o cambio una ventana de juego.");
            if (desktop.Foreground() != minecraft)
                throw new InvalidOperationException("Pulsa F9 desde Minecraft, despues de controller/arm.");
            Active = true; // Even a partial change must be restored on failure.
            try {
                desktop.Present(game,screen);
                if (desktop.Foreground() != minecraft) throw new InvalidOperationException("Minecraft perdio el foco.");
            } catch { Normal(false); throw; }
        }
        public void Normal(bool focusGame) {
            FocusConfirmed = false;
            // Latch the same stop word as native F8, independently of the TCP host.
            StopConfirmed = desktop.SameWindow(game.Handle,game.Pid) && desktop.Stop(game.Pid);
            if (Active) {
                if (desktop.SameWindow(game.Handle,game.Pid)) desktop.Restore(game);
                Active = false;
            }
            if (focusGame && desktop.SameWindow(game.Handle,game.Pid)) FocusConfirmed = desktop.Focus(game.Handle);
        }
        public void Prepare() {
            Normal(false);
            if (!desktop.SameWindow(minecraft,minecraftPid)) throw new InvalidOperationException("Minecraft se cerro.");
            if (!desktop.Focus(minecraft)) throw new InvalidOperationException("Windows no permitio dar foco. Pulsa Minecraft.");
        }
        public bool StillFocused() {
            return desktop.SameWindow(game.Handle,game.Pid) && desktop.SameWindow(minecraft,minecraftPid)
                && desktop.Foreground() == minecraft;
        }
    }
    public sealed class WindowsDesktop : Desktop {
        [DllImport("user32.dll")] static extern bool IsWindow(IntPtr h);
        [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
        [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll", SetLastError=true)] static extern bool SetForegroundWindow(IntPtr h);
        [DllImport("user32.dll", SetLastError=true)] static extern bool AttachThreadInput(uint from,uint to,bool attach);
        [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
        [DllImport("user32.dll", EntryPoint="GetWindowLongW")] static extern int GetLong32(IntPtr h,int index);
        [DllImport("user32.dll", EntryPoint="GetWindowLongPtrW")] static extern IntPtr GetLong64(IntPtr h,int index);
        [DllImport("user32.dll", EntryPoint="SetWindowLongW", SetLastError=true)] static extern int SetLong32(IntPtr h,int index,int value);
        [DllImport("user32.dll", EntryPoint="SetWindowLongPtrW", SetLastError=true)] static extern IntPtr SetLong64(IntPtr h,int index,IntPtr value);
        [DllImport("user32.dll", SetLastError=true)] static extern bool GetWindowPlacement(IntPtr h,ref Placement p);
        [DllImport("user32.dll", SetLastError=true)] static extern bool GetWindowRect(IntPtr h,out Rect rect);
        [DllImport("user32.dll", SetLastError=true)] static extern bool SetWindowPos(IntPtr h,IntPtr after,int x,int y,int w,int height,uint flags);
        [DllImport("user32.dll")] public static extern short GetAsyncKeyState(int key);
        [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern IntPtr OpenFileMappingW(uint access,bool inherit,string name);
        [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr MapViewOfFile(IntPtr h,uint access,uint high,uint low,UIntPtr size);
        [DllImport("kernel32.dll")] static extern bool UnmapViewOfFile(IntPtr h);
        [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
        [DllImport("kernel32.dll")] static extern void SetLastError(uint error);
        static long GetStyle(IntPtr h,int index) { return IntPtr.Size == 8 ? GetLong64(h,index).ToInt64() : (long)(uint)GetLong32(h,index); }
        static void SetStyle(IntPtr h,int index,long value) {
            SetLastError(0);
            long result = IntPtr.Size == 8 ? SetLong64(h,index,new IntPtr(value)).ToInt64() : SetLong32(h,index,unchecked((int)value));
            if (result == 0 && Marshal.GetLastWin32Error() != 0) throw new Win32Exception(Marshal.GetLastWin32Error());
        }
        static void Check(bool ok) { if (!ok) throw new Win32Exception(Marshal.GetLastWin32Error()); }
        public bool SameWindow(IntPtr h,uint pid) { uint owner; return IsWindow(h) && GetWindowThreadProcessId(h,out owner) != 0 && owner == pid; }
        public IntPtr Foreground() { return GetForegroundWindow(); }
        public Snapshot Capture(IntPtr h,uint pid) {
            Placement placement = new Placement(); placement.Length = Marshal.SizeOf(typeof(Placement));
            Check(GetWindowPlacement(h,ref placement));
            Rect rect; Check(GetWindowRect(h,out rect));
            return new Snapshot { Handle=h, Pid=pid, Style=GetStyle(h,-16), Extended=GetStyle(h,-20), Placement=placement, Bounds=rect };
        }
        public void Present(Snapshot state,Rect screen) {
            SetStyle(state.Handle,-16,state.Style & ~0x21C40000L); // Caption and resize frame.
            SetStyle(state.Handle,-20,state.Extended & ~0x00000300L); // Client/window edges.
            // TOPMOST + NOACTIVATE + FRAMECHANGED + SHOWWINDOW. Minecraft keeps input focus.
            Check(SetWindowPos(state.Handle,new IntPtr(-1),screen.Left,screen.Top,screen.Right-screen.Left,screen.Bottom-screen.Top,0x70));
        }
        public void Restore(Snapshot state) {
            SetStyle(state.Handle,-16,state.Style); SetStyle(state.Handle,-20,state.Extended);
            Rect rect = state.Bounds;
            IntPtr order = new IntPtr((state.Extended & 8) != 0 ? -1 : -2);
            Check(SetWindowPos(state.Handle,order,rect.Left,rect.Top,rect.Right-rect.Left,rect.Bottom-rect.Top,0x30)); // Restore bounds without activation.
        }
        public void RaiseControls(IntPtr h) { Check(SetWindowPos(h,new IntPtr(-1),0,0,0,0,0x13)); }
        public bool Focus(IntPtr h) {
            if (SetForegroundWindow(h)) return GetForegroundWindow() == h;
            // Temporarily join foreground input queues for this explicit user action only.
            uint ignored; uint foreground = GetWindowThreadProcessId(GetForegroundWindow(),out ignored);
            uint own = GetCurrentThreadId();
            bool attached = foreground != 0 && foreground != own && AttachThreadInput(own,foreground,true);
            try { return SetForegroundWindow(h) && GetForegroundWindow() == h; }
            finally { if (attached) AttachThreadInput(own,foreground,false); }
        }
        public static bool LatchStop(IntPtr view) {
            int version = Marshal.ReadInt32(view,4);
            if (Marshal.ReadInt32(view,0) != 0x52453443 || (version != 1 && version != 2) || Marshal.ReadInt32(view,28) != 1) return false;
            // Aligned single-word write, matching native F8. Never publish a control frame here.
            Marshal.WriteInt32(view,24,1);
            return true;
        }
        public bool Stop(uint pid) {
            IntPtr handle = OpenFileMappingW(6,false,"Local\\RE4Bridge-Control-"+pid);
            if (handle == IntPtr.Zero) return false;
            try {
                IntPtr view = MapViewOfFile(handle,6,0,0,new UIntPtr(40));
                if (view == IntPtr.Zero) return false;
                try {
                    return LatchStop(view);
                } finally { UnmapViewOfFile(view); }
            } finally { CloseHandle(handle); }
        }
    }
}
