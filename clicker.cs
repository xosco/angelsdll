using System;
using System.Runtime.InteropServices;
using System.Threading;

class Program
{
    [DllImport("user32.dll")]
    static extern short GetAsyncKeyState(int vKey);

    [DllImport("user32.dll")]
    static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);

    [DllImport("kernel32.dll")]
    static extern bool QueryPerformanceCounter(out long lpPerformanceCount);

    [DllImport("kernel32.dll")]
    static extern bool QueryPerformanceFrequency(out long lpFrequency);

    // Явное выравнивание структуры для 64-битных систем
    [StructLayout(LayoutKind.Explicit, Size = 40)]
    struct INPUT
    {
        [FieldOffset(0)] public uint type;
        [FieldOffset(8)] public KEYBDINPUT ki;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct KEYBDINPUT
    {
        public ushort wVk;
        public ushort wScan;
        public uint dwFlags;
        public uint time;
        public IntPtr dwExtraInfo;
    }

    const uint INPUT_KEYBOARD = 1;
    const uint KEYEVENTF_KEYUP = 0x0002;
    const uint KEYEVENTF_SCANCODE = 0x0008; // Флаг аппаратного нажатия (обязательно для ИГР)
    const int  VK_F6 = 0x75;

    // Аппаратные скан-коды (DirectInput) вместо виртуальных
    const ushort DIK_F = 0x21; // Скан-код клавиши F
    const ushort DIK_G = 0x22; // Скан-код клавиши G

    static volatile bool g_running     = true;
    static volatile bool g_active      = false;
    static volatile bool g_configuring = true;
    static volatile int  g_cps         = 50; 
    static volatile int  g_toggleKey   = 0;

    static INPUT[] g_inputsDown = new INPUT[2];
    static INPUT[] g_inputsUp   = new INPUT[2];
    static int     g_inputSize = 0;

    static void Main(string[] args)
    {
        Console.Title = "Roblox Fixed X64 Clicker [F + G]";

        g_inputSize = Marshal.SizeOf(typeof(INPUT));
        
        // КЛАВИША F (Нажатие)
        g_inputsDown[0].type = INPUT_KEYBOARD;
        g_inputsDown[0].ki.wScan = DIK_F; // Используем скан-код
        g_inputsDown[0].ki.dwFlags = KEYEVENTF_SCANCODE; 

        // КЛАВИША F (Отпускание)
        g_inputsUp[0].type = INPUT_KEYBOARD;
        g_inputsUp[0].ki.wScan = DIK_F;
        g_inputsUp[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

        // КЛАВИША G (Нажатие)
        g_inputsDown[1].type = INPUT_KEYBOARD;
        g_inputsDown[1].ki.wScan = DIK_G;
        g_inputsDown[1].ki.dwFlags = KEYEVENTF_SCANCODE;

        // КЛАВИША G (Отпускание)
        g_inputsUp[1].type = INPUT_KEYBOARD;
        g_inputsUp[1].ki.wScan = DIK_G;
        g_inputsUp[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

        Thread clickThread = new Thread(ClickerThread);
        clickThread.Priority     = ThreadPriority.Highest;
        clickThread.IsBackground = true;
        clickThread.Start();

        bool keyWasPressed = false;

        while (g_running)
        {
            if (g_configuring)
            {
                ConfigureSettings();
                // ИСПРАВЛЕНО: Считываем текущее состояние клавиши, чтобы избежать моментального двойного клика
                keyWasPressed = (GetAsyncKeyState(g_toggleKey) & 0x8000) != 0;
            }

            if ((GetAsyncKeyState(VK_F6) & 0x8000) != 0)
            {
                g_active      = false;
                g_configuring = true;
                Thread.Sleep(500);
                continue;
            }

            if (g_toggleKey != 0)
            {
                bool keyIsPressed = (GetAsyncKeyState(g_toggleKey) & 0x8000) != 0;

                if (keyIsPressed && !keyWasPressed)
                {
                    g_active = !g_active;
                    Console.ForegroundColor = g_active ? ConsoleColor.Green : ConsoleColor.Red;
                    Console.WriteLine(g_active ? "[СТАТУС: РАБОТАЕТ]" : "[СТАТУС: ПАУЗА]");
                    Console.ResetColor();
                }

                keyWasPressed = keyIsPressed;
            }

            Thread.Sleep(10);
        }
    }

    static void ConfigureSettings()
    {
        g_active = false;
        Console.Clear();
        Console.WriteLine("=== НАСТРОЙКИ КЛИКЕРА (X64 СТРУКТУРА) ===");
        Console.WriteLine("Будут кликать: F и G");
        Console.WriteLine("\n1. Нажмите ЛЮБУЮ клавишу для ВКЛ/ВЫКЛ...");

        Thread.Sleep(500);

        bool found = false;
        while (!found)
        {
            for (int i = 8; i < 190; i++)
            {
                if (i == VK_F6) continue;
                if ((GetAsyncKeyState(i) & 0x8000) != 0)
                {
                    g_toggleKey = i;
                    Console.WriteLine("Клавиша выбрана! (Код: " + i + ")");
                    found = true;
                    break;
                }
            }
            Thread.Sleep(10);
        }

        Thread.Sleep(500);

        Console.Write("\n2. Введите CPS (Рекомендуется 20-50 для игр): ");
        string input = Console.ReadLine();

        int rawCps;
        if (!int.TryParse(input, out rawCps) || rawCps <= 0)
            rawCps = 50; // Ставим безопасный дефолт

        g_cps = rawCps;

        Console.Clear();
        Console.WriteLine("=== КЛИКЕР ГОТОВ ===");
        Console.WriteLine("Активация      : [Код " + g_toggleKey + "]");
        Console.WriteLine("Кнопки спама   : F + G");
        Console.WriteLine("Скорость (CPS) : ~" + g_cps);
        Console.WriteLine("-------------------------------------------");
        Console.WriteLine("1. Запустите игру от ИМЕНИ АДМИНИСТРАТОРА (важно!).");
        Console.WriteLine("2. Нажмите кнопку активации для старта.");
        Console.WriteLine("-------------------------------------------");

        g_configuring = false;
    }

    static void ClickerThread()
    {
        long frequency;
        QueryPerformanceFrequency(out frequency);

        while (g_running)
        {
            if (g_active && !g_configuring)
            {
                double totalCycleTime = 1.0 / g_cps;
                double holdTime = totalCycleTime / 2.0; 

                long t1, t2;

                // Нажатие
                QueryPerformanceCounter(out t1);
                SendInput((uint)g_inputsDown.Length, g_inputsDown, g_inputSize);
                
                do { QueryPerformanceCounter(out t2); } 
                while ((double)(t2 - t1) / frequency < holdTime);

                // Отпускание
                QueryPerformanceCounter(out t1);
                SendInput((uint)g_inputsUp.Length, g_inputsUp, g_inputSize);

                do { QueryPerformanceCounter(out t2); } 
                while ((double)(t2 - t1) / frequency < (totalCycleTime - holdTime));
            }
            else
            {
                Thread.Sleep(10);
            }
        }
    }
}