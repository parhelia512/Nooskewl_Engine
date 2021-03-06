#include "Nooskewl_Engine/engine.h"
#include "Nooskewl_Engine/error.h"
#include "Nooskewl_Engine/image.h"
#include "Nooskewl_Engine/internal.h"
#include "Nooskewl_Engine/map.h"
#include "Nooskewl_Engine/map_entity.h"
#include "Nooskewl_Engine/shader.h"
#include "Nooskewl_Engine/tilemap.h"

using namespace Nooskewl_Engine;

Tilemap::Tilemap(std::string map_filename) :
	lighting_enabled(false)
{
	map_filename = "maps/" + map_filename;

	for (int i = 0; i < 256; i++) {
		std::string filename = std::string("tiles/tiles" + itos(i) + ".tga");
		Image *image;
		try {
			image = new Image(filename, true);
		}
		catch (Error e) {
			if (i == 0) {
				throw LoadError("no tile sheets!");
			}
			else {
				break;
			}
		}
		sheets.push_back(image);
	}

	SDL_RWops *f;
	try {
		f = open_file(map_filename);
	}
	catch (Error e) {
		for (size_t i = 0; i < sheets.size(); i++) {
			delete sheets[i];
		}
		sheets.clear();
		throw e;
	}

	size.w = SDL_ReadLE16(f);
	size.h = SDL_ReadLE16(f);
	num_layers = SDL_fgetc(f);

	layers = new Layer[num_layers];

	for (int layer = 0; layer < num_layers; layer++) {
		layers[layer].groups = std::vector<Group *>();
		layers[layer].sheets_used = std::vector<int>();
		layers[layer].sheet = new int *[size.h];
		layers[layer].x = new int *[size.h];
		layers[layer].y = new int *[size.h];
		layers[layer].solid = new bool *[size.h];
		for (int row = 0; row < size.h; row++) {
			layers[layer].sheet[row] = new int[size.w];
			layers[layer].x[row] = new int[size.w];
			layers[layer].y[row] = new int[size.w];
			layers[layer].solid[row] = new bool[size.w];
			for (int col = 0; col < size.w; col++) {
				layers[layer].x[row][col] = (char)SDL_fgetc(f);
				layers[layer].y[row][col] = (char)SDL_fgetc(f);
				layers[layer].sheet[row][col] = (char)SDL_fgetc(f);
				layers[layer].solid[row][col] = SDL_fgetc(f) != 0;
				if (layers[layer].x[row][col] >= 0 && std::find(layers[layer].sheets_used.begin(), layers[layer].sheets_used.end(), layers[layer].sheet[row][col]) == layers[layer].sheets_used.end()) {
					layers[layer].sheets_used.push_back(layers[layer].sheet[row][col]);
				}
			}
		}
	}

	int num_groups = SDL_ReadLE16(f);

	for (int i = 0; i < num_groups; i++) {
		Group *g = new Group;
		g->type = SDL_ReadLE32(f);
		g->layer = SDL_fgetc(f);
		g->position.x = SDL_ReadLE16(f);
		g->position.y = SDL_ReadLE16(f);
		g->size.w = SDL_ReadLE16(f);
		g->size.h = SDL_ReadLE16(f);
		layers[g->layer].groups.push_back(g);
	}

	int num_walls = SDL_ReadLE16(f);

	for (int i = 0; i < num_walls; i++) {
		Wall *w = new Wall;

		w->position.x = SDL_ReadLE16(f);
		w->position.y = SDL_ReadLE16(f);
		w->position.z = SDL_ReadLE16(f);

		w->size.x = SDL_ReadLE16(f);
		w->size.y = SDL_ReadLE16(f);
		w->size.z = SDL_ReadLE16(f);

		walls.push_back(w);
	}

	SDL_RWclose(f);

	for (int layer = 0; layer < num_layers; layer++) {
		std::sort(layers[layer].sheets_used.begin(), layers[layer].sheets_used.end());
	}

	Day_Night_Portion p;
	p.colour = noo.colours[15];
	p.percent = 30;
	day_night_splits.push_back(p);
	p.colour = noo.colours[7];
	p.percent = 4;
	day_night_splits.push_back(p);
	p.colour = noo.colours[6];
	p.percent = 4;
	day_night_splits.push_back(p);
	p.colour = noo.colours[5];
	p.percent = 25;
	day_night_splits.push_back(p);
	p.colour = noo.colours[5];
	p.percent = 25;
	day_night_splits.push_back(p);
	p.colour = noo.colours[32];
	p.percent = 4;
	day_night_splits.push_back(p);
	p.colour = noo.colours[24];
	p.percent = 4;
	day_night_splits.push_back(p);
	p.colour = noo.colours[22];
	p.percent = 4;
	day_night_splits.push_back(p);
}

Tilemap::~Tilemap()
{
	for (size_t i = 0; i < sheets.size(); i++) {
		delete sheets[i];
	}

	if (layers) {
		for (int layer = 0; layer < num_layers; layer++) {
			for (int row = 0; row < size.h; row++) {
				delete[] layers[layer].sheet[row];
				delete[] layers[layer].x[row];
				delete[] layers[layer].y[row];
				delete[] layers[layer].solid[row];
			}
			delete[] layers[layer].sheet;
			delete[] layers[layer].x;
			delete[] layers[layer].y;
			delete[] layers[layer].solid;
			for (size_t i = 0; i < layers[layer].groups.size(); i++) {
				delete layers[layer].groups[i];
			}
		}

		delete[] layers;
	}
}

int Tilemap::get_num_layers()
{
	return num_layers;
}

Size<int> Tilemap::get_size()
{
	return size;
}

bool Tilemap::is_solid(int layer, Point<int> position)
{
	if (position.x < 0 || position.y < 0 || position.x >= size.w || position.y >= size.h) {
		return true;
	}

	int start_layer = layer < 0 ? 0 : layer;
	int end_layer = layer < 0 ? num_layers - 1 : layer;

	for (int i = start_layer; i <= end_layer; i++) {
		Layer l = layers[i];
		if (l.solid[position.y][position.x]) {
			return true;
		}
	}

	return false;
}

bool Tilemap::collides(int layer, Point<int> topleft, Point<int> bottomright)
{
	int start_layer = layer < 0 ? 0 : layer;
	int end_layer = layer < 0 ? num_layers - 1 : layer;

	int start_column = topleft.x / noo.tile_size;
	int end_column = bottomright.x / noo.tile_size;
	int start_row = topleft.y / noo.tile_size;
	int end_row = bottomright.y / noo.tile_size;

	start_column = MIN(size.w-1, MAX(0, start_column));
	end_column = MIN(size.w-1, MAX(0, end_column));
	start_row = MIN(size.h-1, MAX(0, start_row));
	end_row = MIN(size.h-1, MAX(0, end_row));

	for (int i = start_layer; i <= end_layer; i++) {
		Layer l = layers[i];

		for (int row = start_row; row <= end_row; row++) {
			for (int column = start_column; column <= end_column; column++) {
				if (l.solid[row][column]) {
					return true;
				}
			}
		}
	}

	return false;
}

void Tilemap::draw(int layer, Point<float> position, bool use_depth_buffer)
{
	Layer l = layers[layer];

	for (size_t sheet = 0; sheet < l.sheets_used.size(); sheet++) {
		int sheet_num = l.sheets_used[sheet];

		sheets[sheet_num]->start();

		for (int row = 0; row < size.h; row++) {
			for (int col = 0; col < size.w; col++) {
				int s = l.sheet[row][col];
				if (s == sheet_num) {
					int x = l.x[row][col];
					int y = l.y[row][col];
					int sx = x * noo.tile_size;
					int sy = y * noo.tile_size;
					float dx = position.x + col * noo.tile_size;
					float dy = position.y + row * noo.tile_size;
					int dw = noo.tile_size;
					int dh = noo.tile_size;

					// Clipping
					if (dx < -noo.tile_size || dy < -noo.tile_size || dx >= noo.screen_size.w+noo.tile_size || dy >= noo.screen_size.h+noo.tile_size) {
						continue;
					}

					SDL_Colour light;

					if (lighting_enabled) {
						get_tile_lighting(Point<int>(col, row), light);
					}
					else {
						light = noo.white;
					}

					sheets[s]->draw_region_tinted_z(
						light,
						Point<int>(sx, sy),
						Size<int>(dw, dh),
						Point<float>(dx, dy),
						use_depth_buffer ? get_z(layer, col, row) : 0.0f,
						0
					);
				}
			}
		}

		sheets[sheet_num]->end();
	}
}

std::vector<Tilemap::Group *> Tilemap::get_groups(int layer)
{
	int start_layer = layer < 0 ? 0 : layer;
	int end_layer = layer < 0 ? num_layers - 1 : layer;

	std::vector<Group *> g;

	for (int i = start_layer; i <= end_layer; i++) {
		Layer l = layers[i];

		g.insert(g.end(), l.groups.begin(), l.groups.end());
	}

	return g;
}

bool Tilemap::checkcoll_bresenhams_box(Point<float> a, Point<float> b, Point<float> topleft, Point<float> bottomright, Point<int> &result)
{
	int dx = b.x - a.x;
	int dy = b.y - a.y;

	float step_x, step_y;

	#define SIGN(n) (n < 0 ? -1 : (n > 0 ? 1 : 0))

	if (abs(dx) >= abs(dy)) {
		step_x = SIGN(dx);
		step_y = fabs((float)dy / dx) * SIGN(dy);
	}
	else {
		step_y = SIGN(dy);
		step_x = fabs((float)dx / dy) * SIGN(dx);
	}

	#undef SIGN

	a += 0.5f;

	while (true) {
		if  ((int)a.x >= (int)topleft.x && (int)a.y >= (int)topleft.y && (int)a.x < (int)bottomright.x && (int)a.y < (int)bottomright.y) {
			result.x = (int)a.x;
			result.y = (int)a.y;
			return true;
		}
		if ((int)a.x == (int)b.x && (int)a.y == (int)b.y) {
			break;
		}
		a.x += step_x;
		a.y += step_y;
	}

	return false;
}

bool Tilemap::checkcoll_line_wall(Point<float> tile_pos, Point<float> orig_tile_pos, Point<float> light_pos, float light_z, Tilemap::Wall *w, Tilemap::Wall *tile_wall, Tilemap::Wall *light_wall)
{
	bool is_face = tile_wall && (orig_tile_pos.y >= tile_wall->position.y + tile_wall->size.y - tile_wall->size.z);

	if (!is_face && tile_wall && light_z <= tile_wall->position.z + tile_wall->size.z) {
		return true;
	}

	if (w == tile_wall && tile_pos.y <= light_pos.y) {
		return false;
	}

	if (w == light_wall && light_pos.y == light_wall->position.y + light_wall->size.y - 1 && orig_tile_pos.y >= light_wall->position.y + light_wall->size.y - 1) {
		return false;
	}

	Point<int> topleft(w->position.x, w->position.y);
	Point<int> bottomright(w->position.x + w->size.x, w->position.y + w->size.y);

	if (light_pos.x >= topleft.x && light_pos.y >= topleft.y && light_pos.x <= (bottomright.x-1) && light_pos.y <= (bottomright.y-1) && tile_pos.y <= light_pos.y) {
		return true;
	}

	Point<int> result;

	if (checkcoll_bresenhams_box(light_pos, tile_pos, topleft, bottomright, result)) {
		Wall *result_wall = get_tile_wall(result);
		if (tile_pos.y < light_pos.y && tile_pos.y == result.y && abs(tile_pos.x - result.x) == 1 && tile_wall != 0 && result_wall != 0 && result_wall->position.y + result_wall->size.y <= tile_wall->position.y + tile_wall->size.y) {
			return false;
		}

		return true;
	}

	return false;
}

void Tilemap::get_tile_lighting(Point<int> tile_position, SDL_Colour &out)
{
	Point<int> orig_tile_pos = tile_position;

	int out_colour[3];
	SDL_Colour day_time_colour = get_day_time_colour();

	if (indoors) {
		out_colour[0] = ambient_light.r;
		out_colour[1] = ambient_light.g;
		out_colour[2] = ambient_light.b;
	}
	else {
		out_colour[0] = day_time_colour.r;
		out_colour[1] = day_time_colour.g;
		out_colour[2] = day_time_colour.b;
	}

	Wall *tile_wall = get_tile_wall(tile_position);

	if (tile_wall) {
		tile_position.y = tile_wall->position.y + tile_wall->size.y - 1;
	}

	std::vector<Map_Entity *> &entities = noo.map->get_entities();
	int num_lights = 1;

	for (size_t i = 0; i < entities.size(); i++) {
		Map_Entity *map_entity = entities[i];

		if (indoors && map_entity == noo.player) {
			continue;
		}

		Brain *brain = map_entity->get_brain();

		if (brain == 0) {
			continue;
		}

		Light_Brain *light_brain = dynamic_cast<Light_Brain *>(brain);

		if (light_brain == 0) {
			continue;
		}

		num_lights++;

		Vec3D<float> lposition = light_brain->get_position();
		SDL_Colour lcolour = light_brain->get_colour();
		float lreach = light_brain->get_reach();
		float lfalloff = light_brain->get_falloff();

		Point<float> light_pos(lposition.x, lposition.y);
		float light_z = lposition.z;
		Wall *light_wall = get_tile_wall(light_pos);

		bool hits_wall = false;

		for (size_t j = 0; j < walls.size(); j++) {
			Wall *w = walls[j];

			if (w->position.z + w->size.z < light_z) {
				continue;
			}

			if (checkcoll_line_wall(tile_position, orig_tile_pos, light_pos, light_z, w, tile_wall, light_wall)) {
				hits_wall = true;
				break;
			}
		}

		if (hits_wall) {
			continue;
		}

		float distance_light_to_tile = (Vec3D<float>(tile_position.x, tile_position.y, 0) - lposition).length();

		float mul;

		if (distance_light_to_tile <= lreach) {
			mul = 1.0f;
		}
		else if (distance_light_to_tile - lreach <= lfalloff) {
			mul = 1.0f - ((distance_light_to_tile - lreach) / lfalloff);
		}
		else {
			mul = 0.0f;
		}

		out_colour[0] += lcolour.r * mul;
		out_colour[1] += lcolour.g * mul;
		out_colour[2] += lcolour.b * mul;
	}

	out.r = MIN(255, out_colour[0]);
	out.g = MIN(255, out_colour[1]);
	out.b = MIN(255, out_colour[2]);
	out.a = 255;

	if (indoors) {
		float outdoor_percent = outdoor_effect / 100.0f;
		out.r *= 1.0f - outdoor_percent;
		out.g *= 1.0f - outdoor_percent;
		out.b *= 1.0f - outdoor_percent;
		out.r += outdoor_percent * day_time_colour.r;
		out.g += outdoor_percent * day_time_colour.g;
		out.b += outdoor_percent * day_time_colour.b;
	}
}

void Tilemap::enable_lighting(bool enabled)
{
	lighting_enabled = enabled;
}

void Tilemap::set_lighting_parameters(bool indoors, int outdoor_effect, SDL_Colour ambient_light)
{
	this->indoors = indoors;
	this->outdoor_effect = outdoor_effect;
	this->ambient_light = ambient_light;
}

float Tilemap::get_z(int layer, int x, int y)
{
	Layer l = layers[layer];
	for (size_t i = 0; i < l.groups.size(); i++) {
		Group *g = l.groups[i];
		if ((g->type & Group::GROUP_OBJECT) == 0) {
			continue;
		}
		if (x >= g->position.x && y >= g->position.y && x < (g->position.x+g->size.w) && y < (g->position.y+g->size.h)) {
			// We multiply by 0.01f so the map transition which is 3D keeps graphics on the same plane.
			// 0.01f is big enough that a 16 bit depth buffer still works and small enough it looks right
			return -(1.0f - ((float)((g->position.y + g->size.h - 1) * noo.tile_size) / (float)(size.h * noo.tile_size))) * 0.01f;
		}
	}
	return -(1.0f - ((float)(y * noo.tile_size) / (float)(size.h * noo.tile_size))) * 0.01f;
}

Tilemap::Wall *Tilemap::get_tile_wall(Point<int> tile_position)
{
	for (size_t i = 0; i < walls.size(); i++) {
		Wall *w = walls[i];
		Point<int> start(w->position.x, w->position.y - w->position.z - w->size.z);
		Point<int> end(start.x + w->size.x, w->position.y + w->size.y);
		if (tile_position.x >= start.x && tile_position.y >= start.y && tile_position.x < end.x && tile_position.y < end.y) {
			return w;
		}
	}

	return 0;
}

SDL_Colour Tilemap::get_day_time_colour()
{
	float day_time = noo.get_day_time();
	int day_time_i = day_time * 100;

	int split;
	int count = 0;

	for (split = 0; split < day_night_splits.size(); split++) {
		if (day_time_i <= count + day_night_splits[split].percent) {
			break;
		}
		count += day_night_splits[split].percent;
	}

	int remain = day_time_i - count;
	float p = (float)remain / day_night_splits[split].percent;
	float inv_p = 1.0f - p;

	SDL_Colour result;
	SDL_Colour a = day_night_splits[split].colour;
	SDL_Colour b = day_night_splits[split+1].colour;

	result.r = (a.r * inv_p) + (b.r * p);
	result.g = (a.g * inv_p) + (b.g * p);
	result.b = (a.b * inv_p) + (b.b * p);
	result.a = 255;

	return result;
}
