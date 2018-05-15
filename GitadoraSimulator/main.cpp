/*
업데이트 할 것

실시간 싱크및박자 자동조절(추가할수도있고 안할수도있음)
롱노트 및 키보드 인식방법 변경

*/

#pragma warning(disable:4244 4996)

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <rng.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <atlconv.h>
#include <locale.h>

using namespace Gdiplus;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

#include "define.h"

INT GearX[11] = {
	GEAR_LEFT,
	GEAR_LEFT + GEAR_XC,
	GEAR_LEFT + GEAR_XC + GEAR_XH,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS + GEAR_XT,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS + GEAR_XT + GEAR_XK,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS + GEAR_XT + GEAR_XK + GEAR_XM,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS + GEAR_XT + GEAR_XK + GEAR_XM + GEAR_XF,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS + GEAR_XT + GEAR_XK + GEAR_XM + GEAR_XF + GEAR_XR,
	GEAR_LEFT + GEAR_XC + GEAR_XH + GEAR_XL + GEAR_XS + GEAR_XT + GEAR_XK + GEAR_XM + GEAR_XF + GEAR_XR + GEAR_RIGHT
};
COLORREF df_color[4] = { 0x00FF70E1, 0x00A15EFF, 0x002BC5F0, 0x00FFBE68 }; // mas, ext, adv, bas
char Tdf_dis[4][10] = { "MASTER", "EXTREME", "ADVANCED", "BASIC" };
int scorex[6] = { 0, };
int obt_score = 0; // 노트 눌렀을때 오르는 점수
float one_beat = 0.0; // 원 비트
int one_beat_count = 0; // 원 비트 카운트
INT Ptext = 0, Gtext = 0, Btext = 0, Mtext = 0; // 판정 표시 텍스트
char data[1024]; // 파일 데이터
int full_screen = 0; // 풀스크린 체크
HBITMAP hb, ob;

struct _NOTE {
	boolean onscr = false; // 노트가 화면에 떠 있는지
	int time; // 노트 위치
	int num; // 노트키
}note[NOTE_MAX];

struct _LINE {
	boolean onscr = false; // 라인이 화면에 떠 있는지
	int time; // 라인 위치
}line[LINE_MAX];

struct _OPTION {
	struct __HOTKEY {
		char key[9]; // 단축키
	}hotkey;
	float sp; // 스피드(배속)
}option;

struct _GAME {
	struct __NOTE {
		int note_count = 0; // 지금까지 지나간 노트 수
		int cnt = 0; // 노트 인덱스
		int hitgear[10] = { 0, }; // 노트를 처리한 기어 번호
	}note;
	struct __MUSIC {
		int id;
		wchar_t title[64] = { 0, }; // 타이틀
		wchar_t artist[64] = { 0, }; // 아티스트
		wchar_t comp[64] = { 0, }; // 작곡가
		wchar_t lyric[64] = { 0, }; // 작사가
		int bpm; // BPM
		int length; // 길이
		wchar_t df[4][8]; // Basic, Advanced, Extreme, Master
	}music[100];
	struct __CHOICE {
		int ptr; // 현재 선택된 노래 인덱스
		int df_ptr; // 현재 선택된 난이도 (1~4) // mas, ext, adv, bas
		int music_count; // 노래 총 개수
	}choice;
	struct __PLAY {
		int music_ptr; // 플레이하는 노래
		int df_ptr; // 플레이하는 난이도
		int perfect, great, good, ok, miss, mc, score; // 퍼펙, 그렛, 굳, 오케, 미스, 맥콤, 스코어
		double acc; // 달성률
		double skill; // 스킬
		int note_count; // 총 노트수
	}play;
	struct __LINE {
		int cnt = 0; // 라인 인덱스
	}line;

	int screen = 0; // 메인화면/플레이중/결과화면
	int playtime = 0; // 플레이시간(틱)
	int realframe = 0; // 리얼 프레임(BPM측정시 얻어낸 프레임)
	int combo_timer = 0; // 콤보 표시 움직임 타이머
	int beatcnt = 0; // 비트카운트 (마디선 생성용)
}game;

struct _ME {
	int perfect = 0, great = 0, bad = 0, miss = 0; // 판정 수
	int score = 0; // 점수
	int combo = 0; // 콤보
	int mc = 0; // 맥스 콤보
	int gauge = 0; // HP 게이지
	boolean press[9] = { false, }; // 키를 누르고 있는지
	int press_timer[9] = { 0, }; // 조명 타이머
}me;

#pragma region TCHAR
wchar_t Ttitle[9][64], Tartist[64];
char Tkey[7][10], Tspeed[5];
char TBS[30], TT[20], TFPS[20], TBPM[20], Tresolution[20], TnK[5], TNG[10], TGD[10];
char Tscore[20], Tpt[20], TP[20], TG[20], TB[20], TM[20], TAscScore[20], TMT[50];
char TPerfect[20], TGreat[20], TBad[20], TMiss[20], Tmc[20], Tacc[20];
char Tlevelcode[10], TmagicN[20];
#pragma endregion

void judgment(int);
void RT_CREATE_NOTE(int);
void RT_CREATE_LINE();
int MOST_NEAR_NOTE(int);
int digit(int);
int digit_num(int, int);
int round(float);
int _mod(int, int);
int EXC_SCORE(int);
boolean exist(char[]);
void GAME_START(char*);
int FIND_MP3();
int FIND_SONG_IMAGE();

void PlayMusic(char *music_name) {
	char str[256];
	strcpy(str, "open \"");
	strcat(str, music_name);
	strcat(str, "\" alias MediaFile");
	mciSendString(str, NULL, 0, NULL);
}

void StopMusic() {
	mciSendString("close MediaFile", NULL, 0, NULL);
}

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

/* 이미지 파일 그리기 */
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
	HDC hdc, indc, inindc;
	PAINTSTRUCT ps;
	HBITMAP bit, obit;
	HBRUSH brush, obrush;
	HFONT title_font, artist_font, result_font, df_font, df_font2, mod_font, df_dis_font, totalnote_font, Tbpm_font, bpm_font, acc_font, acc_font2, ofont;
	HFONT r_df_font, r_df_font2, r_acc_font, r_acc_font2, r_skill_font, r_skill_font2, r_judgetext_font, r_judge_font;
	HPEN pen, open;
	RECT rt, wrect, rect;
	DWORD dwRead, dwWritten;
	FILE *fr, *fw;
	INT i;
	static INT x, y;
	INT cur_screen_width = GAME_RESOLUTION_WIDTH, cur_screen_height = GAME_RESOLUTION_HEIGHT;
	DOUBLE ex_width = 1.0, ex_height = 1.0;

	switch (iMessage) {
	case WM_CREATE:
	{
		#pragma region 곡 정보 불러오기
		setlocale(LC_ALL, ".OCP");
		wchar_t count[16], id[16], bpm[16], length[16];
		char len[16];

		fr = _wfopen(L"musicinfo.txt", L"r, ccs=UTF-16LE");
		fwscanf(fr, L"%s", count);
		game.choice.music_count = _wtoi(count);

		for (i = 0; i < game.choice.music_count; i++) {
			fwscanf(fr, L"%s", id);
			fwscanf(fr, L"%s%s%s%s", game.music[i].title, game.music[i].artist, game.music[i].comp, game.music[i].lyric);
			fwscanf(fr, L"%s%s", bpm, length);
			fwscanf(fr, L"%s%s%s%s", game.music[i].df[0], game.music[i].df[1], game.music[i].df[2], game.music[i].df[3]);

			game.music[i].id = _wtoi(id);
			game.music[i].bpm = _wtoi(bpm);
			wcstombs(len, length, 16);
			game.music[i].length = (len[0] - '0') * 60 + (len[2] - '0') * 10 + (len[3] - '0');
		}
		fclose(fr);
		#pragma endregion

		#pragma region 옵션 데이터 불러오기
		fr = fopen("option.ini", "r");
		fscanf(fr, "Crash Cymbal: %c\n", &option.hotkey.key[0]);
		fscanf(fr, "Hihat: %c\n", &option.hotkey.key[1]);
		fscanf(fr, "Left Pedal: %c\n", &option.hotkey.key[2]);
		fscanf(fr, "Snare: %c\n", &option.hotkey.key[3]);
		fscanf(fr, "High Tom: %c\n", &option.hotkey.key[4]);
		fscanf(fr, "Right Pedal: %c\n", &option.hotkey.key[5]);
		fscanf(fr, "Low Tom: %c\n", &option.hotkey.key[6]);
		fscanf(fr, "Floor Tom: %c\n", &option.hotkey.key[7]);
		fscanf(fr, "Ride Cymbal: %c\n", &option.hotkey.key[8]);
		fscanf(fr, "Speed: %.1f\n", &option.sp);
		fclose(fr);
		#pragma endregion

		#pragma region Frame
		SetTimer(hWnd, 1, 1000 / FPS, NULL);
		#pragma endregion

		game.screen = GP_RESULT;
		me.score = 520498;
		option.sp = 4.5;
		me.gauge = 100;
		game.choice.ptr = 10;
		game.choice.df_ptr = 1;
		game.play.music_ptr = 32;
		game.play.df_ptr = 1;
		game.play.perfect = 848;
		game.play.great = 27;
		game.play.good = 4;
		game.play.ok = 0;
		game.play.miss = 2;
		game.play.mc = 749;
		game.play.note_count = 881;
		game.play.score = 936094;

	}
	break;
	case WM_TIMER:
	{
		switch (wParam) {
		case 1:
			#pragma region 윈도우 창 크기로 비율 설정
			GetWindowRect(hWnd, &wrect);
			cur_screen_width = wrect.right - wrect.left;
			cur_screen_height = wrect.bottom - wrect.top;
			ex_width = (double)cur_screen_width / (double)GAME_RESOLUTION_WIDTH;
			ex_height = (double)cur_screen_height / (double)(GAME_RESOLUTION_HEIGHT+ TASKBAR_HEIGHT);
			#pragma endregion

			#pragma region DoubleBuffering_head
			GetClientRect(hWnd, &rt);
			hdc = GetDC(hWnd);
			indc = CreateCompatibleDC(hdc);
			if (hb == NULL) {
				hb = CreateCompatibleBitmap(hdc, wrect.right, wrect.bottom);
			}
			ob = (HBITMAP)SelectObject(indc, hb);
			FillRect(indc, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));
			SetBkMode(indc, TRANSPARENT);
			#pragma endregion

#pragma region 노트 생성
			if (game.screen == GP_PLAY) {
				if (game.beatcnt-- <= 0) { // 마디선 생성은 조금 고민해봐야함
					game.beatcnt = 8;
					RT_CREATE_LINE();
				}
			}
#pragma endregion

#pragma region 노트&마디선 낙하
			for (i = 0; i < NOTE_MAX; i++) { // 노트가 떨어지게 함
				if (note[i].onscr)
					note[i].time -= option.sp;
			}
			for (i = 0; i < LINE_MAX; i++) {
				if (line[i].onscr)
					line[i].time -= option.sp;
			}
#pragma endregion

#pragma region 플레이 타임 증가
			game.playtime++;
#pragma endregion

#pragma region 노트 이펙트 타이머
			for (i = 1; i < 8; i++) {
				game.note.hitgear[i] = game.note.hitgear[i] > 0 ? game.note.hitgear[i] - 1 : game.note.hitgear[i];
			}
#pragma endregion

#pragma region 콤보 표시 타이머
			game.combo_timer = game.combo_timer > 0 ? game.combo_timer - 1 : game.combo_timer;
#pragma endregion

#pragma region 키 조명 타이머
			for (i = 0; i < 7; i++) {
				if (me.press_timer[i] <= 0)
					me.press[i] = false;
				if (me.press[i])
					me.press_timer[i]--;
			}
#pragma endregion

			#pragma region 폰트 생성
			setlocale(LC_ALL, ".OCP");
			title_font = CreateFont(H(40), 0, 0, 0, 500, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, NULL, TEXT("Meiryo"));
			artist_font = CreateFont(H(30), 0, 0, 0, 500, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, NULL, TEXT("Meiryo"));
			result_font = CreateFont(H(35), 0, 0, 0, 500, 1, 0, 0, ANSI_CHARSET, 0, 0, 0, NULL, TEXT("Meiryo"));
			mod_font = CreateFont(H(12), 0, 0, 0, 100, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			df_dis_font = CreateFont(H(16), 0, 0, 0, 100, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			df_font = CreateFont(H(50), 0, 0, 0, 100, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			df_font2 = CreateFont(H(35), 0, 0, 0, 100, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			Tbpm_font = CreateFont(H(15), 0, 0, 0, 100, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			bpm_font = CreateFont(H(30), 0, 0, 0, 500, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무B"));

			r_df_font = CreateFont(H(55), 0, 0, 0, 500, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_df_font2 = CreateFont(H(47), 0, 0, 0, 500, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_acc_font = CreateFont(H(55), 0, 0, 0, 500, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_acc_font2 = CreateFont(H(47), 0, 0, 0, 500, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_skill_font = CreateFont(H(85), 0, 0, 0, 900, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_skill_font2 = CreateFont(H(55), 0, 0, 0, 900, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_judgetext_font = CreateFont(H(25), 0, 0, 0, 200, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			r_judge_font = CreateFont(H(25), 0, 0, 0, 200, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, NULL, TEXT("HY나무L"));
			#pragma endregion

			pen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
			open = (HPEN)SelectObject(indc, pen);
			brush = (HBRUSH)GetStockObject(NULL_BRUSH);
			obrush = (HBRUSH)SelectObject(indc, brush);
			SetBkMode(indc, TRANSPARENT);
			__textcolor(255, 255, 255);

			#pragma region 곡 선택 화면
			if (game.screen == GP_CHOICE) // 곡 선택 화면
			{
				/* BG 표시 */
				__draw(L".\\image\\matixx-1.png", 0, 0, GAME_RESOLUTION_WIDTH, GAME_RESOLUTION_HEIGHT);

				/* 자켓 표시 */
				__pen(150, 185, 253, 4);
				__rect(315, 35, 615, 335);
				WCHAR src[32];
				wsprintfW(src, L".\\image\\jacket\\%d.png", game.music[game.choice.ptr].id);
				__draw(src, 315, 35, 300, 300);

				/* 곡 리스트 - 자켓 선 표시 */
				__pen(150, 185, 243, 4);
				__line(615, 185, 695, 185);
				__line(695, 185, 695, 320);

				/* 곡 리스트 표시 */
				__pen(100, 100, 100, 1);
				for (i = 0; i < 9; i++) {
					if (i == 4) {
						__pen(150, 185, 253, 4);
						__rect(675, 320, 1281, 395);
						__rect(715, 325, 780, 390);
						WCHAR src[32];
						wsprintfW(src, L".\\image\\jacket\\%d.png", game.music[game.choice.ptr].id);
						__draw(src, 715, 325, 65, 65);
						__pen(100, 100, 100, 1);
					}
					else {
						__rect(710, 20 + 75 * i, 1281, 20 + 75 * (i + 1));
						__rect(750, 25 + 75 * i, 815, 25 + 75 * i + 65);
						WCHAR src[32];
						wsprintfW(src, L".\\image\\jacket\\%d.png", game.music[_mod((game.choice.ptr + i - 4), game.choice.music_count)].id);
						__draw(src, 750, 25 + 75 * i, 65, 65);
					}
				}

				/* 곡 리스트 스크롤 바 표시 */
				__rect(1254, 140, 1260, 640);

				/* BPM 표시 */
				__brush(43, 50, 69);
				__rect(52, 302, 290, 340);
				__line(73, 330, 282, 330);
				__font(Tbpm_font);
				__text("BPM", 73, 306);
				__font(bpm_font);
				char Tbpm[4];
				sprintf(Tbpm, "%d", game.music[game.choice.ptr].bpm);
				__text(Tbpm, 135, 303);

				/* 드럼 분포도 표시 */
				__nullbrush;
				__rect(213, 353, 370, 685);

				/* 난이도 표시 */
				__rect(427, 353, 653, 690);
				WCHAR Tdf[4];
				for (i = 0; i < 4; i++) {
					DeleteObject(SelectObject(indc, obrush));
					brush = CreateSolidBrush(df_color[i]);
					obrush = (HBRUSH)SelectObject(indc, brush);
					__rect(532, 370 + 80 * i, 638, 386 + 80 * i);

					__brush(0, 0, 0);
					__rect(532, 386 + 80 * i, 638, 438 + 80 * i);

					SetTextAlign(indc, TA_BOTTOM | TA_LEFT);
					__font(mod_font);
					__text("DRUM", 533, 386 + 80 * i);

					SetTextAlign(indc, TA_BOTTOM | TA_RIGHT);
					__font(df_dis_font);
					__text(Tdf_dis[i], 637, 387 + 80 * i);
					SetTextAlign(indc, TA_TOP | TA_LEFT);

					__font(df_font);
					wsprintfW(Tdf, L"%c.", game.music[game.choice.ptr].df[3 - i][0]);
					__textW(Tdf, 550, 390 + 80 * i);

					__font(df_font2);
					wsprintfW(Tdf, L"%c%c", game.music[game.choice.ptr].df[3 - i][2], game.music[game.choice.ptr].df[3 - i][3]);
					__textW(Tdf, 586, 403 + 80 * i);
				}
				__pen(150, 185, 253, 4);
				__nullbrush;
				__rect(434, 285 + 80 * game.choice.df_ptr, 646, 368 + 80 * game.choice.df_ptr);
				

				/* 곡 정보 표시 */
				/* 타이틀 표시 */
				__pen(100, 100, 100, 1);
				__font(title_font);
				for (i = 0; i < 9; i++) {
					wsprintfW(Ttitle[i], L"%s", game.music[_mod((game.choice.ptr + i - 4), game.choice.music_count)].title);
					if (i == 4) { 
						__textW(Ttitle[i], 790, 40 + 75 * i);
					}
					else {
						__textW(Ttitle[i], 825, 40 + 75 * i);
					}
				}

				/* 아티스트 표시 */
				__font(artist_font);
				wsprintfW(Tartist, L"%s", game.music[game.choice.ptr].artist);
				__textW(Tartist, 810, 368);
			}
			#pragma endregion

			#pragma region 플레이 화면
			else if (game.screen == GP_PLAY) // 플레이 화면
			{
				#pragma region 키 조명 그리기
				for (i = 0; i < 9; i++) {
					if (me.press[i]) {
						switch (i) {
						case 0: __draw(L".\\image\\C_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XC - 1, 440); break;
						case 1: __draw(L".\\image\\H_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XH - 1, 440); break;
						case 2: __draw(L".\\image\\L_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XL - 1, 440); break;
						case 3: __draw(L".\\image\\S_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XS - 1, 440); break;
						case 4: __draw(L".\\image\\T_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XT - 1, 440); break;
						case 5: __draw(L".\\image\\K_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XK - 1, 440); break;
						case 6: __draw(L".\\image\\M_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XM - 1, 440); break;
						case 7: __draw(L".\\image\\F_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XF - 1, 440); break;
						case 8: __draw(L".\\image\\R_light.png", GEAR_DIST0 + GearX[i] + 1, 0, GEAR_XR - 1, 440); break;
						}
					}
				}
				#pragma endregion

				#pragma region 기어 그리기
				for (i = 1; i < 9; i++) {
					__line(GEAR_DIST0 + GearX[i], 0, GEAR_DIST0 + GearX[i], GEARHEIGHT + 2 + 59);
				}
				__line(GEAR_DIST0, 0, GEAR_DIST0, GAME_RESOLUTION_HEIGHT);
				__line(GEAR_DIST0 + GearX[10], 0, GEAR_DIST0 + GearX[10], GAME_RESOLUTION_HEIGHT);
				__line(GEAR_DIST0, GEARHEIGHT - JUDGELINE_UP, GEAR_DIST0 + GearX[10], GEARHEIGHT - JUDGELINE_UP);
				__line(GEAR_DIST0, GEARHEIGHT + JUDGELINE_DOWN, GEAR_DIST0 + GearX[10], GEARHEIGHT + JUDGELINE_DOWN);
				__line(GEAR_DIST0, GEARHEIGHT + JUDGELINE_DOWN + DRUM_DISPLAY + GAUGE_DISPLAY, GEAR_DIST0 + GearX[10], GEARHEIGHT + JUDGELINE_DOWN + DRUM_DISPLAY + GAUGE_DISPLAY);
				__draw(L".\\image\\drumset.png", GEAR_DIST0 + GEAR_LEFT + 1, GEARHEIGHT + JUDGELINE_DOWN + 1, 515, 58);
				#pragma endregion

				#pragma region 스피드 표시
				__draw(L".\\image\\speed.png", GEAR_DIST0 + GearX[0] + 15, GEARHEIGHT + JUDGELINE_DOWN + DRUM_DISPLAY + 10, 32, 10);
				sprintf(Tspeed, "%.1f", option.sp);
				__text(Tspeed, GEAR_DIST0 + GearX[0] + 15, GEARHEIGHT + JUDGELINE_DOWN + DRUM_DISPLAY + 25);
				#pragma endregion

				#pragma region 게이지 표시
				switch (me.gauge / 33) {
				case 0: __brush(255, 97, 97); break;
				case 1: __brush(0, 169, 237); break;
				case 2: __brush(0, 169, 237); break;
				case 3: __brush(229, 240, 0); break;
				}

				__rect(GEAR_DIST0 + GearX[1], GEARHEIGHT + JUDGELINE_DOWN + DRUM_DISPLAY + 10, GEAR_DIST0 + GearX[1] + (double)430 * (double)me.gauge / (double)100, GEARHEIGHT + JUDGELINE_DOWN + DRUM_DISPLAY + 40);
				__nullbrush;
				#pragma endregion

				#pragma region 제목 표시
				__rect(953, 10, 953 + INGAME_JACKET_SIZE, 10 + INGAME_JACKET_SIZE); // 실제 게임보다 X축-30
				__rect(1021, 10, 1021 + INGAME_MUSICNAME_WIDTH, 10 + INGAME_MUSICNAME_HEIGHT); // 실제 게임보다 X축-30
				#pragma endregion

				#pragma region 스코어 표시
				__draw(L".\\image\\score.png", 108, 50, 72, 54);
				for (i = 0; i < 6; i++) {
					switch (digit_num(me.score, i + 1)) {
					case 0: __draw(L".\\image\\score-0.png", 266 - 26 * i, 124 - 4 * i, 20, 54); break;
					case 1: __draw(L".\\image\\score-1.png", 266 - 26 * i, 124 - 4 * i, 13, 53); break;
					case 2: __draw(L".\\image\\score-2.png", 266 - 26 * i, 124 - 4 * i, 22, 55); break;
					case 3: __draw(L".\\image\\score-3.png", 266 - 26 * i, 124 - 4 * i, 22, 57); break;
					case 4: __draw(L".\\image\\score-4.png", 266 - 26 * i, 124 - 4 * i, 23, 55); break;
					case 5: __draw(L".\\image\\score-5.png", 266 - 26 * i, 124 - 4 * i, 19, 53); break;
					case 6: __draw(L".\\image\\score-6.png", 266 - 26 * i, 124 - 4 * i, 19, 51); break;
					case 7: __draw(L".\\image\\score-7.png", 266 - 26 * i, 124 - 4 * i, 17, 51); break;
					case 8: __draw(L".\\image\\score-8.png", 266 - 26 * i, 124 - 4 * i, 21, 56); break;
					case 9: __draw(L".\\image\\score-9.png", 266 - 26 * i, 124 - 4 * i, 22, 54); break;
					}
				}
				#pragma endregion

				/*
				#pragma region 노트 이펙트 그리기
				for (i = 1; i < 8; i++) {
				switch (game.note.hitgear[i] / 2) {
				case 10: OnPaint(indc, L".\\image\\NoteEffect01.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  9: OnPaint(indc, L".\\image\\NoteEffect02.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  8: OnPaint(indc, L".\\image\\NoteEffect03.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  7: OnPaint(indc, L".\\image\\NoteEffect04.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  6: OnPaint(indc, L".\\image\\NoteEffect05.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  5: OnPaint(indc, L".\\image\\NoteEffect06.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  4: OnPaint(indc, L".\\image\\NoteEffect07.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  3: OnPaint(indc, L".\\image\\NoteEffect08.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  2: OnPaint(indc, L".\\image\\NoteEffect09.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  1: OnPaint(indc, L".\\image\\NoteEffect10.png", (GEAR_DIST0 + GearX[option.key_count][i - 1] - 75) * ex_width, 350 * ex_height, 200 * ex_width, 200 * ex_height); break;
				case  0: break;
				}
				}
				#pragma endregion

				#pragma region 게임 중 상태표시
				sprintf(Tresolution, "%dx%d", cur_screen_width, cur_screen_height);
				sprintf(Tpt, "%02d:%02d", TIME / 60, TIME % 60);
				sprintf(Tscore, "%d", me.score);
				sprintf(TP, "PERFECT: %d", me.perfect);
				sprintf(TG, "GREAT: %d", me.great);
				sprintf(TB, "BAD: %d", me.bad);
				sprintf(TM, "MISS: %d", me.miss);
				sprintf(Tacc, "%.2f％", CAL_ACC);
				if (option.fk)	sprintf(Tmode, "[  %s %d  ] [F]", mode[option.mode - 1], option.df);
				else            sprintf(Tmode, "[  %s %d  ]", mode[option.mode - 1], option.df);
				sprintf(TBS, "BPM: %d x %d (FPS %d)", option.bpm, option.sp, (int)(1000.0 / (float)game.realframe));
				sprintf(TnK, "%dK", option.key_count);
				if (option.dp == 1)			strcpy(TNG, "BITMAP");
				else if (option.dp == 2)	strcpy(TNG, "RECTANGLE");
				if (option.gd == 1)			strcpy(TGD, "VISIBLE");
				else if (option.gd == 2)	strcpy(TGD, "GHOST");
				sprintf(TAscScore, "+ %d", obt_score);

				// 기어 밑에 버튼 키 표시
				for (i = 0; i < 7; i++) {
				if (option.hotkey.key[option.key_count][i] == 0x20)
				sprintf(Tkey[i], "SP");
				else
				sprintf(Tkey[i], "%c", option.hotkey.key[option.key_count][i]);
				}

				TextOut(indc, 10 * ex_width, 10 * ex_height, Tresolution, strlen(Tresolution));
				TextOut(indc, 10 * ex_width, 30 * ex_height, Tpt, strlen(Tpt));
				TextOut(indc, 10 * ex_width, 40 * ex_height, Tscore, strlen(Tscore));
				TextOut(indc, 10 * ex_width, 60 * ex_height, TP, strlen(TP));
				TextOut(indc, 10 * ex_width, 70 * ex_height, TG, strlen(TG));
				TextOut(indc, 10 * ex_width, 80 * ex_height, TB, strlen(TB));
				TextOut(indc, 10 * ex_width, 90 * ex_height, TM, strlen(TM));
				TextOut(indc, 10 * ex_width, 100 * ex_height, Tacc, strlen(Tacc));
				TextOut(indc, 420 * ex_width, 10 * ex_height, Tmode, strlen(Tmode));
				TextOut(indc, 420 * ex_width, 20 * ex_height, TBS, strlen(TBS));
				TextOut(indc, 420 * ex_width, 40 * ex_height, TnK, strlen(TnK));
				TextOut(indc, 420 * ex_width, 50 * ex_height, TNG, strlen(TNG));
				TextOut(indc, 420 * ex_width, 60 * ex_height, TGD, strlen(TGD));

				TextOut(indc, 230 * ex_width, (scoreY + 60) * ex_height, TAscScore, strlen(TAscScore));
				TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][0] + 15) * ex_width, 510 * ex_height, Tkey[0], strlen(Tkey[0]));
				TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][1] + 15) * ex_width, 510 * ex_height, Tkey[1], strlen(Tkey[1]));
				TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][2] + 15) * ex_width, 510 * ex_height, Tkey[2], strlen(Tkey[2]));
				TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][3] + 20) * ex_width, 510 * ex_height, Tkey[3], strlen(Tkey[3]));
				if (option.key_count >= 5)	TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][4] + 15) * ex_width, 510 * ex_height, Tkey[4], strlen(Tkey[4]));
				if (option.key_count >= 6)	TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][5] + 15) * ex_width, 510 * ex_height, Tkey[5], strlen(Tkey[5]));
				if (option.key_count >= 7)	TextOut(indc, (GEAR_DIST0 + GearX[option.key_count][6] + 15) * ex_width, 510 * ex_height, Tkey[6], strlen(Tkey[6]));
				SelectObject(indc, open);
				DeleteObject(pen);
				#pragma endregion

				#pragma region 콤보 그리기
				// 콤보 자리수별 x좌표 설정
				switch (digit(me.combo)) {
				case 1:
				scorex[0] = 220;
				break;
				case 2:
				scorex[0] = 245;
				scorex[1] = 195;
				break;
				case 3:
				scorex[0] = 270;
				scorex[1] = 220;
				scorex[2] = 170;
				break;
				case 4:
				scorex[0] = 295;
				scorex[1] = 245;
				scorex[2] = 195;
				scorex[3] = 145;
				break;
				case 5:
				scorex[0] = 320;
				scorex[1] = 270;
				scorex[2] = 220;
				scorex[3] = 170;
				scorex[4] = 120;
				break;
				case 6:
				scorex[0] = 345;
				scorex[1] = 295;
				scorex[2] = 245;
				scorex[3] = 195;
				scorex[4] = 145;
				scorex[5] = 95;
				break;
				}

				for (i = 0; i < digit(me.combo); i++) {
				switch (digit_num(me.combo, i + 1)) {
				case 0:
				OnPaint(indc, L".\\image\\score0.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 1:
				OnPaint(indc, L".\\image\\score1.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 2:
				OnPaint(indc, L".\\image\\score2.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 3:
				OnPaint(indc, L".\\image\\score3.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 4:
				OnPaint(indc, L".\\image\\score4.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 5:
				OnPaint(indc, L".\\image\\score5.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 6:
				OnPaint(indc, L".\\image\\score6.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 7:
				OnPaint(indc, L".\\image\\score7.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 8:
				OnPaint(indc, L".\\image\\score8.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				case 9:
				OnPaint(indc, L".\\image\\score9.png", scorex[i] * ex_width, (scoreY - game.combo_timer * 4) * ex_height, 43 * ex_width, 64 * ex_height);
				break;
				}
				}
				#pragma endregion

				#pragma region 마디선 그리기
				for (i = 0; i < LINE_MAX; i++)
				{
				brush = (HBRUSH)GetStockObject(DC_BRUSH);
				obrush = (HBRUSH)SelectObject(indc, brush);

				if (line[i].onscr) { // 마디선이 화면에 떠 있으면
				if (line[i].time <= -52) // 마디선이 화면 밖으로 나가면 그리지 않음
				{
				line[i].onscr = false;
				}
				else if (line[i].time <= GEARHEIGHT)
				{
				SetDCBrushColor(indc, RGB(150, 150, 150));
				Rectangle(indc, (GEAR_DIST0 + 1) * ex_width, (GEARHEIGHT - line[i].time - 2) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][option.key_count]) * ex_width, (GEARHEIGHT - line[i].time + 2) * ex_height);
				}
				}

				SetBkMode(indc, TRANSPARENT);
				SelectObject(indc, obrush);
				DeleteObject(brush);
				}
				#pragma endregion

				#pragma region 노트 처리
				for (i = 0; i < NOTE_MAX; i++)
				{
				if (note[i].onscr) { // 노트가 화면에 떠 있으면
				if (note[i].time <= -6 * option.sp) { // 노트를 놓치면 미스 처리
				//PlaySound(".\\sound\\miss.wav", NULL, SND_ASYNC);
				Ptext = 0;
				Gtext = 0;
				Btext = 0;
				Mtext = 1;
				me.miss++;
				me.combo = 0;
				note[i].onscr = false;
				}
				else if (note[i].time <= -52) // 노트가 화면 밖으로 나가면 그리지 않음
				{

				}
				#pragma region 노트 그리기
				else if (note[i].time <= GEARHEIGHT)
				{
				#pragma region 비트맵 그래픽 노트
				if (option.dp == 1) { // 비트맵
				inindc = CreateCompatibleDC(indc);

				switch (option.key_count) {
				case 4:
				if (note[i].num == 2 || note[i].num == 3) {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE2));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_4K_DIST2 * ex_width, 10 * ex_height, inindc, 0, 0, 35, 10, SRCCOPY);
				}
				else {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE1));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_4K_DIST1 * ex_width, 10 * ex_height, inindc, 0, 0, 40, 10, SRCCOPY);
				}
				break;
				case 5:
				if (note[i].num == 2 || note[i].num == 4) {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE2));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_5K_DIST2 * ex_width, 10 * ex_height, inindc, 0, 0, 35, 10, SRCCOPY);
				}
				else if (note[i].num == 3) {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE3));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_5K_DIST3 * ex_width, 10 * ex_height, inindc, 0, 0, 50, 10, SRCCOPY);
				}
				else {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE1));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_5K_DIST1 * ex_width, 10 * ex_height, inindc, 0, 0, 40, 10, SRCCOPY);
				}
				break;
				case 6:
				if (note[i].num == 2 || note[i].num == 5) {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE2));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_6K_DIST2 * ex_width, 10 * ex_height, inindc, 0, 0, 35, 10, SRCCOPY);
				}
				else {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE1));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_6K_DIST1 * ex_width, 10 * ex_height, inindc, 0, 0, 40, 10, SRCCOPY);
				}
				break;
				case 7:
				if (note[i].num == 2 || note[i].num == 6) {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE2));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_7K_DIST2 * ex_width, 10 * ex_height, inindc, 0, 0, 35, 10, SRCCOPY);
				}
				else if (note[i].num == 4) {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE3));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_7K_DIST4 * ex_width, 10 * ex_height, inindc, 0, 0, 50, 10, SRCCOPY);
				}
				else {
				bit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NOTE1));
				obit = (HBITMAP)SelectObject(inindc, bit);
				StretchBlt(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 4) * ex_height,
				GEAR_7K_DIST1 * ex_width, 10 * ex_height, inindc, 0, 0, 40, 10, SRCCOPY);
				}
				break;
				}

				SelectObject(inindc, obit);
				DeleteObject(bit);
				DeleteDC(inindc);
				}
				#pragma endregion

				#pragma region 직사각형 그래픽 노트
				else if (option.dp == 2) {
				brush = (HBRUSH)GetStockObject(DC_BRUSH);
				obrush = (HBRUSH)SelectObject(indc, brush);

				switch (option.key_count) {
				case 4:
				if (note[i].num == 2 || note[i].num == 3) {
				SetDCBrushColor(indc, RGB(57, 203, 239));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_4K_DIST2) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				else {
				SetDCBrushColor(indc, RGB(214, 218, 222));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_4K_DIST1) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				break;
				case 5:
				if (note[i].num == 2 || note[i].num == 4) {
				SetDCBrushColor(indc, RGB(57, 203, 239));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_5K_DIST2) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				else if (note[i].num == 3) {
				SetDCBrushColor(indc, RGB(234, 181, 58));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_5K_DIST3) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				else {
				SetDCBrushColor(indc, RGB(214, 218, 222));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_5K_DIST1) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				break;
				case 6:
				if (note[i].num == 2 || note[i].num == 5) {
				SetDCBrushColor(indc, RGB(57, 203, 239));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_6K_DIST2) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				else {
				SetDCBrushColor(indc, RGB(214, 218, 222));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_6K_DIST1) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				break;
				case 7:
				if (note[i].num == 2 || note[i].num == 6) {
				SetDCBrushColor(indc, RGB(57, 203, 239));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_7K_DIST2) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				else if (note[i].num == 4) {
				SetDCBrushColor(indc, RGB(234, 181, 58));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_7K_DIST4) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				else {
				SetDCBrushColor(indc, RGB(214, 218, 222));
				Rectangle(indc, (GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + 1) * ex_width, (GEARHEIGHT - note[i].time - 5) * ex_height,
				(GEAR_DIST0 + GearX[option.key_count][note[i].num - 1] + GEAR_7K_DIST1) * ex_width, (GEARHEIGHT - note[i].time + 5) * ex_height);
				}
				break;
				}

				SetBkMode(indc, TRANSPARENT);
				SelectObject(indc, obrush);
				DeleteObject(brush);
				}
				#pragma endregion

				}
				#pragma endregion


				}
				}
				#pragma endregion

				#pragma region 판정 그리기
				if (Ptext == 1) {
				OnPaint(indc, L".\\image\\perfect.png", 175 * ex_width, judgeY * ex_height, 133 * ex_width, 25 * ex_height);
				}
				else if (Gtext == 1) {
				OnPaint(indc, L".\\image\\great.png", 185 * ex_width, judgeY * ex_height, 113 * ex_width, 25 * ex_height);
				}
				else if (Btext == 1) {
				OnPaint(indc, L".\\image\\bad.png", 205 * ex_width, judgeY * ex_height, 74 * ex_width, 25 * ex_height);
				}
				else if (Mtext == 1) {
				OnPaint(indc, L".\\image\\miss.png", 203 * ex_width, judgeY * ex_height, 78 * ex_width, 25 * ex_height);
				}
				#pragma endregion
				*/
			}
#pragma endregion

			#pragma region 결과 화면
			else if (game.screen == GP_RESULT) // 결과 화면
			{
				/* 곡 자켓 표시 */
				WCHAR src[32];
				wsprintfW(src, L".\\image\\jacket\\%d.png", game.music[game.play.music_ptr].id);
				__draw(src, 442, 125, 388, 388);

				/* 곡 제목, 아티스트, 난이도 표시 */
				__rect(440, 530, 829, 663);
				__line(449, 623, 819, 623);
				__brush(143, 145, 142);
				__rect(449, 539, 819, 612);
				DeleteObject(SelectObject(indc, obrush));
				brush = CreateSolidBrush(df_color[game.play.df_ptr - 1]);
				obrush = (HBRUSH)SelectObject(indc, brush);
				__rect(658, 632, 819, 655);
				__font(title_font);
				wsprintfW(Ttitle[i], L"%s", game.music[game.play.music_ptr].title);
				__textW(Ttitle[i], 456, 547);
				__font(artist_font);
				wsprintfW(Ttitle[i], L"%s", game.music[game.play.music_ptr].artist);
				__textW(Ttitle[i], 456, 585);
				SetTextAlign(indc, TA_BOTTOM | TA_LEFT);
				__font(mod_font);
				__text("DRUM", 662, 654);
				SetTextAlign(indc, TA_BOTTOM | TA_RIGHT);
				__font(df_dis_font);
				__text(Tdf_dis[game.play.df_ptr - 1], 815, 655);
				SetTextAlign(indc, TA_TOP | TA_LEFT);

				/* 난이도, 달성률, 스킬 표시 */
				char Temp[10];
				char Tacc[10];
				char Tskill[10];
				__pen(208, 235, 233, 2);
				__line(916, 213, 1258, 213);
				__font(result_font);
				__text("難易度", 920, 184);
				__line(890, 286, 1233, 286);
				__text("達成率", 898, 256);
				__line(842, 415, 1181, 415);
				__text("曲別SKILL", 851, 382);

				/* 난이도 표시 */
				__font(r_df_font);
				sprintf(Temp, "%c.", game.music[game.play.music_ptr].df[4 - game.play.df_ptr][0]);
				__text(Temp, 1081, 160);
				__font(r_df_font2);
				sprintf(Temp, "%c%c", game.music[game.play.music_ptr].df[4 - game.play.df_ptr][2], game.music[game.play.music_ptr].df[4 - game.play.df_ptr][3]);
				__text(Temp, 1125, 168);

				/* 달성률 표시 */
				game.play.acc = (double)(game.play.perfect * 90 + game.play.great * 40 + game.play.mc * 10) / (double)game.play.note_count; // 달성률 = P*90+G*40+MC*10 / 총 노트수
				gcvt(game.play.acc, 4, Tacc);
				__font(r_acc_font);
				sprintf(Temp, "%c%c.", Tacc[0], Tacc[1]);
				__text(Temp, 1044, 232);
				__font(r_acc_font2);
				sprintf(Temp, "%c%c％", Tacc[3], Tacc[4]);
				__text(Temp, 1120, 240);

				/* 스킬 표시 */
				game.play.skill = wcstod(game.music[game.play.music_ptr].df[4 - game.play.df_ptr], NULL) * game.play.acc / 5.0; // 스킬 = 난이도 * 달성률/5
				gcvt(game.play.skill, 5, Tskill);
				__font(r_skill_font);
				sprintf(Temp, "%c%c%c.", Tskill[0], Tskill[1], Tskill[2]);
				__text(Temp, 992, 328);
				__font(r_skill_font2);
				sprintf(Temp, "%c%c", Tskill[4], Tskill[5]);
				__text(Temp, 1129, 353);

				/* 스코어 계산 */
				game.play.score = (int)((double)27384632 / (double)EXC_SCORE(game.play.note_count) * 1000000.0); // 스코어 = 가점 / EXC점 * 1,000,000

				/* 판정, 스코어 표시 */
				char judgetext[][12] = {"Perfect", "Great", "Good", "Ok", "Miss", "MaxCombo", "Score"};
				__font(r_judgetext_font);
				for (i = 0; i < 7; i++) {
					__text(judgetext[i], 850, 482 + 24 * i);
				}
				__font(r_judge_font);
				SetTextAlign(indc, TA_TOP | TA_RIGHT);
				sprintf(Temp, "%d", game.play.perfect);			__text(Temp, 1034, 483);
				sprintf(Temp, "%d", game.play.great);			__text(Temp, 1034, 507);
				sprintf(Temp, "%d", game.play.good);			__text(Temp, 1034, 531);
				sprintf(Temp, "%d", game.play.ok);				__text(Temp, 1034, 555);
				sprintf(Temp, "%d", game.play.miss);			__text(Temp, 1034, 579);
				sprintf(Temp, "%d", game.play.mc);				__text(Temp, 1034, 603);
				sprintf(Temp, "%d", game.play.score);			__text(Temp, 1070, 627);
				sprintf(Temp, "%d", (int)((double)game.play.perfect / (double)game.play.note_count * 100.0));	__text(Temp, 1094, 483);
				sprintf(Temp, "%d", (int)((double)game.play.great / (double)game.play.note_count * 100.0));		__text(Temp, 1094, 507);
				sprintf(Temp, "%d", (int)((double)game.play.good / (double)game.play.note_count * 100.0));		__text(Temp, 1094, 531);
				sprintf(Temp, "%d", (int)((double)game.play.ok / (double)game.play.note_count * 100.0));		__text(Temp, 1094, 555);
				sprintf(Temp, "%d", (int)((double)game.play.miss / (double)game.play.note_count * 100.0));		__text(Temp, 1094, 579);
				sprintf(Temp, "%d", (int)((double)game.play.mc / (double)game.play.note_count * 100.0));		__text(Temp, 1094, 603);
				SetTextAlign(indc, TA_TOP | TA_LEFT);
				for (i = 0; i < 6; i++) {
					__text("％", 1093, 485 + 24 * i);
				}
			}
			#pragma endregion


			#pragma region 오브젝트 삭제
			DeleteObject(SelectObject(indc, open));
			DeleteObject(SelectObject(indc, obrush));
			DeleteObject(title_font);
			DeleteObject(artist_font);
			DeleteObject(result_font);
			DeleteObject(mod_font);
			DeleteObject(df_dis_font);
			DeleteObject(df_font);
			DeleteObject(df_font2);
			DeleteObject(Tbpm_font);
			DeleteObject(bpm_font);
			DeleteObject(r_df_font);
			DeleteObject(r_df_font2);
			DeleteObject(r_acc_font);
			DeleteObject(r_acc_font2);
			DeleteObject(r_skill_font);
			DeleteObject(r_skill_font2);
			DeleteObject(r_judgetext_font);
			DeleteObject(r_judge_font);
			#pragma endregion

			#pragma region DoubleBuffering_tail
			SelectObject(indc, ob);
			DeleteDC(indc);
			ReleaseDC(hWnd, hdc);
			InvalidateRect(hWnd, NULL, FALSE);
			#pragma endregion
			break;
		}
	}
	break;
	case WM_KEYDOWN:
	{
		#pragma region 노트 누르기
		for (i = 0; i < 9; i++) {
			if (((wParam == 0x20 && option.hotkey.key[i] == 0x20) ||
				(wParam == option.hotkey.key[i] - 0x20))
				) {
				me.press[i] = true;
				me.press_timer[i] = 7;
				if (game.screen == GP_PLAY)
					judgment(i + 1);
			}
		}
		#pragma endregion

		switch (wParam)
		{
		case VK_UP:
			game.choice.ptr = game.choice.ptr <= 0 ? game.choice.music_count - 1 : game.choice.ptr - 1;
			break;
		case VK_DOWN:
			game.choice.ptr = game.choice.ptr >= game.choice.music_count - 1 ? 0 : game.choice.ptr + 1;
			break;
		case VK_LEFT:
			game.choice.df_ptr = game.choice.df_ptr <= 1 ? 4 : game.choice.df_ptr - 1;
			break;
		case VK_RIGHT:
			game.choice.df_ptr = game.choice.df_ptr >= 4 ? 1 : game.choice.df_ptr + 1;
			break;
		case 0x0D: // Enter
			if (game.screen == GP_CHOICE) { // 노래 선택
				game.screen = GP_PLAY;
				game.play.music_ptr = game.choice.ptr;
				game.play.df_ptr = game.choice.df_ptr;
				game.play.perfect = game.play.great = game.play.good = game.play.ok = game.play.miss = game.play.mc = game.play.score = 0;
			}
			/*GetWindowRect(hWnd, &rect);
			if (full_screen) {
				SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top + 40, rect.right, rect.bottom - 40, SWP_SHOWWINDOW);
				full_screen = 0;
			}
			else {
				SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top - 40, rect.right, rect.bottom + 40, SWP_SHOWWINDOW);
				full_screen = 1;
			}*/
			break;
		case VK_ESCAPE: // ESC
			if (game.screen == GP_RESULT) { // 결과창 나오기
				game.screen = GP_CHOICE;
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
	}
	break;
	case WM_LBUTTONDOWN:
	{
	}
	break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
#pragma region DoubleBuffering
		if (hb)
			DrawBitmap(hdc, 0, 0, hb);
#pragma endregion
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
	{
#pragma region DoubleBuffering
		if (hb) {
			DeleteObject(hb);
		}
#pragma endregion
#pragma region 옵션 데이터 저장
		fw = fopen("option.ini", "w");
		fprintf(fw, "Crash Cymbal: %c\n", option.hotkey.key[0]);
		fprintf(fw, "Hihat: %c\n", option.hotkey.key[1]);
		fprintf(fw, "Left Pedal: %c\n", option.hotkey.key[2]);
		fprintf(fw, "Snare: %c\n", option.hotkey.key[3]);
		fprintf(fw, "High Tom: %c\n", option.hotkey.key[4]);
		fprintf(fw, "Right Pedal: %c\n", option.hotkey.key[5]);
		fprintf(fw, "Low Tom: %c\n", option.hotkey.key[6]);
		fprintf(fw, "Floor Tom: %c\n", option.hotkey.key[7]);
		fprintf(fw, "Ride Cymbal: %c\n", option.hotkey.key[8]);
		fprintf(fw, "Speed: %.1f\n", option.sp);
		fclose(fw);
#pragma endregion
		PostQuitMessage(0);
		KillTimer(hWnd, 1);
	}
	break;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void judgment(int n)
{
	int i;
	double judgment_ratio; // 판정 배율
	int mnn = MOST_NEAR_NOTE(n);

	//judgment_ratio = 2 * pow((double)option.sp, 0.7);// Speed 배속일때의 판정배율은 judgment_ratio
	judgment_ratio = option.sp;
	obt_score = 20 + (int)pow((double)me.combo, 0.9);

	if (game.screen == GP_PLAY)
	{
		if (note[mnn].num == n)
		{
			if (note[mnn].time >= -(1.7*judgment_ratio) && note[mnn].time <= (1.7*judgment_ratio)) // 퍼펙트 처리
			{
				//PlaySound(".\\sound\\hit1.wav", NULL, SND_ASYNC);
				Ptext = 1;
				Gtext = 0;
				Btext = 0;
				Mtext = 0;
				game.note.note_count++;
				me.perfect++;
				me.combo++;
				me.score += obt_score;
				note[mnn].onscr = false;
				if (me.combo > me.mc)
					me.mc = me.combo;
				game.combo_timer = 8;
				game.note.hitgear[n] = NOTE_EFFECT_MAX * 2; // 히트 이펙트 표시용
			}
			else if (note[mnn].time >= -(4 * judgment_ratio) && note[mnn].time <= (4 * judgment_ratio)) // 그레이트 처리
			{
				//PlaySound(".\\sound\\hit1.wav", NULL, SND_ASYNC);
				Ptext = 0;
				Gtext = 1;
				Btext = 0;
				Mtext = 0;
				game.note.note_count++;
				me.great++;
				me.combo++;
				me.score += obt_score * 0.5;
				note[mnn].onscr = false;
				if (me.combo > me.mc)
					me.mc = me.combo;
				game.combo_timer = 8;
				game.note.hitgear[n] = NOTE_EFFECT_MAX * 2; // 히트 이펙트 표시용
			}
			else if (note[mnn].time >= -(6 * judgment_ratio) && note[mnn].time <= (6 * judgment_ratio)) // 배드 처리
			{
				//PlaySound(".\\sound\\hit1.wav", NULL, SND_ASYNC);
				Ptext = 0;
				Gtext = 0;
				Btext = 1;
				Mtext = 0;
				game.note.note_count++;
				me.bad++;
				me.combo = 0;
				me.score += obt_score * 0.25;
				note[mnn].onscr = false;
				game.combo_timer = 8;
			}
		}
	}
}

int MOST_NEAR_NOTE(int n)
{
	int min = 1000;
	int min_index = 100;

	for (int i = 0; i < NOTE_MAX; i++) {
		if (note[i].onscr) {
			if (note[i].num == n) {
				if (note[i].time < min) {
					min = note[i].time;
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

void RT_CREATE_NOTE(int n)
{
	/*
	실시간으로 노트를 생성한다.
	노트는 맨 꼭대기에 생성되며 시간이 지날수록 밑으로 내려가는 방식이다.
	*/
	note[game.note.cnt%NOTE_MAX].time = GEARHEIGHT;
	note[game.note.cnt%NOTE_MAX].num = n;
	note[game.note.cnt%NOTE_MAX].onscr = true;
	game.note.cnt++;
}

void RT_CREATE_LINE()
{
	line[game.line.cnt%LINE_MAX].time = GEARHEIGHT;
	line[game.line.cnt%LINE_MAX].onscr = true;
	game.line.cnt++;
}

void GAME_START(char *music_name)
{
	/* 값 초기화 */
	game.screen = GP_PLAY;
	game.playtime = 0;
	game.note.cnt = 0;
	game.note.note_count = 0;
	for (int i = 0; i < NOTE_MAX; i++)
		note[i].onscr = false;
	Ptext = Gtext = Btext = Mtext = 0;
	me.perfect = me.great = me.bad = me.miss = me.score = me.combo = me.mc = 0;

	char music_dir[256] = ".\\music\\";
	strcat(music_dir, music_name);
	PlayMusic(music_dir);
}
/*
int FIND_MP3()
{
	HANDLE hSrch;
	WIN32_FIND_DATA wfd;
	BOOL bResult = TRUE;
	int idx = 0;
	char temp[256];

	hSrch = FindFirstFile(".\\music\\*.mp3", &wfd);
	if (hSrch == INVALID_HANDLE_VALUE)	return -1;
	while (bResult) {
		wsprintf(temp, "%s", wfd.cFileName);
		strcpy(game.music.choice.music_title[idx++], strtok(temp, ".")); // 확장자는 떼고 저장
		bResult = FindNextFile(hSrch, &wfd);
	}
	FindClose(hSrch);

	return idx;
}

int FIND_SONG_IMAGE()
{
	HANDLE hSrch;
	WIN32_FIND_DATA wfd;
	BOOL bResult = TRUE;
	int idx = 0;
	char temp[256];

	hSrch = FindFirstFile(".\\song_image\\*.jpg", &wfd);
	if (hSrch == INVALID_HANDLE_VALUE)	return -1;
	while (bResult) {
		wsprintf(temp, "%s", wfd.cFileName);
		strcpy(game.music.choice.song_image[idx++], strtok(temp, ".")); // 확장자는 떼고 저장
		bResult = FindNextFile(hSrch, &wfd);
	}
	FindClose(hSrch);

	return idx;
}*/

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

int EXC_SCORE(int note_count) {
	return (50 * note_count * (note_count + 1));
}

/*
boolean exist(char searchText[])
{
	for (int i = 0; i < 1024; i++) {
		if (!strcmp(game.music.choice.song_image[i], searchText))
			return true;
	}
	return false;
}*/