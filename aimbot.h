#include <stdio.h>
#include <Windows.h>
#include <WinGDI.h>

#pragma comment(lib, "winmm.lib") //timeGetTime

//COLORREF targetColors[] = { 0x1e3d3c, 0x7beeff, 0x3c625b, 0x5bc1c0,  0x609c7b, 0x2f4c3f, 0x215a92 }; //картошка
COLORREF targetColors[] = { 0x1a3248, 0x47d6f8, 0x13aef6, 0x3096f0, 0x3dffff, 0x2999be, 0x2394FF, 0x006FEA, 0x54faf4, 0x23f8f8, 0x19e5fa, 0x1295e0, 0x3ca4a4 };
bool aim;
bool fov;
bool isAiming;
// settings
DWORD Daimkey = VK_LBUTTON;
int tolerance = 30;
int aimsens = 5;
int aimfov = 50;

bool IsPressed = false;

BYTE colorDeviation = 0x0;
BYTE* bitData = NULL;

BOOL ScanPixel(HWND hwnd, PLONG pixelX, PLONG pixelY, RECT scanArea, COLORREF *targetColors, BYTE deviation, COLORREF *foundColor)
{
	deviation = tolerance;

	HDC hdc = GetWindowDC(hwnd);

	LONG scanWidth = scanArea.right - scanArea.left;
	LONG scanHeight = scanArea.bottom - scanArea.top;

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biWidth = scanWidth;
	bmi.bmiHeader.biHeight = -scanHeight;
	bmi.bmiHeader.biCompression = BI_RGB;

	BYTE *bitData = new BYTE[3 * scanWidth * scanHeight];

	GetDIBits(hdc, 0, scanArea.left, scanArea.top, bitData, &bmi, DIB_RGB_COLORS);

	// Release the DC
	ReleaseDC(hwnd, hdc);

	LONG aimFovLeft = scanWidth / 2 - (aimfov * 2);
	LONG aimFovRight = scanWidth / 2 + (aimfov * 2);

	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		for (int y = scanHeight / 2; y >= 0; y--)
		{
			for (int x = aimFovLeft; x <= aimFovRight; x += 2)
			{
				BYTE r = bitData[3 * ((y * scanWidth) + x) + 2];
				BYTE g = bitData[3 * ((y * scanWidth) + x) + 1];
				BYTE b = bitData[3 * ((y * scanWidth) + x) + 0];

				for (int i = 0; i < 3; i++) //3 if targetColors[3]
				{
					UINT targetR = GetRValue(targetColors[i]);
					UINT targetG = GetGValue(targetColors[i]);
					UINT targetB = GetBValue(targetColors[i]);

					if (r <= (targetR + tolerance) && r >= (targetR - tolerance) && g <= (targetG + tolerance) && g >= (targetG - tolerance) && b <= (targetB + tolerance) && b >= (targetB - tolerance))
					{
						*pixelX = scanArea.left + x;
						*pixelY = scanArea.top + y;
						*foundColor = targetColors[i];
						delete[] bitData;
						return TRUE;
					}
				}
			}
		}
	}
	else
	{
		for (int y = scanHeight; y >= 0; y--)
		{
			for (int x = aimFovLeft; x <= aimFovRight; x += 2)
			{
				BYTE r = bitData[3 * ((y * scanWidth) + x) + 2];
				BYTE g = bitData[3 * ((y * scanWidth) + x) + 1];
				BYTE b = bitData[3 * ((y * scanWidth) + x) + 0];

				for (int i = 0; i < 3; i++) //3 if targetColors[3]
				{
					UINT targetR = GetRValue(targetColors[i]);
					UINT targetG = GetGValue(targetColors[i]);
					UINT targetB = GetBValue(targetColors[i]);

					if (r <= (targetR + tolerance) && r >= (targetR - tolerance) && g <= (targetG + tolerance) && g >= (targetG - tolerance) && b <= (targetB + tolerance) && b >= (targetB - tolerance))
					{
						*pixelX = scanArea.left + x;
						*pixelY = scanArea.top + y;
						*foundColor = targetColors[i];
						delete[] bitData;
						return TRUE;
					}
				}
			}
		}
	}

	delete[] bitData;
	return FALSE;
}



COLORREF foundColor;
void Aiming(HWND hwnd, LONG pixelX, LONG pixelY)
{
	LONG aimX = pixelX - (GetSystemMetrics(SM_CXSCREEN) / 2);
	LONG aimY = pixelY - (GetSystemMetrics(SM_CYSCREEN) / 2);

	aimX /= aimsens;
	aimY /= aimsens;

	mouse_event(MOUSEEVENTF_MOVE, aimX, aimY, 0, 0);
}
void DrawAimFov(HWND hwnd, LONG centerX, LONG centerY, int aimfov)
{
	HDC hdc = GetWindowDC(hwnd);
	HPEN hPen;

	if (isAiming) {
		hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0)); // Red color when aiming
	}
	else {
		hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); // White color when not aiming
	}

	SelectObject(hdc, hPen);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));

	// Disable window background erasing to prevent flickering
	SetBkMode(hdc, TRANSPARENT);

	// Draw the aim FOV
	Ellipse(hdc, centerX - aimfov, centerY - aimfov, centerX + aimfov, centerY + aimfov);

	DeleteObject(hPen);
	ReleaseDC(hwnd, hdc);
}
HWND hwnd = NULL;
void fovdraw()
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	while (!hwnd)
	{
		hwnd = FindWindowA(0, "The Legend");
	}

	//try to get screen size of hwnd
	RECT rc{ 0 };
	GetClientRect(hwnd, &rc);
	LONG width = rc.right;
	LONG height = rc.bottom;

	//wait while the game window is not fully initialised
	while (rc.right == 0 || rc.bottom == 0)
	{
		Sleep(1000);
		printf(".");
		GetClientRect(hwnd, &rc);
		if (rc.right != 0 || rc.bottom != 0)
			break;
	}

	//get correct screen size of hwnd
	GetClientRect(hwnd, &rc);
	width = rc.right;
	height = rc.bottom;
	printf("\n");
	printf("bluestacks (%dx%d)\n", width, height);

	//calculate scan area
	LONG centerX = width / 2;
	LONG centerY = height / 2;
	LONG fovX = centerX / aimfov;
	LONG fovY = centerY / aimfov;
	RECT scanArea = { centerX - fovX, centerY - fovY, centerX + fovX, centerY + fovX };
	//calculate scan area size
	LONG scanWidth = scanArea.right - scanArea.left;
	LONG scanHeight = scanArea.bottom - scanArea.top;
	printf("fov (%d,%d,%d,%d)\nfov size: %d, %d\n", scanArea.left, scanArea.top, scanArea.right, scanArea.bottom, scanWidth, scanHeight);

	bitData = new BYTE[3 * scanWidth * scanHeight];
	LONG pixelX = 0;
	LONG pixelY = 0;
	while (fov)
	{
		DrawAimFov(hwnd, centerX, centerY, aimfov);
	}
}

void aimbot()
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	while (!hwnd)
	{
		//hwnd = FindWindowA(0, "BlueStacks");
		hwnd = FindWindowA(0, "The Legend");
	}

	//try to get screen size of hwnd
	RECT rc{ 0 };
	GetClientRect(hwnd, &rc);
	LONG width = rc.right;
	LONG height = rc.bottom;

	//wait while the game window is not fully initialised
	while (rc.right == 0 || rc.bottom == 0)
	{
		Sleep(1000);
		printf(".");
		GetClientRect(hwnd, &rc);
		if (rc.right != 0 || rc.bottom != 0)
			break;
	}

	//get correct screen size of hwnd
	GetClientRect(hwnd, &rc);
	width = rc.right;
	height = rc.bottom;
	printf("\n");
	printf("bluestacks (%dx%d)\n", width, height);

	//calculate scan area
	LONG centerX = width / 2;
	LONG centerY = height / 2;
	LONG fovX = centerX / aimfov;
	LONG fovY = centerY / aimfov;
	RECT scanArea = { centerX - fovX, centerY - fovY, centerX + fovX, centerY + fovX };
	//calculate scan area size
	LONG scanWidth = scanArea.right - scanArea.left;
	LONG scanHeight = scanArea.bottom - scanArea.top;
	printf("fov (%d,%d,%d,%d)\nfov size: %d, %d\n", scanArea.left, scanArea.top, scanArea.right, scanArea.bottom, scanWidth, scanHeight);

	bitData = new BYTE[3 * scanWidth * scanHeight];
	LONG pixelX = 0;
	LONG pixelY = 0;
	COLORREF foundColor = 0;

	while (aim)
	{
		if (GetAsyncKeyState(Daimkey) & 0x8000)
		{
			if (GetForegroundWindow() == hwnd && ScanPixel(hwnd, &pixelX, &pixelY, scanArea, targetColors, colorDeviation, &foundColor))
			{

				LONG aimX = (pixelX - centerX) / (aimsens * 2);
				LONG aimY = pixelY - centerY;
				
			isAiming = true;
				aimX /= aimsens;
				aimY /= aimsens;

				if (foundColor == targetColors[0] || foundColor == targetColors[1] || foundColor == targetColors[2])
					mouse_event(MOUSEEVENTF_MOVE, aimX - 1, aimY, 0, 0);
			}
		}
		delete[] bitData;
		Sleep(1);
		isAiming = false;
	}
	//return 0;
}
bool recoil;
int speed = 100;

int startTime = GetTickCount();
void recoilcontrol()
{
	while (recoil)
	{
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			INPUT input;
			input.type = INPUT_MOUSE;
			input.mi.dx = 0;
			input.mi.dy = speed / 100;
			input.mi.mouseData = 0;
			input.mi.dwFlags = MOUSEEVENTF_MOVE;
			input.mi.time = 0;
			input.mi.dwExtraInfo = 0;

			SendInput(1, &input, sizeof(INPUT));

			Sleep(10);
		}
	}
}
