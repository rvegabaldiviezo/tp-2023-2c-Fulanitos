#include <stdarg.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include "structures.h"
#include "custom_logs.h"

#define RECTANGLE_WIDTH 50

/*
void log_rect(t_log* logger, const char* title, const char* body, ...)
{
	va_list arguments;
	va_start(arguments, body);
	body = string_from_vformat(body, arguments);

	char* rectangle = string_new();
	int odd_align = (string_length(title) % 2 != 0) ? 1 : 0;
	char* top_left = string_repeat('-', (RECTANGLE_WIDTH - string_length(title)) / 2);
	char* top_right = string_repeat('-', (RECTANGLE_WIDTH - string_length(title)) / 2 + odd_align);
	string_append_with_format(&rectangle, "+%s%s%s+\n", top_left, title, top_right);
	free(top_left);
	free(top_right);

	int line_count = 1;
	for (int i = 0; i < strlen(body); i++) {
		char c = body[i];

		if (i == 0) 
		{
			string_append_with_format(&rectangle, "| %c", c);
			line_count++;
		} 
		else if (c == '\n') 
		{
			char* fill = string_repeat(' ', RECTANGLE_WIDTH - line_count >= 0 ? RECTANGLE_WIDTH - line_count : 0);
			string_append_with_format(&rectangle, "%s|\n| ", fill);
			line_count = 1;
			free(fill);
		}
		else if (i == strlen(body) - 1)
		{
			char* fill = string_repeat(' ', RECTANGLE_WIDTH - line_count - 1 >= 0 ? RECTANGLE_WIDTH - line_count - 1 : 0);
			string_append_with_format(&rectangle, "%c%s|\n", c, fill);
			line_count = 1;
			free(fill);
		}
		else 
		{
			string_append_with_format(&rectangle, "%c", c);
			line_count++;
		}
	}
	char* bottom = string_repeat('-', RECTANGLE_WIDTH);
	string_append_with_format(&rectangle, "+%s+\n", bottom);

	log_trace(logger, "\n%s", rectangle);
	free(rectangle);
}

void log_rectangle(t_log* logger, char border, char fill, align align, const char* message, ...)
{
	va_list arguments;
	va_start(arguments, message);
	char* msg = string_from_vformat(message, arguments);
	int msg_length = string_length(msg);

	if(msg_length > RECTANGLE_WIDTH) {
		log_trace(logger, "%c%s", border, msg);
		return;
	}

	int padding_left_count = 0;
	int padding_right_count = 0;

	int odd_align = (msg_length % 2 != 0) ? 1 : 0;

	switch(align) {
		case LEFT:
			padding_left_count = 0;
			padding_right_count = RECTANGLE_WIDTH - msg_length;
			break;
		case CENTER: 
			padding_left_count = (RECTANGLE_WIDTH - msg_length) / 2;
			padding_right_count = (RECTANGLE_WIDTH - msg_length) / 2 + odd_align;
			break;
		case RIGHT:
			padding_left_count = RECTANGLE_WIDTH - msg_length;
			padding_right_count = 0;
			break;
	}

	char* padding_left = string_repeat(fill, padding_left_count);
	char* padding_righ = string_repeat(fill, padding_right_count);

	log_trace(logger, "%c%s%s%s%c", border, padding_left, msg, padding_righ, border);

	va_end(arguments);
}

*/
