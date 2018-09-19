#pragma warning(disable:4244 4996)

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <rng.h>
#include <gdiplus.h>
#include <atlconv.h>

using namespace Gdiplus;

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

#include "define.h"

char data[1024]; // ���� ������

struct _NOTE {
	int timing; // ��Ʈ ���� Ÿ�̹�
	int num; // �巳 ����
	double pos; // ��Ʈ�� ��ġ
	boolean onscr = false; // ��Ʈ�� ȭ�鿡 �� �ִ���
}note[NOTE_MAX];

struct _LINE {
	boolean onscr = false; // ������ ȭ�鿡 �� �ִ���
	int time; // ���� ��ġ
}line[LINE_MAX];

struct _OPTION {
	struct __HOTKEY {
		char key[9]; // ����Ű
	}hotkey;
	double sp; // ���ǵ�(���)
}option;

struct _GAME {
	struct __NOTE {
		//int note_count = 0; // ���ݱ��� ������ ��Ʈ ��
		int start = 0; // �˻� ������ �ε���
		int cnt = 0; // ��Ʈ �ε���
					 //int hitgear[10] = { 0, }; // ��Ʈ�� ó���� ��� ��ȣ
	}note;
	struct __MUSIC {
		int id;
		wchar_t title[64] = { 0, }; // Ÿ��Ʋ
		wchar_t artist[64] = { 0, }; // ��Ƽ��Ʈ
		wchar_t comp[64] = { 0, }; // �۰
		wchar_t lyric[64] = { 0, }; // �ۻ簡
		int bpm; // BPM
		int length; // ����
		wchar_t df[4][8]; // [0~3], mas, ext, adv, bas
		int all_note_count[4] = { 0, }; // �� ��Ʈ�� [0~3], mas, ext, adv, bas
		int note_count[4][9] = { 0, }; // �巳�� ��Ʈ�� [0~3], mas, ext, adv, bas
	}music[100];
	struct __CHOICE {
		int ptr; // ���� ���õ� �뷡 �ε���
		int df_ptr; // ���� ���õ� ���̵� (0~3), mas, ext, adv, bas
		int music_count; // �뷡 �� ����
	}choice;
	struct __PLAY {
		int music_ptr; // �÷����ϴ� �뷡
		int df_ptr; // �÷����ϴ� ���̵� (0~3), mas, ext, adv, bas
		int perfect, great, good, ok, miss, mc, score; // ����, �׷�, ��, ����, �̽�, ����, ���ھ�
		double acc; // �޼���
		double skill; // ��ų
		int time; // �÷���Ÿ��(ƽ)
	}play;
	struct __LINE {
		int cnt = 0; // ���� �ε���
	}line;

	int screen = 0; // ����ȭ��/�÷�����/���ȭ��
					//int combo_timer = 0; // �޺� ǥ�� ������ Ÿ�̸�
	int beatcnt = 0; // ��Ʈī��Ʈ (���� ������)
}game;

struct _ME {
	int perfect = 0, great = 0, bad = 0, miss = 0; // ���� ��
	int score = 0; // ����
	int combo = 0; // �޺�
	int mc = 0; // �ƽ� �޺�
	int gauge = 0; // HP ������
	boolean press[9] = { false, }; // Ű�� ������ �ִ���
	int press_timer[9] = { 0, }; // ���� Ÿ�̸�
}me;


char str[300] = { 0, };
DWORD tick = GetTickCount();
TCHAR Tstr[300];
int half_mode = 0;

void CREATE_NOTE(int);
void CREATE_LINE();
int MOST_NEAR_NOTE(int);
int digit(int);
int digit_num(int, int);
int round(float);
int _mod(int, int);

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hb)
{
	HDC indc;
	HBITMAP ob;
	int bx, by;
	BITMAP bit;

	indc = CreateCompatibleDC(hdc);
	ob = (HBITMAP)SelectObject(indc, hb);

	GetObject(hb, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;

	BitBlt(hdc, x, y, bx, by, indc, 0, 0, SRCCOPY);

	SelectObject(indc, ob);
	DeleteDC(indc);
}

/* �̹��� ���� �׸��� */
void OnPaint(HDC hdc, WCHAR* filename, int x, int y, int w, int h)
{
	Image* I = Image::FromFile(filename);
	Graphics G(hdc);
	G.DrawImage(I, x, y, w, h);
	delete I;
}

#include "WinAPI.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	HBRUSH br, obr;
	HPEN p, op;
	FILE *fr, *fw;
	INT i;
	static INT x, y;
	const int INTERVAL_THRESHOLD = 60;


	switch (iMessage) {
	case WM_CREATE:
	{
		strcpy(str, "=");
	}
	break;
	case WM_TIMER:
	{

	}
	break;
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case 0x41:
			if (GetTickCount() - tick > INTERVAL_THRESHOLD) // different
				strcat(str, " ");
			if (wParam == 0x41 && GetKeyState(VK_CONTROL) < 0) // half (ctrl+a)
				strcat(str, "H(A)");
			else // normal
				strcat(str, "A");
			tick = GetTickCount();
			break;
		case 0x53:
			if (GetTickCount() - tick > INTERVAL_THRESHOLD) // different
				strcat(str, " ");
			if (wParam == 0x53 && GetKeyState(VK_CONTROL) < 0) // half
				strcat(str, "H(S)");
			else // normal
				strcat(str, "S");
			tick = GetTickCount();
			break;
		case 0x44:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "D");
			}
			else { // different
				strcat(str, " D");
			}
			tick = GetTickCount();
			break;
		case 0x46:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "F");
			}
			else { // different
				strcat(str, " F");
			}
			tick = GetTickCount();
			break;
		case 0x47:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "G");
			}
			else { // different
				strcat(str, " G");
			}
			tick = GetTickCount();
			break;
		case 0x48:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "H");
			}
			else { // different
				strcat(str, " H");
			}
			tick = GetTickCount();
			break;
		case 0x4A:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "J");
			}
			else { // different
				strcat(str, " J");
			}
			tick = GetTickCount();
			break;
		case 0x4B:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "K");
			}
			else { // different
				strcat(str, " K");
			}
			tick = GetTickCount();
			break;
		case 0x4C:
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "L");
			}
			else { // different
				strcat(str, " L");
			}
			tick = GetTickCount();
			break;
		case 0x20: // Space
			if (GetTickCount() - tick < INTERVAL_THRESHOLD) { // at once
				strcat(str, "X");
			}
			else { // different
				strcat(str, " X");
			}
			tick = GetTickCount();
			break;
		case 0x0D: // Enter

			break;
		case VK_ESCAPE: // ESC

			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		InvalidateRect(hWnd, NULL, TRUE);
	}
	break;
	case WM_LBUTTONDOWN:
	{
		x = LOWORD(lParam); // Ŭ���� x��ǥ
		y = HIWORD(lParam); // Ŭ���� y��ǥ

		InvalidateRect(hWnd, NULL, TRUE);
	}
	break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);

		sprintf(Tstr, "%s", str);
		TextOut(hdc, 10, 10, Tstr, strlen(Tstr));

		p = CreatePen(PS_SOLID, 4, RGB(100, 100, 100));
		op = (HPEN)SelectObject(hdc, p);
		br = (HBRUSH)GetStockObject(BLACK_BRUSH);
		obr = (HBRUSH)SelectObject(hdc, br);
		SetBkMode(hdc, TRANSPARENT);
		Rectangle(hdc, 10, 10, 510, 960);

		// thick pen
		__line(40, 10, 40, 960);
		__line(500, 10, 500, 960);
		__line(40, 40, 500, 40);
		__line(40, 840, 500, 840);


		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

int MOST_NEAR_NOTE(int n)
{
	int min = 1000;
	int min_index = 100;

	for (int i = 0; i < NOTE_MAX; i++) {
		if (note[i].onscr) {
			if (note[i].num == n) {
				if (note[i].timing < min) {
					min = note[i].timing;
					min_index = i;
				}
			}
		}
	}

	if (min_index == 100)
		return -1;
	else
		return min_index;
}

void CREATE_NOTE(int n)
{
	note[game.note.cnt].pos = GEARHEIGHT;
	note[game.note.cnt].num = n;
	note[game.note.cnt].onscr = true;
	game.note.cnt++;
}

void CREATE_LINE()
{
	line[game.line.cnt].time = GEARHEIGHT;
	line[game.line.cnt].onscr = true;
	game.line.cnt++;
}

int digit(int n)
{
	int i = 0;
	while (n >= (int)pow(10, ++i));
	return i;
}

int digit_num(int n, int d)
{
	return (n % (int)pow(10, d)) / (int)pow(10, d - 1);
}

int round(float n)
{
	return (int)n + (int)(n * 2) % 2;
}

int _mod(int n, int q)
{
	if (n >= 0)		return n % q;
	else			return n + q;
}