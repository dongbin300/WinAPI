#pragma once

#define CAL_ACC (((double)(me.perfect*10+me.great*9+me.bad*5+me.miss*0) / (double)(me.perfect*10+me.great*10+me.bad*10+me.miss*10)) * 100.0)
#define NOTE_CREATE_TICK ((int)(30000.0 / (float)(option.bpm * game.realframe)))

#pragma region GAME SYSTEM DEFINE
#define GAME_RESOLUTION_WIDTH 600
#define GAME_RESOLUTION_HEIGHT 900
#define TASKBAR_HEIGHT 40
#define NOTE_MAX 2000
#define LINE_MAX 10
#define SAMETIME_MAX 4
#define scoreY 100
#define judgeY 300
#define NOTE_EFFECT_MAX 10
#pragma endregion

#pragma region GAMEPLAY DEFINE
#define GP_CHOICE 0x0000
#define GP_PLAY 0x0001
#define GP_RESULT 0x0002
#pragma endregion

#pragma region PLAY SCREEN XY
#define GEARHEIGHT 567
#define FALL_ZONE 562
#define GEAR_DIST0 295
#define GEAR_LEFT 3
#define GEAR_XC 71
#define GEAR_XH 50
#define GEAR_XL 52
#define GEAR_XS 57
#define GEAR_XT 54
#define GEAR_XK 63
#define GEAR_XM 49
#define GEAR_XF 49
#define GEAR_XR 72
#define GEAR_RIGHT 4
#define GEAR_YC 44
#define GEAR_YH 34
#define GEAR_YL 49
#define GEAR_YS 11
#define GEAR_YT 10
#define GEAR_YK 59
#define GEAR_YM 10
#define GEAR_YF 40
#define GEAR_YR 51
#define JUDGELINE_UP 5
#define JUDGELINE_DOWN 5
#define DRUM_DISPLAY 59
#define GAUGE_DISPLAY 57
#define INGAME_JACKET_SIZE 64
#define INGAME_MUSICNAME_WIDTH 219
#define INGAME_MUSICNAME_HEIGHT 64
#pragma endregion

#define __rect(x1, y1, x2, y2) Rectangle(hdc, x1, y1, x2, y2)
#define __line(x1, y1, x2, y2) MoveToEx(hdc, x1, y1, NULL); LineTo(hdc, x2, y2)
#define __draw(src, x, y, w, h) OnPaint(hdc, src, x, y, w, h)
#define __font(f) SelectObject(hdc, f)
#define __pen(r, g, b, w) DeleteObject(SelectObject(hdc, open)); pen = CreatePen(PS_SOLID, w, RGB(r, g, b)); open = (HPEN)SelectObject(hdc, pen)
#define __brush(r, g, b) DeleteObject(SelectObject(hdc, obrush)); brush = CreateSolidBrush(RGB(r, g, b)); obrush = (HBRUSH)SelectObject(hdc, brush)
#define __nullbrush DeleteObject(SelectObject(hdc, obrush)); brush = (HBRUSH)GetStockObject(NULL_BRUSH); obrush = (HBRUSH)SelectObject(hdc, brush)
#define __text(text, x, y) TextOut(hdc, x, y, text, strlen(text))
#define __textW(text, x, y) TextOutW(hdc, x, y, text, wcslen(text))
#define __textcolor(r, g, b) SetTextColor(hdc, RGB(r, g, b))
