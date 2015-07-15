#include "Nooskewl_Engine/engine.h"
#include "Nooskewl_Engine/font.h"
#include "Nooskewl_Engine/image.h"
#include "Nooskewl_Engine/internal.h"
#include "Nooskewl_Engine/utf8.h"

using namespace Nooskewl_Engine;

Font::Font(std::string filename, int size, int actual_size) :
	size(size),
	actual_size(actual_size)
{
	filename = "fonts/" + filename;

	file = open_file(filename);

	font = TTF_OpenFontRW(file, true, size);

	if (font == 0) {
		SDL_RWclose(file);
		throw LoadError("TTF_OpenFontRW failed");
	}

	height = get_ascent();
}

Font::~Font()
{
	clear_cache();

	if (font) {
		TTF_CloseFont(font);
	}
}

void Font::clear_cache()
{
	std::map<Uint32, Image *>::iterator it;
	for  (it = glyphs.begin(); it != glyphs.end(); it++) {
		std::pair<int, Image *> p = *it;
		delete p.second;
	}
	glyphs.clear();
}

int Font::get_text_width(std::string text)
{
	cache_glyphs(text);

	int width = 0;
	int offset = 0;
	int ch;

	while ((ch = utf8_char_next(text, offset)) != 0) {
		Image *g = glyphs[ch];
		width += g->w;
	}

	return width;
}

int Font::get_height()
{
	return height;
}

int Font::get_ascent()
{
	return TTF_FontAscent(font);
}

int Font::get_descent()
{
	return TTF_FontDescent(font);
}

int Font::get_padding()
{
	return height - size;
}

void Font::enable_shadow(SDL_Colour shadow_colour, Shadow_Type shadow_type)
{
	this->shadow_colour = shadow_colour,
	this->shadow_type = shadow_type;
}

void Font::disable_shadow()
{
	this->shadow_type = NO_SHADOW;
}

void Font::draw(SDL_Colour colour, std::string text, Point<int> dest_position)
{
	cache_glyphs(text);

	Point<int> pos = dest_position;
	pos.y += (actual_size - size);

	int offset = 0;
	int ch;

	// Optionally draw a shadow
	if (shadow_type != NO_SHADOW) {
		noo.enable_depth_buffer(true);
		noo.clear_depth_buffer(1.0f);

		while ((ch = utf8_char_next(text, offset)) != 0) {
			Image *g = glyphs[ch];

			g->start();

			if (shadow_type == DROP_SHADOW) {
				g->draw_tinted(shadow_colour, pos+Point<int>(1, 0), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(0, 1), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+1, Image::FLIP_V);
			}
			else if (shadow_type == FULL_SHADOW) {
				g->draw_tinted(shadow_colour, pos+Point<int>(-1, -1), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(0, -1), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(1, -1), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(-1, 0), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(1, 0), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(-1, 1), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(0, 1), Image::FLIP_V);
				g->draw_tinted(shadow_colour, pos+Point<int>(1, 1), Image::FLIP_V);
			}
			g->end();

			pos.x += g->w;
		}

		noo.enable_depth_buffer(false);
	}

	pos.x = dest_position.x;
	offset = 0;

	while ((ch = utf8_char_next(text, offset)) != 0) {
		Image *g = glyphs[ch];

		/* Glyphs are rendered upside down, so we FLIP_V them rather than flip the memory which would be slow */

		g->start();
		g->draw_tinted(colour, pos, Image::FLIP_V);
		g->end();

		pos.x += g->w;
	}
}

int Font::draw_wrapped(SDL_Colour colour, std::string text, Point<int> dest_position, int w, int line_height, int max_lines, int started_time, int delay, bool dry_run, bool &full, int &num_lines, int &width)
{
printf("x=%d\n", dest_position.x);
	full = false;
	char buf[2] = { 0 };
	int curr_y = dest_position.y;
	bool done = false;
	int lines = 0;
	if (max_lines == -1) {
		max_lines = 1000000;
	}
	Uint32 elapsed;
	if (started_time < 0) {
		elapsed = 1000000;
	}
	else {
		elapsed = SDL_GetTicks() - started_time;
	}
	int chars_to_draw;
	if (delay == 0) {
		chars_to_draw = 1000000;
	}
	else {
		chars_to_draw = elapsed / delay;
	}
	int chars_drawn = 0;
	int max_width = 0;
	std::string p = text;
	int total_position = 0;
	while (done == false && lines < max_lines) {
		int count = 0;
		int max = 0;
		int this_w = 0;
		int chars_drawn_this_time = 0;
		Uint32 ch = utf8_char(p, count);
		while (ch) {
			cache_glyph(ch);
			Image *g = glyphs[ch];
			this_w += g->w;
			if (this_w >= w) {
				if (count == 0) {
					done = true;
				}
				else {
					if (this_w > w) {
						count--;
					}
				}
				break;
			}
			if (ch == ' ') {
				max = count;
			}
			count++;
			ch =  utf8_char(p, count);
			if (chars_drawn+count < chars_to_draw) {
				chars_drawn_this_time++;
			}
		}
		if (utf8_char(p, count) == 0) {
			max = count;
		}
		int old_max = max;
		max = MIN(chars_drawn_this_time, max);
		if (done == false) {
			std::string s = utf8_substr(p, 0, max);
			int line_w = get_text_width(s);
			if (line_w > max_width) {
				max_width = line_w;
			}
			if (dry_run == false) {
				draw(colour, s, Point<int>(dest_position.x, curr_y));
				printf("drew at %d\n", dest_position.x);
			}
			total_position += max;
			p = utf8_substr(text, total_position);
			Uint32 ch = utf8_char(p, 0);
			if (ch == ' ') {
				total_position++;
				p = utf8_substr(text, total_position);
			}
			chars_drawn = total_position;
			curr_y += line_height;
			if (max < old_max) {
				done = true;
			}
			else {
				lines++;
				if (lines >= max_lines) {
					full = true;
				}
			}
		}
		if (utf8_char(p, 0) == 0) {
			done = true;
			full = true;
		}
		if (chars_drawn >= chars_to_draw) {
			done = true;
		}
	}

	width = max_width;
	num_lines = lines;

	return chars_drawn;
}

void Font::cache_glyph(Uint32 ch)
{
	if (glyphs.find(ch) != glyphs.end()) {
		return;
	}

	std::string s = utf8_char_to_string(ch);
	SDL_Surface *surface = TTF_RenderUTF8_Solid(font, s.c_str(), noo.white);
	if (surface == 0) {
		errormsg("Error rendering glyph\n");
		return;
	}

	Image *g = new Image(surface);

	SDL_FreeSurface(surface);

	glyphs[ch] = g;
}

void Font::cache_glyphs(std::string text)
{
	int offset = 0;
	int ch;
	while ((ch = utf8_char_next(text, offset)) != 0) {
		cache_glyph(ch);
	}
}
