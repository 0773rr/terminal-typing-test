#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#define DIM "\033[2m"
#define RESET "\033[0m"
#define GREEN "\033[42m"
#define RED "\033[41m"

struct termios standard_terminal;

void enable_raw_mode(){

	struct termios program_terminal;

	tcgetattr(STDIN_FILENO, &standard_terminal);

	program_terminal = standard_terminal;

	program_terminal.c_lflag &= ~(ICANON | ECHO | ISIG);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &program_terminal);

}

void disable_raw_mode(){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &standard_terminal);
}

void output_text(char text[], int place_of_endline_spaces[]){

	struct winsize terminal_window_size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_window_size);
	int terminal_window_width = terminal_window_size.ws_col - 1;

	int new_line_start = 0;
	int last_space_on_line = 0;
	int line_amount = 0;

	printf("\r\n" DIM);

	while (1) {

		if (strlen(text + new_line_start) <= terminal_window_width) {
			printf("%s", text + new_line_start);
			break;
		}

		last_space_on_line = 0;

		for (int i = new_line_start; i - new_line_start < terminal_window_width; i++) {

			if (text[i] == '\0') {
				break;
			}
			else if (text[i] == ' ') {
				last_space_on_line = i;	
			}

		}

		printf("%.*s\n", last_space_on_line - new_line_start, text + new_line_start);

		place_of_endline_spaces[last_space_on_line] = last_space_on_line - new_line_start + 1;

		new_line_start = last_space_on_line + 1;

		line_amount++;

	}

	printf(RESET);
	if (line_amount > 0){
		printf("\033[%dA", line_amount);
	}
	printf("\r");

}

void input_text(char text[], int place_of_endline_spaces[]){

	int i = 0;
	int mistakes = 0;
	char input;

	struct timespec start, end;
	int time_started = 0;

	while (text[i] != '\0'){

		input = getchar();

		if (input == 3) {
			if (i == 0) {
				printf("\n%s", "WPM: 0.0 Accuracy: 0.0%");
				return;
			}
			break;
		}

		if (!time_started && input != 8 && input != 127) {
			time_started = 1;
			clock_gettime(CLOCK_MONOTONIC, &start);
		}

		if (input == 8 || input == 127){
			if (i > 0){
				i--;
				if (place_of_endline_spaces[i]) {
					printf("\033[A\033[%dG" DIM " " RESET "\b", place_of_endline_spaces[i]);	
				}
				else {
					printf("\b" DIM "%c" RESET "\b", text[i]);
				}
			}
		}
		else if (input == text[i]){
			printf(GREEN "%c" RESET, input);
			i++;
		}
		else{
			printf(RED "%c" RESET, text[i]);
			i++;
			mistakes++;
		}

		if (i > 0 && place_of_endline_spaces[i-1] && input != 8 && input != 127) {
			printf("\n");
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	double minutes = ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0) / 60.0;
	double wpm = (double)i / 5.0 / minutes;
	double accuracy = (double)(i - mistakes) / (double)i * 100.0;
	printf("\nwpm %.1f accuracy %.1f", wpm, accuracy);
}

int main(){

	FILE *file = fopen("paste_your_text_here.txt", "rb");

	if (file == NULL) {
		perror("Error opening the text file");
		return 1;
	}

	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	rewind(file);

	char *text = malloc(file_size + 1);
	size_t byte_amount = fread(text, 1, file_size, file);
	text[byte_amount] = '\0';
	if (byte_amount > 0 && text[byte_amount - 1] == '\n') {
		text[byte_amount - 1] = '\0';
	}

	fclose(file);

	int *place_of_endline_spaces = calloc(strlen(text), sizeof(int));

	output_text(text, place_of_endline_spaces);
	enable_raw_mode();
	input_text(text, place_of_endline_spaces);
	disable_raw_mode();

	free(text);
	free(place_of_endline_spaces);
	return 0;
}
