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
        void PrepareView(Snapshot state,Rect screen);
        void Present(Snapshot state, Rect screen);
        void PrepareInput(Snapshot state,Rect screen);
        void Restore(Snapshot state);
        bool Focus(IntPtr window);
        bool Stop(uint pid);
        void StartMouse(Snapshot game,IntPtr minecraft,uint minecraftPid);
        void EndMouse();
        bool MouseFailed {get;}
    }
    // The mode changes only one verified game window; it never sends desktop keys.
    public sealed class Session {
        readonly Desktop desktop;
        readonly Snapshot game;readonly Snapshot inputWindow;
        readonly IntPtr minecraft;
        readonly uint minecraftPid;
        public bool Active { get; private set; }
        public bool Prepared {get;private set;}
        Rect preparedScreen;
        public bool StopConfirmed { get; private set; }
        public bool FocusConfirmed { get; private set; }
        public Session(Desktop api, IntPtr re4, uint re4Pid, IntPtr mc, uint mcPid) {
            desktop = api; minecraft = mc; minecraftPid = mcPid;
            if (!api.SameWindow(re4,re4Pid) || !api.SameWindow(mc,mcPid))
                throw new InvalidOperationException("Las ventanas cambiaron. Reinicia VIEW-RE4.");
            game = api.Capture(re4,re4Pid);inputWindow=api.Capture(mc,mcPid);
        }
        public void Enter(Rect screen) {
            if (Active) return;
            if(!Prepared || screen.Left!=preparedScreen.Left || screen.Top!=preparedScreen.Top || screen.Right!=preparedScreen.Right || screen.Bottom!=preparedScreen.Bottom)
                throw new InvalidOperationException("Pulsa Preparar Minecraft antes de arm y F9 para ajustar la vista.");
            if (!desktop.SameWindow(game.Handle,game.Pid) || !desktop.SameWindow(minecraft,minecraftPid))
                throw new InvalidOperationException("Se cerro o cambio una ventana de juego.");
            if (desktop.Foreground() != minecraft)
                throw new InvalidOperationException("Pulsa F9 desde Minecraft, despues de controller/arm.");
            Active = true; // Even a partial change must be restored on failure.
            try {
                desktop.Present(game,screen);
                desktop.StartMouse(game,minecraft,minecraftPid);
                if (desktop.Foreground() != minecraft) throw new InvalidOperationException("Minecraft perdio el foco.");
            } catch { Normal(false); throw; }
        }
        public void Normal(bool focusGame) {
            FocusConfirmed = false;
            // Latch the same stop word as native F8, independently of the TCP host.
            StopConfirmed = desktop.SameWindow(game.Handle,game.Pid) && desktop.Stop(game.Pid);
            try { desktop.EndMouse(); } finally {
            if (Active || Prepared) {
                try{if(desktop.SameWindow(inputWindow.Handle,inputWindow.Pid))desktop.Restore(inputWindow);}
                finally{if(desktop.SameWindow(game.Handle,game.Pid))desktop.Restore(game);}
                Active = false;Prepared=false;
            }
            }
            if (focusGame && desktop.SameWindow(game.Handle,game.Pid)) FocusConfirmed = desktop.Focus(game.Handle);
        }
        public void Prepare(Rect screen) {
            Normal(false);
            if (!desktop.SameWindow(minecraft,minecraftPid)) throw new InvalidOperationException("Minecraft se cerro.");
            Prepared=true;preparedScreen=screen;
            try {
                desktop.PrepareInput(inputWindow,screen);
                desktop.PrepareView(game,screen); // Resize only while the native stop is latched.
                if (!desktop.Focus(minecraft)) throw new InvalidOperationException("Windows no permitio dar foco. Pulsa Minecraft.");
            }catch{Normal(false);throw;}
        }
        public bool StillFocused() {
            return desktop.SameWindow(game.Handle,game.Pid) && desktop.SameWindow(minecraft,minecraftPid)
                && desktop.Foreground() == minecraft && !desktop.MouseFailed;
        }
    }
    public interface MouseDesktop {
        bool SameWindow(IntPtr h,uint pid);
        IntPtr Foreground();
        IntPtr At(Point point);
        bool Send(IntPtr h,uint message,uint buttons,Point point,ushort wheel);
        uint Modifiers();
        bool MenuOpen {get;}
    }
    // Only live user button/wheel events over the presented RE4 window are forwarded.
    // Raw mouse motion and keyboard input remain Minecraft's own events.
    public sealed class MouseRouter {
        readonly MouseDesktop desktop;readonly Snapshot game;readonly IntPtr minecraft;readonly uint minecraftPid;
        uint held;
        public bool Failed {get;private set;}
        public MouseRouter(MouseDesktop api,Snapshot state,IntPtr mc,uint pid){desktop=api;game=state;minecraft=mc;minecraftPid=pid;}
        public static bool ButtonOrWheel(uint message){
            return message==0x200 || message==0x201 || message==0x202 || message==0x204 || message==0x205
                || message==0x207 || message==0x208 || message==0x20a || message==0x20b || message==0x20c || message==0x20e;
        }
        public bool Route(int code,uint message,Point point,uint data,uint flags){
            if(!ButtonOrWheel(message) || code<0 || flags!=0 || Failed || !desktop.SameWindow(game.Handle,game.Pid)
                || !desktop.SameWindow(minecraft,minecraftPid) || desktop.Foreground()!=minecraft)return false;
            if(message==0x200 && !desktop.MenuOpen)return false;
            uint bit=0;bool down=false,up=false;ushort high=0;
            switch(message){
            case 0x200:break;
            case 0x201:bit=1;down=true;break;case 0x202:bit=1;up=true;break;
            case 0x204:bit=2;down=true;break;case 0x205:bit=2;up=true;break;
            case 0x207:bit=16;down=true;break;case 0x208:bit=16;up=true;break;
            case 0x20b:case 0x20c:
                high=(ushort)(data>>16);if(high!=1 && high!=2)return false;
                bit=high==1?32u:64u;down=message==0x20b;up=!down;break;
            case 0x20a:case 0x20e:high=(ushort)(data>>16);break;
            default:return false;
            }
            // A forwarded down must have a matching up even after Minecraft captures the mouse.
            if(!(up && (held&bit)!=0) && desktop.At(point)!=game.Handle)return false;
            uint next=down?held|bit:up?held&~bit:held;
            if(!desktop.Send(minecraft,message,next|desktop.Modifiers(),point,high)){Failed=true;return true;}
            held=next;return message!=0x200; // Cursor motion must continue through Windows; only clicks/wheel are consumed.
        }
        public void Release(Point point){
            if(desktop.SameWindow(minecraft,minecraftPid)){
                foreach(uint bit in new uint[]{1,2,16,32,64})if((held&bit)!=0){
                    held&=~bit;uint message=bit==1?0x202u:bit==2?0x205u:bit==16?0x208u:0x20cu;
                    desktop.Send(minecraft,message,held|desktop.Modifiers(),point,(ushort)(bit==32?1:bit==64?2:0));
                }
            }
            held=0;
        }
    }
    public sealed class WindowsDesktop : Desktop, MouseDesktop {
        [StructLayout(LayoutKind.Sequential)] struct MouseHookData { public Point Position;public uint Data,Flags,Time;public UIntPtr Extra; }
        delegate IntPtr MouseCallback(int code,IntPtr message,IntPtr data);
        [DllImport("user32.dll",SetLastError=true)] static extern IntPtr SetWindowsHookExW(int type,MouseCallback callback,IntPtr module,uint thread);
        [DllImport("user32.dll",SetLastError=true)] static extern bool UnhookWindowsHookEx(IntPtr hook);
        [DllImport("user32.dll")] static extern IntPtr CallNextHookEx(IntPtr hook,int code,IntPtr message,IntPtr data);
        [DllImport("kernel32.dll",CharSet=CharSet.Unicode)] static extern IntPtr GetModuleHandleW(string module);
        [DllImport("user32.dll")] static extern IntPtr WindowFromPoint(Point point);
        [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr h,uint flags);
        [DllImport("user32.dll")] static extern bool GetCursorPos(out Point point);
        [DllImport("user32.dll")] static extern bool ScreenToClient(IntPtr h,ref Point point);
        [DllImport("user32.dll",SetLastError=true)] static extern bool PostMessageW(IntPtr h,uint message,UIntPtr wparam,IntPtr lparam);
        MouseCallback mouseCallback;IntPtr mouseHook;MouseRouter mouseRouter;uint mouseGamePid;
        IntPtr menuMapping,menuView;
        public bool MenuOpen {get {
            if(menuView==IntPtr.Zero)return false;
            int before=Marshal.ReadInt32(menuView,8);if((before&1)!=0)return false;
            bool menu=Marshal.ReadInt32(menuView,16)==1 && Marshal.ReadInt32(menuView,32)==1;
            return menu && unchecked(GetTickCount()-(uint)Marshal.ReadInt32(menuView,12))<500 && before==Marshal.ReadInt32(menuView,8);
        }}
        public bool MouseFailed {get{return mouseRouter!=null && mouseRouter.Failed;}}
        public IntPtr At(Point point){return GetAncestor(WindowFromPoint(point),2);}
        public uint Modifiers(){return (GetAsyncKeyState(0x10)&0x8000)!=0?4u|((GetAsyncKeyState(0x11)&0x8000)!=0?8u:0u):((GetAsyncKeyState(0x11)&0x8000)!=0?8u:0u);}
        public bool Send(IntPtr h,uint message,uint buttons,Point point,ushort wheel){
            // Wheel messages use screen coordinates; button messages use the Minecraft client area.
            if(message!=0x20a && message!=0x20e && !ScreenToClient(h,ref point))return false;
            uint coordinates=(uint)((point.X&0xffff)|((point.Y&0xffff)<<16));
            uint wparam=buttons|((uint)wheel<<16);
            return PostMessageW(h,message,new UIntPtr(wparam),new IntPtr(unchecked((int)coordinates)));
        }
        public void StartMouse(Snapshot game,IntPtr mc,uint mcPid){
            EndMouse();
            menuMapping=OpenFileMappingW(4,false,"Local\\RE4Bridge-OverlayCommand-"+game.Pid);
            if(menuMapping!=IntPtr.Zero){menuView=MapViewOfFile(menuMapping,4,0,0,new UIntPtr(128));
                if(menuView==IntPtr.Zero || Marshal.ReadInt32(menuView)!=0x5245344f || Marshal.ReadInt32(menuView,4)!=1){if(menuView!=IntPtr.Zero)UnmapViewOfFile(menuView);menuView=IntPtr.Zero;CloseHandle(menuMapping);menuMapping=IntPtr.Zero;}}
            mouseRouter=new MouseRouter(this,game,mc,mcPid);mouseGamePid=game.Pid;
            mouseCallback=OnMouse;mouseHook=SetWindowsHookExW(14,mouseCallback,GetModuleHandleW(null),0);
            if(mouseHook==IntPtr.Zero){int error=Marshal.GetLastWin32Error();EndMouse();throw new Win32Exception(error,"No se pudo preparar el raton de VIEW.");}
        }
        IntPtr OnMouse(int code,IntPtr message,IntPtr data){
            try {
                if(code>=0 && mouseRouter!=null && MouseRouter.ButtonOrWheel((uint)message.ToInt64())){
                    MouseHookData input=(MouseHookData)Marshal.PtrToStructure(data,typeof(MouseHookData));
                    if(mouseRouter.Route(code,(uint)message.ToInt64(),input.Position,input.Data,input.Flags)){
                        if(mouseRouter.Failed)Stop(mouseGamePid);
                        return new IntPtr(1);
                    }
                }
            } catch {if(mouseRouter!=null)Stop(mouseGamePid);}
            return CallNextHookEx(mouseHook,code,message,data);
        }
        public void EndMouse(){
            MouseRouter old=mouseRouter;mouseRouter=null; // Fail closed even if unhooking fails.
            if(old!=null){Point point;GetCursorPos(out point);old.Release(point);}
            if(mouseHook!=IntPtr.Zero){
                bool removed=UnhookWindowsHookEx(mouseHook);int error=Marshal.GetLastWin32Error();
                if(!removed && error!=1404)throw new Win32Exception(error);
                mouseHook=IntPtr.Zero;
            }
            mouseCallback=null;mouseGamePid=0;
            if(menuView!=IntPtr.Zero){UnmapViewOfFile(menuView);menuView=IntPtr.Zero;}
            if(menuMapping!=IntPtr.Zero){CloseHandle(menuMapping);menuMapping=IntPtr.Zero;}
        }
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
        [DllImport("kernel32.dll")] static extern uint GetTickCount();
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
        public void PrepareView(Snapshot state,Rect screen) {
            SetStyle(state.Handle,-16,state.Style & ~0x21C40000L); // Caption and resize frame.
            SetStyle(state.Handle,-20,(state.Extended & ~0x00000300L)|0x08000000L); // Client/window edges.
            // Resize in the background before arm; never reset graphics while entering the active view.
            Check(SetWindowPos(state.Handle,new IntPtr(-2),screen.Left,screen.Top,screen.Right-screen.Left,screen.Bottom-screen.Top,0x70));
        }
        public void PrepareInput(Snapshot state,Rect screen){
            SetStyle(state.Handle,-16,state.Style&~0x21C40000L);
            SetStyle(state.Handle,-20,state.Extended&~0x00000300L);
            Check(SetWindowPos(state.Handle,new IntPtr(-2),screen.Left,screen.Top,screen.Right-screen.Left,screen.Bottom-screen.Top,0x70));
        }
        public void Present(Snapshot state,Rect screen) {
            // Only raise the already sized window: NOACTIVATE + NOSIZE + NOMOVE + SHOWWINDOW.
            Check(SetWindowPos(state.Handle,new IntPtr(-1),0,0,0,0,0x53));
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
