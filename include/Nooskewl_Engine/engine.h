#ifndef ENGINE_H
#define ENGINE_H

#include "Nooskewl_Engine/main.h"
#include "Nooskewl_Engine/basic_types.h"

namespace Nooskewl_Engine {

class Brain;
class CPA;
class Font;
class GUI;
class Image;
class Map;
class Map_Entity;
class MML;
class Shader;
class Stats;
class Translation;
class XML;

class NOOSKEWL_ENGINE_EXPORT Engine {
public:
	static const Uint32 TICKS_PER_FRAME = (1000 / 60);

	static void wait_callback(void *data);

	/* Publicly accessible variables */
	// Audio
	bool mute;
	MML *music;
	MML *button_mml;
	MML *item_mml;
	MML *widget_mml;
	// Graphics
	std::string window_title; // set this first thing to change it
	float scale;
	float font_scale;
	bool use_hires_font;
	Size<int> real_screen_size;
	Size<int> screen_size;
	Point<int> screen_offset;
	int tile_size;
	bool fullscreen;
	bool opengl;
	SDL_Colour colours[256];
	SDL_Colour shadow_colour;
	SDL_Colour four_blacks[4];
	SDL_Colour four_whites[4];
	SDL_Colour black;
	SDL_Colour white;
	SDL_Colour magenta;
	Font *font;
	Image *window_image;
	Image *speech_window_image;
	Image *name_box_image_top;
	Image *name_box_image_bottom;
	Image *name_box_image_top_right;
	Image *name_box_image_bottom_right;
	Shader *current_shader;
	Shader *default_shader;
	Shader *brighten_shader;
#ifdef NOOSKEWL_ENGINE_WINDOWS
	IDirect3DDevice9 *d3d_device;
	bool d3d_lost;
	IDirect3DSurface9 *render_target;
#endif
	// Input
	int joy_b1;
	int joy_b2;
	int joy_b3;
	int joy_b4;
	int key_b1;
	int key_b2;
	int key_b3;
	int key_b4;
	// Other
	std::string language;
	Translation *t;
	Translation *game_t;
	CPA *cpa;
	Map *map;
	std::string last_map_name;
	Map_Entity *player;
	std::vector<Map_Entity *> party;
	std::vector<GUI *> guis;
	XML *miscellaneous_xml;
	Image *target_image;
	Image *work_image; // screen size
	bool is_waiting;

	Engine();
	~Engine();

	bool start(int argc, char **argv);
	void end();

	bool handle_event(SDL_Event *sdl_event, bool is_joystick_repeat = false);
	bool update();
	void draw();

	bool check_milestone(int number);
	bool check_milestone(std::string name);
	void set_milestone(int number, bool completed);
	int milestone_name_to_number(std::string name);
	std::string milestone_number_to_name(int number);
	int get_num_milestones();
	void clear_milestones();

	void clear(SDL_Colour colour);
	void flip();

	void clear_depth_buffer(float value);
	void enable_depth_buffer(bool enable);
	bool is_depth_buffer_enabled();

	void set_screen_size(int w, int h);
	void set_default_projection();
	void set_map_transition_projection(float angle);
	void set_matrices(glm::mat4 &model, glm::mat4 &view, glm::mat4 &proj);
	void update_projection();

#ifdef NOOSKEWL_ENGINE_WINDOWS
	void set_initial_d3d_state();
#endif

	void draw_line(SDL_Colour colour, Point<float> a, Point<float> b, float thickness = 1.0f);
	void draw_rectangle(SDL_Colour colour, Point<float> pos, Size<float> size, float thickness = 1.0f);
	void draw_triangle(SDL_Colour vertex_colours[3], Point<float> a, Point<float> b, Point<float> c);
	void draw_triangle(SDL_Colour colour, Point<float> a, Point<float> b, Point<float> c);
	void draw_quad(SDL_Colour vertex_colours[4], Point<float> dest_position, Size<float> dest_size);
	void draw_quad(SDL_Colour colour, Point<float> dest_position, Size<float> dest_size);
	void draw_9patch_tinted(SDL_Colour tint, Image *image, Point<int> dest_position, Size<int> dest_size);
	void draw_9patch(Image *image, Point<int> dest_position, Size<int> dest_size);
	void reset_fancy_draw();
	void fancy_draw(SDL_Colour colour, std::string text, Point<int> position);

	void load_palette(std::string name);
	std::string load_text(std::string filename);
	void play_music(std::string name);

	bool save_game(SDL_RWops *file);
	bool load_game(SDL_RWops *file, int *loaded_time);

	// For keeping track of time
	void new_game_started();
	void game_loaded(int loaded_time);
	void game_paused();
	void game_unpaused();
	int get_play_time();
	float get_day_time(); // 0.0f - 1.0f (midnight AM to midnight PM)

	bool save_map(Map *map, bool save_player);

	void add_notification(std::string text);

	void set_target_image(Image *image);
	void set_target_backbuffer();

	void recreate_work_image();

private:
	void init_video();
	void shutdown_video();
	void init_audio();
	void shutdown_audio();
	void load_fonts();
	void destroy_fonts();
	void check_joysticks();
	void set_mouse_cursor();
	void set_window_icon();
	void maybe_expand_milestones(int number);
	void load_milestones();
	void load_translation();
	void clear_buffers();
	void setup_default_shader();

	bool save_milestones(SDL_RWops *file);
	bool load_milestones(SDL_RWops *file, int version);
	Map *load_map(SDL_RWops *file, int version, bool load_player, int time);
	Map_Entity *load_entity(SDL_RWops *file, int version, int time);
	Brain *load_brain(SDL_RWops *file, int version);
	Stats *load_stats(SDL_RWops *file, int version);
	bool load_spells(SDL_RWops *file, Stats *stats, int version);
	bool load_inventory(SDL_RWops *file, Stats *stats, int version);

	SDL_Window *window;
	bool vsync;
	Uint32 last_frame;
	int accumulated_delay;

	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	SDL_GLContext opengl_context;

	SDL_Joystick *joy;
	int num_joysticks;

	SDL_AudioDeviceID audio_device;

#ifdef NOOSKEWL_ENGINE_WINDOWS
	HWND hwnd;
	D3DPRESENT_PARAMETERS d3d_pp;
	IDirect3D9 *d3d;
	HICON mouse_cursor;
#endif

#ifdef __linux__
	X11::Display *x_display;
	X11::Window x_window;
	X11::Cursor mouse_cursor;
#endif

	bool use_custom_cursor;

	std::string default_opengl_vertex_source;
	std::string default_opengl_fragment_source;
	std::string brighten_opengl_fragment_source;
	std::string default_d3d_vertex_source;
	std::string default_d3d_fragment_source;
	std::string brighten_d3d_fragment_source;
	std::string d3d_technique_source;

	bool *milestones;
	int num_milestones;
	std::map<int, std::string> ms_number_to_name;
	std::map<std::string, int> ms_name_to_number;

	bool depth_buffer_enabled;

	bool doing_map_transition;
	bool moved_player_during_map_transition;
	Uint32 map_transition_start;
	Map *old_map;
	std::string new_map_name;
	Point<int> new_map_position;
	Direction new_map_direction;
	static const int map_transition_duration = 500;

	Uint32 fancy_draw_start;

	int loaded_time;
	time_t session_start;
	bool paused;
	time_t pause_start;
	int paused_time;

	std::map<std::string, std::pair<int, std::string> > map_saves;

	int save_state_version;

	float escape_triangle_size;
	Point<int> mouse_pos;

	std::vector<std::string> notifications;
	Uint32 notification_start_time;

	bool fullscreen_window;

	struct Joy_Repeat {
		bool is_button;
		int button;
		int axis;
		int value;
		Uint32 initial_press_time;
		bool down;
	};
	int find_joy_repeat(bool is_button, int button_or_axis);
	std::vector<Joy_Repeat> joystick_repeats;

	const int story_start_hour; // time of day the game starts
	const int seconds_per_hour; // seconds of play time per game world hour
};

NOOSKEWL_ENGINE_EXPORT extern Engine noo;

} // End namespace Nooskewl_Engine

#endif // ENGINE_H
