#ifndef IMAGE_H
#define IMAGE_H

#include "Nooskewl_Engine/main.h"
#include "Nooskewl_Engine/basic_types.h"

namespace Nooskewl_Engine {

class Shader;

class NOOSKEWL_ENGINE_EXPORT Image {
public:
	friend class NOOSKEWL_ENGINE_EXPORT Shader;
	friend class NOOSKEWL_ENGINE_EXPORT Vertex_Cache;

	enum Flags {
		FLIP_H = 1,
		FLIP_V = 2
	};

	static void release_all();
	static void reload_all();
	static int get_unfreed_count();
	static unsigned char *read_tga(std::string filename, Size<int> &out_size, SDL_Colour *out_palette = 0);

	static bool dumping_colours;
	static bool keep_data;
	static bool save_rle;
	static bool ignore_palette;

	std::string filename;
	Size<int> size;

	Image(std::string filename, bool is_absolute_path = false);
	Image(SDL_Surface *surface);
	Image(Size<int> size);
	~Image();

	void release();
	void reload();

	bool save(std::string filename);

	void start(bool repeat = false); // call before every group of draws of the same Image
	void end(); // call after every group of draws

	void stretch_region_tinted_repeat(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, Size<int> dest_size, int flags = 0);
	void stretch_region_tinted(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, Size<int> dest_size, int flags = 0);
	void stretch_region(Point<float> source_position, Size<int> source_size, Point<float> dest_position, Size<int> dest_size, int flags = 0);
	void draw_region_lit_z(SDL_Colour colours[4], Point<float> source_position, Size<int> source_size, Point<float> dest_position, float z, int flags = 0);
	void draw_region_tinted_z(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, float z, int flags = 0);
	void draw_region_tinted(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, int flags = 0);
	void draw_region_z(Point<float> source_position, Size<int> source_size, Point<float> dest_position, float z, int flags = 0);
	void draw_region(Point<float> source_position, Size<int> source_size, Point<float> dest_position, int flags = 0);
	void draw_z(Point<float> dest_position, float z, int flags = 0);
	void draw_tinted(SDL_Colour tint, Point<float> dest_position, int flags = 0);
	void draw(Point<float> dest_position, int flags = 0);

	// These ones call start/end automatically each time
	void stretch_region_tinted_repeat_single(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, Size<int> dest_size, int flags = 0);
	void stretch_region_tinted_single(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, Size<int> dest_size, int flags = 0);
	void stretch_region_single(Point<float> source_position, Size<int> source_size, Point<float> dest_position, Size<int> dest_size, int flags = 0);
	void draw_region_tinted_single(SDL_Colour tint, Point<float> source_position, Size<int> source_size, Point<float> dest_position, int flags = 0);
	void draw_region_tinted_z_single(SDL_Colour, Point<float> source_position, Size<int> source_size, Point<float> dest_position, float z, int flags = 0);
	void draw_region_z_single(Point<float> source_position, Size<int> source_size, Point<float> dest_position, float z, int flags = 0);
	void draw_region_single(Point<float> source_position, Size<int> source_size, Point<float> dest_position, int flags = 0);
	void draw_tinted_single(SDL_Colour, Point<float> dest_position, int flags = 0);
	void draw_z_single(Point<float> dest_position, float z, int flags = 0);
	void draw_tinted_single(Point<float> dest_position, int flags = 0);
	void draw_single(Point<float> dest_position, int flags = 0);

	void set_target();
	void release_target();

private:
	struct TGA_Header {
		char idlength;
		char colourmaptype;
		char datatypecode;
		short int colourmaporigin;
		short int colourmaplength;
		char colourmapdepth;
		short int x_origin;
		short int y_origin;
		short width;
		short height;
		char bitsperpixel;
		char imagedescriptor;
		SDL_Colour palette[256];
	};

	static void merge_bytes(unsigned char *pixel, unsigned char *p, int bytes, TGA_Header *header);

	unsigned char find_colour_in_palette(unsigned char *p);

	struct Internal {
		Internal(std::string filename, bool keep_data, bool support_render_to_texture = false);
		Internal(unsigned char *pixels, Size<int> size, bool support_render_to_texture = false);
		~Internal();

		void upload(unsigned char *pixels);

		void release();
		unsigned char *reload(bool keep_data);

		unsigned char *loaded_data;

		std::string filename;
		Size<int> size;
		int refcount;

		bool has_render_to_texture;

	#ifdef NOOSKEWL_ENGINE_WINDOWS
		LPDIRECT3DTEXTURE9 video_texture;
		LPDIRECT3DTEXTURE9 system_texture;
		IDirect3DSurface9 *render_target;
	#endif
		GLuint texture;
		GLuint fbo;
		GLuint depth_buffer;
	};

	static std::vector<Internal *> loaded_images;

	Internal *internal;
};

} // End namespace Nooskewl_Engine

#endif // IMAGE_H
