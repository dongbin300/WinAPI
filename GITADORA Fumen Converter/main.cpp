#include <stdio.h>
#include <string.h>

void PHRASE_NUMBER_CAL(int, int);
double abs_timing(double, int);
void CONVERT_TEXT(char*, int, int);
void ASSIGN_NOTE(FILE*);
double add_division_num(int);

int phrase_number[1500] = { 0, };

#define FPS 60

#define FILEN "44DM"
#define BPM 200
#define OFFSET 300

int main()
{
	PHRASE_NUMBER_CAL(OFFSET, BPM);
	CONVERT_TEXT(FILEN, BPM, OFFSET);
	char filename[32];
	sprintf(filename, ".\\text\\%s-t.txt", FILEN);
	FILE *fr = fopen(filename, "r");
	ASSIGN_NOTE(fr);
}

void PHRASE_NUMBER_CAL(int offset, int bpm) {
	const double n = 60 * FPS / bpm;
	phrase_number[0] = offset;
	for (int i = 1; i < 1500; i++) {
		phrase_number[i] = phrase_number[i - 1] + n;
	}
}

double abs_timing(double timing, int interval) {
	int i = 0;
	interval -= 1000;
	while (1) {
		if (timing < phrase_number[i]) {
			break;
		}
		i++;
	}
	return (phrase_number[i] + ((double)interval / 100.0)*(double)(phrase_number[i + 1] - phrase_number[i]));
}

void CONVERT_TEXT(char *file_name, int bpm, int offset) {
	char original_file_name[32], convert_file_name[32];
	sprintf(original_file_name, ".\\text\\%s.txt", file_name);
	sprintf(convert_file_name, ".\\text\\%s-t.txt", file_name);
	FILE *fr = fopen(original_file_name, "r"); // original
	FILE *fw = fopen(convert_file_name, "w"); // convert
	char ch;
	char note_str[20];
	int half_mode = 0;
	int interval = 0;
	double timing = offset;
	const double n = 60 * FPS / bpm;
	char t[10];
	int td;

	fscanf(fr, "v3");
	fscanf(fr, "%s", t);
	fscanf(fr, "%d-%d-%d", &td, &td, &td);
	while (fgetc(fr) != '*') {
		fseek(fr, -1, SEEK_CUR);
		while ((ch = fgetc(fr)) != '=') {
			if (ch == '[')			half_mode = 1;
			else if (ch == ']')		half_mode = 0;
			else {
				fseek(fr, -1, SEEK_CUR);
				fscanf(fr, "%s", note_str);

				if (interval > 1000) { // 노트가 절대 표시 수치면
					fprintf(fw, "%d %s\n", (int)abs_timing(timing, interval), note_str);
				}
				else { // 노트가 상대 표시 수치면 
					timing += ((double)n * add_division_num(interval) / (double)(half_mode + 1));
					if (!strcmp(note_str, "X")) {
						fseek(fr, +1, SEEK_CUR);
						continue;
					}
					fprintf(fw, "%d %s\n", (int)timing, note_str);
				}
				
			}
			fseek(fr, +1, SEEK_CUR);
		}
		fscanf(fr, "%d", &interval);
	}
	fclose(fw);
	fclose(fr);
}

void ASSIGN_NOTE(FILE *fr) {
	int timing;
	char note_str[10];
	int note_count[9] = { 0, };
	int all_note_count = 0;

	while (!feof(fr)) {
		fscanf(fr, "%d %s\n", &timing, note_str);
		for (int i = 0; i < strlen(note_str); i++) {
			switch (note_str[i]) {
			case 'C': note_count[0]++; break;
			case 'H': note_count[1]++; break;
			case 'L': note_count[2]++; break;
			case 'S': note_count[3]++; break;
			case 'T': note_count[4]++; break;
			case 'K': note_count[5]++; break;
			case 'M': note_count[6]++; break;
			case 'F': note_count[7]++; break;
			case 'R': note_count[8]++; break;
			case 'X': break;
			}
		}
	}
	for (int i = 0; i < 9; i++) {
		all_note_count += note_count[i];
	}
	FILE *fp = fopen(".\\text\\notecount.txt", "w");
	fprintf(fp, "%d ", all_note_count);
	for (int i = 0; i < 9; i++) {
		fprintf(fp, "%d ", note_count[i]);
	}
	fclose(fp);
}

double add_division_num(int adn)
{
	double a;
	if (adn % 10 == 0) {
		a = (double)adn / 10.0;
	}
	else {
		switch (adn) {
		case 5: a = 0.5; break;
		case 2: a = 0.25; break;
		case 7: a = 0.75; break;
		case 1: a = 0.125; break;
		case 3: a = 0.375; break;
		case 6: a = 0.625; break;
		case 8: a = 0.875; break;
		case 33: a = 0.3333; break;
		case 66: a = 0.6666; break;
		case 16: a = 0.1666; break;
		case 83: a = 0.8333; break;
		case 11: a = 0.0625; break;
		case 12: a = 0.0833; break;
		case 25: a = 0.2; break;
		case 17: a = 0.142857; break;
		}
	}
	return a;
}