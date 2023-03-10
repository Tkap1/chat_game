

global constexpr int c_max_keys = 1024;
global constexpr int c_key_w = 'W';
global constexpr int c_key_a = 'A';
global constexpr int c_key_s = 'S';
global constexpr int c_key_d = 'D';
global constexpr int c_key_h = 'H';

typedef DWORD (*thread_proc)(void*);

struct s_thread
{
	HANDLE mutex;
	thread_proc target_func;

	void init(thread_proc in_func)
	{
		target_func = in_func;
		assert(target_func);

		mutex = CreateMutex(null, false, null);
		assert(mutex);

		CreateThread(null, 0, in_func, null, 0, null);
	}

	void lock()
	{
		assert(mutex);
		DWORD wait_result = WaitForSingleObject(mutex, INFINITE);

		if(wait_result == WAIT_OBJECT_0)
		{
			return;
		}
		invalid_else;
	}

	void unlock()
	{
		assert(mutex);
		if(!ReleaseMutex(mutex))
		{
			assert(false);
		}
	}

};

template <int N>
struct s_str
{
	char data[N];

	void from_data(char* src, int size_in_bytes)
	{
		memcpy(data, src, size_in_bytes);
		data[size_in_bytes] = 0;
	}

	void from_cstr(char* src)
	{
		from_data(src, (int)strlen(src));
	}

	int find(char* needle)
	{
		int haystack_len = (int)strlen(data);
		int needle_len = (int)strlen(needle);
		assert(haystack_len > 0);
		assert(needle_len > 0);
		if(needle_len > haystack_len) { return -1; }

		for(int i = 0; i < haystack_len - (needle_len - 1); i++)
		{
			b8 all_match = true;
			for(int j = 0; j < needle_len; j++)
			{
				char haystack_c = data[i + j];
				char needle_c = needle[j];

				if(haystack_c != needle_c)
				{
					all_match = false;
					break;
				}

			}
			if(all_match)
			{
				return i;
			}
		}
		return -1;
	}

};

struct s_key
{
	b8 is_down;
	int count;
};

struct s_stored_input
{
	b8 is_down;
	int key;
};

struct s_input
{
	s_key keys[c_max_keys];
};

enum e_game_state
{
	e_game_state_waiting_for_chatters,
	e_game_state_chatter_introductions,
	e_game_state_playing,
	e_game_state_win,
};

enum e_entity_type
{
	e_entity_type_chatter,
	e_entity_type_projectile,
};

enum e_font
{
	e_font_small,
	e_font_medium,
	e_font_big,
	e_font_count,
};

enum e_skill
{
	e_skill_nova,
	e_skill_shield,
	e_skill_move_left,
	e_skill_move_right,
	e_skill_move_up,
	e_skill_move_down,
	e_skill_count,
};

struct s_timed_message
{
	float time_passed;
	float y;
	s_str<128> text;
};

struct s_entity
{
	b8 dead;
	b8 shield_active;
	e_entity_type type;
	int owner;
	float time_alive;
	float mix_weight;
	float shoot_timer;
	float shield_timer;
	int damage_taken;
	int damage;
	s_v2 pos;
	s_v2 dir;
	s_v2i sprite_index;

	float cooldown_arr[e_skill_count];
	char name[64];
};


#define m_skill_func(name) void name(int entity_index)
typedef m_skill_func(t_skill_func);

func m_skill_func(nova_skill);
func m_skill_func(shield_skill);
func m_skill_func(move_left_skill);
func m_skill_func(move_right_skill);
func m_skill_func(move_up_skill);
func m_skill_func(move_down_skill);

struct s_skill_data
{
	float cooldown;
	char* display_name;
	char* trigger_word;
	t_skill_func* action;
};

global constexpr s_skill_data g_skill_data[e_skill_count] = {
	{
		.cooldown = c_nova_cooldown,
		.display_name = "Nova",
		.trigger_word = "nova",
		.action = &nova_skill,
	},
	{
		.cooldown = c_shield_cooldown,
		.display_name = "Shield",
		.trigger_word = "shield",
		.action = &shield_skill,
	},
	{
		.trigger_word = "a",
		.action = &move_left_skill,
	},
	{
		.trigger_word = "d",
		.action = &move_right_skill,
	},
	{
		.trigger_word = "w",
		.action = &move_up_skill,
	},
	{
		.trigger_word = "s",
		.action = &move_down_skill,
	},
};

struct s_entity_and_skill
{
	int entity_index;
	e_skill skill;
};

struct s_introduction
{
	b8 started;
	float time;
};

struct s_particle_spawn_data
{
	float color_rand;
	float speed_rand;
	float dir_rand;
	s_maybe<s_v2> dir;
	s_v2 spawn_rect;
};

struct s_particle
{
	float time;
	float duration;
	float start_speed;
	s_maybe<float> end_speed;
	s_v2 pos;
	s_v2 start_size;
	s_v2 dir;
	s_maybe<s_v2> end_size;
	s_v4 start_color;
	s_maybe<s_v4> end_color;
	s_entity* target_chatter;
};


struct s_glyph
{
	int advance_width;
	int width;
	int height;
	int x0;
	int y0;
	int x1;
	int y1;
	s_v2 uv_min;
	s_v2 uv_max;
};

struct s_texture
{
	u32 id;
	s_v2 size;
	s_v2 sub_size;
};

struct s_font
{
	float size;
	float scale;
	int ascent;
	int descent;
	int line_gap;
	s_texture texture;
	s_carray<s_glyph, 1024> glyph_arr;
};

#define zeroable_struct(...) union \
{ \
	struct \
	{ \
		__VA_ARGS__ \
	}; \
	struct \
	{ \
		__VA_ARGS__ \
	} frame_data; \
}

struct s_game
{
	b8 reset_game;
	e_game_state state;
	int speed_index;
	float win_time;
	s_random rng;
	s_entity* winner;
	s_sarray<s_timed_message, 128> timed_message_arr;
	s_sarray<s_particle, 131072> particle_arr;
	s_sarray<s_entity, 131072> entity_arr;
	s_carray<s_introduction, 131072> introduction_arr;

	zeroable_struct(
		int num_chatters_alive;
	);

};

struct s_framebuffer
{
	u32 id;
	u32 color;
};

func PROC load_gl_func(char* name);
LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
func char* read_file(char* file_path);
func void write_file(char* file_path, void* data, int size);
func void draw_rect(s_v2 pos, s_v2 size, s_v4 color, s_transform t = zero);
func void draw_texture(s_v2 pos, s_v2 size, s_v4 color, s_texture texture, s_v2 src1, s_v2 src2, s_transform t = zero);
func b8 is_key_down(int key);
func b8 is_key_pressed(int key);
func void apply_event_to_input(s_input* in_input, s_stored_input event);
func s_texture load_texture_from_file(char* path, u32 filtering);
func void maybe_delete_file(char* path);
func b8 check_for_shader_errors(u32 id, char* out_error);
func void draw_particle(s_v2 pos, s_v2 size, s_v4 color);
func void spawn_particles(int count, s_particle_spawn_data data, s_particle in_p);
func s_v2 maybe_bounce_off_of_window(s_v2 pos, s_v2 in_dir, b8* out_bounced);
func void draw_text(char* text, s_v2 in_pos, s_v4 color, e_font font_id, b8 centered, s_transform t = zero);
func s_font load_font(char* path, float font_size);
func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering);
func s_v2 get_text_size(char* text, e_font font_id);
func void remove_projectiles_of_chatter(int index);
func void draw_chatter(s_entity entity, s_v2 size, b8 flip_x);
DWORD WINAPI read_chat_messages(void* user_data);
func void add_timed_message(char* text);
func float get_timed_message_target_y_based_on_array_index(int index);
func int get_num_chatters_alive();
