
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include <stdio.h>
#include "external/glcorearb.h"
#include "external/wglext.h"
#include <math.h>
#include "tklib.h"
#include "math.h"
#include "input.h"
#include "utils.h"
#include <intrin.h>
#define equals(...) = __VA_ARGS__
#include "shader_shared.h"
#include "config.h"
#include "main.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT assert
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_ASSERT assert
#include "external/stb_truetype.h"
#pragma warning(pop)

global int g_width = 0;
global int g_height = 0;
global s_v2 g_window_size = c_base_res;
global s_v2 g_window_center = g_window_size * 0.5f;
global s_sarray<s_transform, 131072> transforms;
global s_sarray<s_transform, 131072> particles;
global s_input g_input = zero;
global b8 playback = false;
global float total_time = 0;
global float delta = 0;
global s_game* game = null;
global s_v2 mouse = zero;
global s_texture g_texture = zero;
global s_carray<s_font, e_font_count> g_font_arr;
global s_carray<s_sarray<s_transform, 131072>, e_font_count> text_arr;
global s_sarray<s_str<600>, 1024> g_chat_messages;
global s_thread g_message_thread = zero;

#define m_gl_funcs \
	X(PFNGLBUFFERDATAPROC, glBufferData) \
	X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
	X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
	X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
	X(PFNGLGENBUFFERSPROC, glGenBuffers) \
	X(PFNGLBINDBUFFERPROC, glBindBuffer) \
	X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
	X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
	X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
	X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
	X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \
	X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
	X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
	X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
	X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
	X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
	X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT) \
	X(PFNWGLGETSWAPINTERVALEXTPROC, wglGetSwapIntervalEXT)

#define X(type, name) global type name = null;
	m_gl_funcs
#undef X

int main(int argc, char** argv)
{

	LARGE_INTEGER freq;
	LARGE_INTEGER start_cycles;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start_cycles);

	maybe_delete_file(c_chatters_file_path);
	maybe_delete_file(c_chat_messages_file_path);

	char* class_name = "practice_class";
	HINSTANCE instance = GetModuleHandle(null);

	stbi_set_flip_vertically_on_load(true);

	game = (s_game*)VirtualAlloc(null, sizeof(*game), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	game->reset_game = true;

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = null;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = null;

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		DUMMY START		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		WNDCLASSEX window_class = zero;
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_OWNDC;
		window_class.lpfnWndProc = DefWindowProc;
		window_class.lpszClassName = class_name;
		window_class.hInstance = instance;
		assert(RegisterClassEx(&window_class));

		HWND window = CreateWindowEx(
			0,
			class_name,
			"TKCHAT",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			null,
			null,
			instance,
			null
		);
		assert(window != INVALID_HANDLE_VALUE);

		HDC dc = GetDC(window);

		PIXELFORMATDESCRIPTOR pfd = zero;
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 24;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int format = ChoosePixelFormat(dc, &pfd);
		assert(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
		assert(SetPixelFormat(dc, format, &pfd));

		HGLRC glrc = wglCreateContext(dc);
		assert(wglMakeCurrent(dc, glrc));

		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load_gl_func("wglCreateContextAttribsARB");
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)load_gl_func("wglChoosePixelFormatARB");

		assert(wglMakeCurrent(null, null));
		assert(wglDeleteContext(glrc));
		assert(ReleaseDC(window, dc));
		assert(DestroyWindow(window));
		assert(UnregisterClass(class_name, instance));

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		DUMMY END		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	HDC dc = null;
	HWND window = null;
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		WINDOW START		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		WNDCLASSEX window_class = zero;
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		window_class.lpfnWndProc = window_proc;
		window_class.lpszClassName = class_name;
		window_class.hInstance = instance;
		window_class.hCursor = LoadCursor(null, IDC_ARROW);
		assert(RegisterClassEx(&window_class));

		window = CreateWindowEx(
			0,
			class_name,
			"TKCHAT",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, (int)c_base_res.x, (int)c_base_res.y,
			null,
			null,
			instance,
			null
		);
		assert(window != INVALID_HANDLE_VALUE);

		dc = GetDC(window);

		int pixel_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_SWAP_METHOD_ARB, WGL_SWAP_COPY_ARB,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0
		};

		PIXELFORMATDESCRIPTOR pfd = zero;
		pfd.nSize = sizeof(pfd);
		int format;
		u32 num_formats;
		assert(wglChoosePixelFormatARB(dc, pixel_attribs, null, 1, &format, &num_formats));
		assert(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
		SetPixelFormat(dc, format, &pfd);

		int gl_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
			0
		};
		HGLRC glrc = wglCreateContextAttribsARB(dc, null, gl_attribs);
		assert(wglMakeCurrent(dc, glrc));

		#define X(type, name) name = (type)load_gl_func(#name);
			m_gl_funcs
		#undef X

		glDebugMessageCallback(gl_debug_callback, null);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		WINDOW END		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


	g_font_arr[e_font_small] = load_font("assets/consola.ttf", 24);
	g_font_arr[e_font_medium] = load_font("assets/consola.ttf", 36);
	g_font_arr[e_font_big] = load_font("assets/consola.ttf", 72);

	g_texture = load_texture_from_file("assets/cakez.png", GL_NEAREST);
	g_texture.sub_size = v2(16, 16);

	u32 vao;
	u32 ssbo;
	u32 program;
	{
		u32 vertex = glCreateShader(GL_VERTEX_SHADER);
		u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
		char* header = "#version 430 core\n";
		char* vertex_src = read_file("shaders/vertex.vertex");
		char* fragment_src = read_file("shaders/fragment.fragment");
		char* vertex_src_arr[] = {header, read_file("src/shader_shared.h"), vertex_src};
		char* fragment_src_arr[] = {header, read_file("src/shader_shared.h"), fragment_src};
		glShaderSource(vertex, array_count(vertex_src_arr), vertex_src_arr, null);
		glShaderSource(fragment, array_count(fragment_src_arr), fragment_src_arr, null);
		glCompileShader(vertex);
		char buffer[1024] = zero;
		check_for_shader_errors(vertex, buffer);
		glCompileShader(fragment);
		check_for_shader_errors(fragment, buffer);
		program = glCreateProgram();
		glAttachShader(program, vertex);
		glAttachShader(program, fragment);
		glLinkProgram(program);
		glUseProgram(program);
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(transforms.elements), null, GL_DYNAMIC_DRAW);

	{
		STARTUPINFO startup_info = zero;
		PROCESS_INFORMATION process_info = zero;
		BOOL result = CreateProcessA(
			c_python_path,
			"python.exe src/get_chatters.py",
			null,
			null,
			FALSE,
			0,
			null,
			null,
			&startup_info,
			&process_info
		);
		assert(result);
	}

	{
		STARTUPINFO startup_info = zero;
		PROCESS_INFORMATION process_info = zero;
		BOOL result = CreateProcessA(
			c_python_path,
			"python.exe src/get_chat_messages.py",
			null,
			null,
			FALSE,
			CREATE_NEW_CONSOLE,
			null,
			null,
			&startup_info,
			&process_info
		);
		assert(result);

		make_process_close_when_app_closes(process_info.hProcess);
	}

	// @Fixme(tkap, 10/03/2023): Vsync / delta calc is broken somehow, maybe stream related? idk
	// if(wglSwapIntervalEXT)
	// {
	// 	wglSwapIntervalEXT(1);
	// }

	b8 quit = false;

	g_message_thread = zero;
	g_message_thread.init(read_chat_messages);

	// s_carray<float, 128> fps_arr;
	// int fps_index = 0;

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		frame start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	while(true)
	{

		u64 start_frame_cycles;
		{
			LARGE_INTEGER temp;
			QueryPerformanceCounter(&temp);
			start_frame_cycles = temp.QuadPart;
		}

		MSG windows_msg;
		while(PeekMessage(&windows_msg, null, 0, 0, PM_REMOVE) > 0)
		{
			if(windows_msg.message == WM_QUIT)
			{
				quit = true;
				break;
			}
			TranslateMessage(&windows_msg);
			DispatchMessage(&windows_msg);
		}
		if(quit) { break; }

		{
			POINT point = zero;
			GetCursorPos(&point);
			ScreenToClient(window, &point);
			mouse.x = clamp((float)point.x, 0.0f, g_window_size.x);
			mouse.y = clamp((float)point.y, 0.0f, g_window_size.y);
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update game start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{

			if(is_key_pressed(key_f1))
			{
				write_file("state", game, sizeof(*game));
			}

			if(is_key_pressed(key_f2))
			{
				s_game* temp_game = (s_game*)read_file("state");
				memcpy(game, temp_game, sizeof(*game));
			}

			if(game->reset_game)
			{
				memset(game, 0, sizeof(*game));
				game->rng.seed = (u32)__rdtsc();
			}

			{
				constexpr float speed_arr[] = {
					1, 2, 4, 0.0f, 0.1f, 0.5f,
				};
				if(is_key_pressed(key_add))
				{
					game->speed_index = circular_index(game->speed_index + 1, array_count(speed_arr));
				}
				if(is_key_pressed(key_subtract))
				{
					game->speed_index = circular_index(game->speed_index - 1, array_count(speed_arr));
				}
				delta *= speed_arr[game->speed_index];
			}

			if(is_key_pressed(key_r))
			{
				game->reset_game = true;
			}

			game->num_chatters_alive = get_num_chatters_alive();

			// draw_texture(g_window_size * 0.5f, g_window_size, v4(1), background, zero, background.size, {.texture_index = 1});
			// draw_text("lobsang", mouse, 1, false);

			b8 update_chatters = true;
			b8 draw_chatters = true;

			s_sarray<s_entity_and_skill, 1024> skills_to_use;

			game->spatial_thing = make_spatial_thing();

			if(g_chat_messages.count > 0)
			{
				g_message_thread.lock();
				foreach_raw(msg_i, msg, g_chat_messages)
				{
					s_str<64> user;
					s_str<600> user_msg;

					int colon_index = msg.find(":");
					assert(colon_index > 0);

					user.from_data(msg.data, colon_index);
					user_msg.from_data(&msg.data[colon_index + 1], (int)strlen(msg.data) - colon_index - 1);

					if(user_msg.data[0] == '!' && strlen(user_msg.data) > 1)
					{
						for(int skill_i = 0; skill_i < e_skill_count; skill_i++)
						{
							if(strcmp(&user_msg.data[1], g_skill_data[skill_i].trigger_word) == 0)
							{
								s_entity_and_skill s = zero;

								foreach(entity_i, entity, game->entity_arr)
								{
									if(entity->type == e_entity_type_chatter)
									{
										if(strcmp(user.data, entity->name) == 0)
										{
											s.entity_index = entity_i;
											break;
										}
									}
								}

								s.skill = (e_skill)skill_i;
								skills_to_use.add(s);
								break;
							}
						}
					}
				}
				g_chat_messages.count = 0;
				g_message_thread.unlock();
			}

			switch(game->state)
			{
				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		waiting for chatters start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				case e_game_state_waiting_for_chatters:
				{
					update_chatters = false;
					draw_chatters = false;

					char* data = read_file(c_chatters_file_path);

					if(data)
					{
						int start = 0;
						int current = start;

						while(true)
						{
							char c = data[current];
							if(c == 0) { break; }
							else if(c == '\n')
							{
								s_entity new_chatter = zero;
								new_chatter.type = e_entity_type_chatter;
								new_chatter.dir.x = game->rng.chance100(50) ? 1.0f : -1.0f;
								new_chatter.sprite_index.x = game->rng.rand_range_ii(0, 2);
								new_chatter.sprite_index.y = game->rng.rand_range_ii(0, 1);

								// @Note(tkap, 10/03/2023): Minus 1 because python writes \r\n instead of just \n. We don't want that \r
								memcpy(new_chatter.name, &data[start], current - start - 1);

								int attempts = 0;
								while(true)
								{
									b8 fits = true;
									new_chatter.pos = random_point_in_rect(g_window_size - v2(c_chatter_size.x * 2, c_chatter_size.y * 2), &game->rng);
									new_chatter.pos.x += c_chatter_size.x;
									new_chatter.pos.y += c_chatter_size.y;
									foreach_raw(entity_i, entity, game->entity_arr)
									{
										assert(entity.type == e_entity_type_chatter);
										if(v2_distance(new_chatter.pos, entity.pos) < 200)
										{
											fits = false;
											break;
										}
									}
									attempts++;
									if(fits || attempts >= 1000) { break; }
								}

								game->entity_arr.add(new_chatter);
								start = current + 1;
							}

							current++;
						}

						game->state = e_game_state_chatter_introductions;
						game->introduction_arr[0].started = true;
					}
				} break;
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		waiting for chatters end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		introductions start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				case e_game_state_chatter_introductions:
				{
					update_chatters = false;
					draw_chatters = false;

					float introduction_time = 5.0f / game->entity_arr.count;
					b8 all_done = true;
					foreach(entity_i, entity, game->entity_arr)
					{
						if(game->introduction_arr[entity_i].started)
						{
							float size = range_lerp(game->introduction_arr[entity_i].time, 0, introduction_time, 512, 64);
							bool flip_x = entity->dir.x < 0;
							draw_chatter(*entity, v2(size), flip_x);

							game->introduction_arr[entity_i].time = at_most(introduction_time, game->introduction_arr[entity_i].time + delta);
							if(game->introduction_arr[entity_i].time >= introduction_time)
							{
								if(!game->entity_arr.is_last(entity_i))
								{
									game->introduction_arr[entity_i + 1].started = true;
								}
							}
							else
							{
								all_done = false;
							}
						}
					}
					if(all_done)
					{
						game->state = e_game_state_playing;
					}
				} break;
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		introductions end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		playing start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				case e_game_state_playing:
				{

					foreach_raw(to_use_i, to_use, skills_to_use)
					{
						e_skill skill = to_use.skill;
						s_entity* entity = &game->entity_arr[to_use.entity_index];
						if(!entity->dead && entity->cooldown_arr[skill] <= 0)
						{
							entity->cooldown_arr[skill] = g_skill_data[skill].cooldown;
							g_skill_data[skill].action(to_use.entity_index);
						}
					}

					int chatters_alive = 0;
					s_entity* first_alive = null;
					foreach(entity_i, entity, game->entity_arr)
					{
						if(entity->type == e_entity_type_chatter && !entity->dead)
						{
							chatters_alive += 1;
							first_alive = entity;
						}
					}

					if(chatters_alive == 1)
					{
						assert(first_alive);
						game->winner = first_alive;
						game->state = e_game_state_win;
					}

				} break;
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		playing end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		win start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
				case e_game_state_win:
				{
					char* text = format_text("%s wins!", game->winner->name);
					draw_text(text, g_window_center - v2(0.0f, g_font_arr[e_font_big].size / 2), v4(1), e_font_big, true);
					game->win_time += delta;
					if(game->win_time >= 7)
					{
						game->reset_game = true;
					}
				} break;
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		win end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

				invalid_default_case;
			}

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update entities start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			foreach(entity_i, entity, game->entity_arr)
			{
				if(entity->dead) { continue; }
				entity->time_alive += delta;
				switch(entity->type)
				{

					// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update chatters start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
					case e_entity_type_chatter:
					{
						if(update_chatters)
						{
							for(int skill_i = 0; skill_i < e_skill_count; skill_i++)
							{
								entity->cooldown_arr[skill_i] -= delta;
							}

							s_v2 dir = v2_normalized(entity->dir);
							entity->pos += dir * delta * 400;
							entity->mix_weight = at_least(0.0f, entity->mix_weight - delta * 4);

							b8 bounced;
							entity->dir = maybe_bounce_off_of_window(entity->pos, entity->dir, &bounced);
							if(bounced)
							{
								entity->mix_weight = 1;
							}

							entity->shoot_timer = at_most(c_shoot_delay, entity->shoot_timer + delta);
							if(entity->shoot_timer >= c_shoot_delay)
							{
								float shortest_distance = 9999999;
								s_entity* closest_entity = null;
								foreach(other_i, other, game->entity_arr)
								{
									if(other_i == entity_i) { continue; }
									if(other->type != e_entity_type_chatter) { continue; }
									if(other->dead) { continue; }

									float distance = v2_distance_squared(entity->pos, other->pos);
									if(distance < shortest_distance)
									{
										shortest_distance = distance;
										closest_entity = other;
									}
								}

								if(closest_entity)
								{
									entity->shoot_timer -= c_shoot_delay;
									s_v2 shoot_dir = v2_normalized(closest_entity->pos - entity->pos);

									s_entity proj = zero;
									proj.owner = entity_i;
									proj.type = e_entity_type_projectile;
									proj.damage = c_projectile_damage;
									proj.pos = entity->pos;
									proj.dir = shoot_dir;
									game->entity_arr.add(proj);
								}
							}

							if(entity->shield_active)
							{
								entity->shield_timer += delta;
								if(entity->shield_timer >= c_shield_duration)
								{
									entity->shield_active = false;
								}
							}
						}

						if(draw_chatters)
						{
							bool flip_x = entity->dir.x < 0;
							draw_chatter(*entity, c_chatter_size, flip_x);

							if(entity->shield_active)
							{
								spawn_particles(
									10,
									{
										.color_rand = 0.5f,
										.speed_rand = 0.5f,
										.dir_rand = 1,
										.dir = maybe(v2(1, 0)),
									},
									{
										.duration = 1.0f,
										.start_speed = 200,
										.end_speed = maybe(0.0f),
										.start_size = v2(16),
										.end_size = maybe(v2(0)),
										.start_color = v4(0.25f, 0.25f, 0.1f),
										.end_color = maybe(v4(0)),
										.target_chatter = entity,
									}
								);
							}

							if(entity->damage_taken > 0 && game->num_chatters_alive < c_chatters_alive_threshold)
							{
								int curr_health = c_max_health - entity->damage_taken;

								float percent = curr_health / (float)c_max_health;

								// @Note(tkap, 07/03/2023): Under
								constexpr s_v2 bar_size = v2(128, 16);
								draw_rect(entity->pos - v2(bar_size.x / 2, bar_size.y * 3), bar_size, v4(0.25f, 0.0f, 0.0f), {.origin_offset = c_origin_topleft});

								// @Note(tkap, 07/03/2023): Over
								draw_rect(entity->pos - v2(bar_size.x / 2, bar_size.y * 3), bar_size * v2(percent, 1.0f), v4(1, 0, 0), {.origin_offset = c_origin_topleft});
							}
						}

					} break;
					// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update chatters end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


					// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update projectiles start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
					case e_entity_type_projectile:
					{

						b8 expired = entity->time_alive >= 2;

						auto cell_arr = query_spatial_thing(entity->pos);
						foreach_raw(cell_i, cell, cell_arr)
						{
							for(int index_i = 0; index_i < cell->count; index_i++)
							{
								int other_i = cell->entity_index_arr[index_i];
								s_entity* other = &game->entity_arr[other_i];
								assert(other->type == e_entity_type_chatter);
								if(other_i == entity->owner) { continue; }
								if(other->dead) { continue; }

								if(rect_collides_rect(entity->pos, c_projectile_size, other->pos, c_chatter_size))
								{
									expired = true;
									if(!other->shield_active)
									{
										other->damage_taken += entity->damage;
									}
									other->mix_weight = 1;
									if(other->damage_taken >= c_max_health)
									{
										other->dead = true;
										remove_projectiles_of_chatter(other_i);

										// @Note(tkap, 07/03/2023): Death particles
										spawn_particles(
											100,
											{
												.color_rand = 0.5f,
												.speed_rand = 0.5f,
												.dir_rand = 1,
												.dir = maybe(v2(1, 0)),
											},
											{
												.duration = 1.0f,
												.start_speed = 300,
												.pos = other->pos,
												.start_size = v2(16),
												.end_size = maybe(v2(0)),
												.start_color = v4(1.0f, 0.1f, 0.1f),
												.end_color = maybe(v4(0)),
											}
										);

										// @Note(tkap, 07/03/2023): Level up particles
										s_entity* owner = &game->entity_arr[entity->owner];
										add_timed_message(format_text("%s killed %s", owner->name, other->name));
										if(!owner->dead)
										{
											spawn_particles(
												100,
												{
													.color_rand = 0.5f,
													.speed_rand = 0.5f,
													.dir = maybe(v2(0, -1)),
													.spawn_rect = c_chatter_size,
												},
												{
													.duration = 0.5f,
													.start_speed = 100,
													.start_size = v2(16),
													.end_size = maybe(v2(0)),
													.start_color = v4(0.25f, 1.0f, 0.25f),
													.end_color = maybe(v4(0)),
													.target_chatter = owner,
												}
											);
										}
									}
									break;
								}
							}
							if(expired) { break; }
						}

						s_v2 dir = v2_normalized(entity->dir);
						entity->pos += entity->dir * delta * 800;
						// draw_texture(entity->pos, v2(32), v4(1), texture, v2(16, 16), v2(16));

						b8 bounced;
						entity->dir = maybe_bounce_off_of_window(entity->pos, entity->dir, &bounced);

						if(expired)
						{
							spawn_particles(
								50,
								{
									.color_rand = 0.33f,
									.speed_rand = 0.33f,
									.dir_rand = 1,
									.dir = maybe(v2(1, 0)),
								},
								{
									.duration = 0.66f,
									.start_speed = 100,
									.end_speed = maybe(-200.0f),
									.pos = entity->pos,
									.start_size = v2(16),
									.end_size = maybe(v2(0)),
									.start_color = rand_color(&game->rng),
									.end_color = maybe(v4(0)),
								}
							);
						}
						else if(bounced)
						{
							spawn_particles(
								10,
								{
									.speed_rand = 0.1f,
									.dir_rand = 1,
									.dir = maybe(v2(1, 0)),
								},
								{
									.duration = 0.5f,
									.start_speed = 100,
									.end_speed = maybe(0.0f),
									.pos = entity->pos,
									.start_size = v2(16),
									.end_size = maybe(v2(0)),
									.start_color = v4(1),
									.end_color = maybe(v4(0)),
								}
							);
						}
						else
						{
							spawn_particles(
								1,
								{
									.dir_rand = 1,
									.dir = maybe(v2(1, 0)),
								},
								{
									.duration = 0.1f,
									.start_speed = 50,
									.pos = entity->pos,
									.start_size = v2(16),
									.end_size = maybe(v2(0)),
									.start_color = rand_color(&game->rng),
									.end_color = maybe(v4(0)),
								}
							);
						}

						if(expired)
						{
							game->entity_arr.remove_and_swap(entity_i--);
						}

					} break;
					// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update projectiles end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update entities end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		remove dead entities start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				foreach_raw(entity_i, entity, game->entity_arr)
				{
					if(entity.type != e_entity_type_chatter && entity.dead)
					{
						game->entity_arr.remove_and_swap(entity_i--);
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		remove dead entities end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update timed messages start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				foreach(msg_i, msg, game->timed_message_arr)
				{
					float target_y = get_timed_message_target_y_based_on_array_index(msg_i);
					msg->y = lerp(msg->y, target_y, delta * 10);

					if(msg->y <= g_window_size.y)
					{
						s_v2 pos = v2(20.0f, msg->y);
						s_v4 color = v4(1);
						float percent_done = msg->time_passed / c_message_duration;
						color.w = lerp(0.5f, 0, powf(percent_done, 4));
						draw_text(msg->text.data, pos, color, e_font_small, false);
					}

					msg->time_passed += delta;
					if(msg->time_passed >= c_message_duration)
					{
						game->timed_message_arr.remove_and_shift(msg_i--);
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update timed messages end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		display skills start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{

				float longest_skill_name = 0;
				for(int skill_i = 0; skill_i < e_skill_count; skill_i++)
				{
					char* text = format_text("!%s", g_skill_data[skill_i].trigger_word);
					s_v2 size = get_text_size(text, e_font_small);
					longest_skill_name = max(longest_skill_name, size.x);
				}

				s_v4 color = hsv_to_rgb(sinf2(total_time * 0.5f) * 360, 1, 1);
				for(int skill_i = 0; skill_i < e_skill_count; skill_i++)
				{
					s_v2 pos = v2(
						g_window_size.x - 10 - longest_skill_name,
						20 + skill_i * g_font_arr[e_font_small].size
					);
					char* text = format_text("!%s", g_skill_data[skill_i].trigger_word);
					draw_text(text, pos, color, e_font_small, false);
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		display skills end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		particles start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				foreach(particle_i, particle, game->particle_arr)
				{
					float percent = particle->time / particle->duration;
					s_v2 size = lerp(particle->start_size, particle->end_size.value, percent);
					s_v4 color = lerp(particle->start_color, particle->end_color.value, percent);
					float speed = lerp(particle->start_speed, particle->end_speed.value, percent);
					s_v2 pos;
					if(particle->target_chatter && !particle->target_chatter->dead)
					{
						pos = particle->target_chatter->pos + particle->pos;
					}
					else
					{
						pos = particle->pos;
					}
					draw_particle(pos, size, color);
					particle->pos += particle->dir * speed * delta;
					particle->time += delta;
					if(particle->time >= particle->duration)
					{
						game->particle_arr.remove_and_swap(particle_i--);
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		particles end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		}

		for(int cell_i = 0; cell_i < game->spatial_thing.cell_arr.max_elements(); cell_i++)
		{
			s_cell* cell = &game->spatial_thing.cell_arr[cell_i];
			// int c_cell_size = roundfi(g_window_size.x / c_cells_right);
			// int x_index = cell_i % c_cells_right;
			// int y_index = cell_i / c_cells_right;
			// s_v2 pos = v2(x_index * c_cell_size, y_index * c_cell_size);
			// draw_rect(pos, v2(c_cell_size), ((x_index + y_index) % 2) == 0 ? v4(0.5f) : v4(0.7f));
			if(cell->entity_index_arr)
			{
				// char* text = format_text("%i", cell->count);
				// draw_text(text, pos, v4(1), e_font_small, false);
				free(cell->entity_index_arr);
			}
		}
		game->frame_data = zero;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update game end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^



		// {
		// 	fps_arr[fps_index] = 1 / delta;
		// 	fps_index = circular_index(fps_index + 1, fps_arr.max_elements());
		// 	float average_fps = 0;
		// 	for(int i = 0; i < fps_arr.max_elements(); i++)
		// 	{
		// 		average_fps += fps_arr[i];
		// 	}
		// 	average_fps /= fps_arr.max_elements();
		// 	draw_text(format_text("%f", average_fps), v2(100), font->size, false);
		// }

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		render start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glViewport(0, 0, g_width, g_height);

			int location = glGetUniformLocation(program, "window_size");
			glUniform2fv(location, 1, &g_window_size.x);

			glBindTexture(GL_TEXTURE_2D, g_texture.id);

			if(transforms.count > 0)
			{
				glDisable(GL_BLEND);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*transforms.elements) * transforms.count, transforms.elements);
				glDrawArraysInstanced(GL_TRIANGLES, 0, 6, transforms.count);
				transforms.count = 0;
			}

			if(particles.count > 0)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*particles.elements) * particles.count, particles.elements);
				glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particles.count);
				particles.count = 0;
			}

			for(int font_i = 0; font_i < e_font_count; font_i++)
			{
				if(text_arr[font_i].count > 0)
				{
					s_font* font = &g_font_arr[font_i];
					glBindTexture(GL_TEXTURE_2D, font->texture.id);
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*text_arr[font_i].elements) * text_arr[font_i].count, text_arr[font_i].elements);
					glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_arr[font_i].count);
					text_arr[font_i].count = 0;
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		render end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		for(int k_i = 0; k_i < c_max_keys; k_i++)
		{
			g_input.keys[k_i].count = 0;
		}

		SwapBuffers(dc);
		Sleep(10);

		{
			LARGE_INTEGER temp;
			QueryPerformanceCounter(&temp);
			total_time = (float)((temp.QuadPart - start_cycles.QuadPart) / (f64)freq.QuadPart);
		}

		delta = 0.01f;

		// {
		// 	LARGE_INTEGER temp;
		// 	QueryPerformanceCounter(&temp);
		// 	delta = (float)((temp.QuadPart - start_frame_cycles) / (f64)freq.QuadPart);
		// }
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		frame end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	return 0;
}

func PROC load_gl_func(char* name)
{
	PROC result = wglGetProcAddress(name);
	if(!result)
	{
		printf("Failed to load %s\n", name);
		assert(false);
	}
	return result;
}

LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch(msg)
	{

		case WM_CLOSE:
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		} break;

		case WM_SIZE:
		{
			g_width = LOWORD(lparam);
			g_height = HIWORD(lparam);
			g_window_size = v2(g_width, g_height);
			g_window_center = g_window_size * 0.5f;
		} break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			int key = (int)wparam;
			if(key < c_max_keys)
			{
				b8 is_down = !(bool)((HIWORD(lparam) >> 15) & 1);

				s_stored_input si = zero;
				si.key = key;
				si.is_down = is_down;
				apply_event_to_input(&g_input, si);
			}
		} break;

		default:
		{
			result = DefWindowProc(window, msg, wparam, lparam);
		}
	}

	return result;
}

void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if(severity >= GL_DEBUG_SEVERITY_HIGH)
	{
		printf("GL ERROR: %s\n", message);
		assert(false);
	}
}

func char* read_file(char* file_path)
{
	HANDLE file = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
	if(file == INVALID_HANDLE_VALUE) { return null; }

	DWORD file_size = GetFileSize(file, null);
	char* result = (char*)VirtualAlloc(null, file_size + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	DWORD bytes_read;
	ReadFile(file, result, file_size, &bytes_read, null);
	CloseHandle(file);
	result[bytes_read] = 0;
	return result;
}

func void write_file(char* file_path, void* data, int size)
{
	HANDLE file = CreateFile(file_path, GENERIC_WRITE, 0, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, null);
	DWORD bytes_written;
	WriteFile(file, data, size, &bytes_written, null);
	CloseHandle(file);
}

func void draw_rect(s_v2 pos, s_v2 size, s_v4 color, s_transform t)
{
	t.pos = pos;
	t.draw_size = size;
	t.color = color;
	t.uv_min = v2(0, 0);
	t.uv_max = v2(1, 1);
	t.mix_color = v4(1);
	transforms.add(t);
}

func void draw_particle(s_v2 pos, s_v2 size, s_v4 color)
{
	s_transform t = zero;
	t.pos = pos;
	t.draw_size = size;
	t.color = color;
	t.uv_min = v2(0, 0);
	t.uv_max = v2(1, 1);
	t.mix_color = v4(1);
	particles.add(t);
}

func void draw_texture(s_v2 pos, s_v2 size, s_v4 color, s_texture texture, s_v2 src1, s_v2 src2, s_transform t)
{
	t.use_texture = true;
	t.pos = pos;
	t.draw_size = size;
	t.color = color;
	t.texture_size = texture.size;
	t.uv_min = v2(
		src1.x / texture.size.x,
		(src1.y + src2.y) / texture.size.y
	);
	t.uv_max = v2(
		(src1.x + src2.x) / texture.size.x,
		src1.y / texture.size.y
	);
	transforms.add(t);
}

func b8 is_key_down(int key)
{
	assert(key < c_max_keys);
	return g_input.keys[key].is_down;
}

func b8 is_key_pressed(int key)
{
	assert(key < c_max_keys);
	return (g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

func void apply_event_to_input(s_input* in_input, s_stored_input event)
{
	in_input->keys[event.key].is_down = event.is_down;
	in_input->keys[event.key].count += 1;
}

func s_texture load_texture_from_file(char* path, u32 filtering)
{
	int width, height, num_channels;
	void* data = stbi_load(path, &width, &height, &num_channels, 4);
	s_texture texture = load_texture_from_data(data, width, height, filtering);
	return texture;
}

func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering)
{
	assert(data);
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

	s_texture texture = zero;
	texture.id = id;
	texture.size = v2(width, height);
	return texture;
}

func void maybe_delete_file(char* path)
{
	DeleteFile(path);
}

func b8 check_for_shader_errors(u32 id, char* out_error)
{
	int compile_success;
	char info_log[1024];
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);

	if(!compile_success)
	{
		glGetShaderInfoLog(id, 1024, null, info_log);
		printf("Failed to compile shader:\n%s", info_log);

		if(out_error)
		{
			strcpy(out_error, info_log);
		}

		return false;
	}
	return true;
}

func void spawn_particles(int count, s_particle_spawn_data data, s_particle in_p)
{
	int particles_available = game->particle_arr.max_elements() - game->particle_arr.count;
	if(particles_available <= 0) { return; }
	int to_spawn = min(count, particles_available);

	in_p.end_size.value = in_p.end_size.valid ? in_p.end_size.value : in_p.start_size;
	in_p.end_color.value = in_p.end_color.valid ? in_p.end_color.value : in_p.start_color;
	in_p.end_speed.value = in_p.end_speed.valid ? in_p.end_speed.value : in_p.start_speed;
	for(int i = 0; i < to_spawn; i++)
	{
		s_particle p = in_p;
		if(data.dir.valid)
		{
			float angle = v2_angle(data.dir.value);
			angle += tau * data.dir_rand * game->rng.randf32();
			p.dir = v2_from_angle(angle);
		}

		// @Note(tkap, 07/03/2023): Color
		{
			float r = 1.0f - (game->rng.randf32() * data.color_rand);
			p.start_color = multiply_rgb(p.start_color, r);
			p.end_color.value = multiply_rgb(p.end_color.value, r);
		}

		// @Note(tkap, 07/03/2023): Speed
		{
			float r = 1.0f - (game->rng.randf32() * data.speed_rand);
			p.start_speed *= r;
			p.end_speed.value *= r;
		}

		// @Note(tkap, 07/03/2023): Position
		{
			p.pos += random_point_in_rect(data.spawn_rect, &game->rng);
			p.pos -= data.spawn_rect * 0.5f;
		}

		game->particle_arr.add(p);
	}
}

func s_v2 maybe_bounce_off_of_window(s_v2 pos, s_v2 in_dir, b8* out_bounced)
{
	s_v2 dir = in_dir;
	if(out_bounced) { *out_bounced = false; }
	if(pos.x > g_window_size.x)
	{
		dir.x = -1;
		if(out_bounced) { *out_bounced = true; }
	}
	else if(pos.x < 0)
	{
		dir.x = 1;
		if(out_bounced) { *out_bounced = true; }
	}
	if(pos.y > g_window_size.y)
	{
		dir.y = -1;
		if(out_bounced) { *out_bounced = true; }
	}
	else if(pos.y < 0)
	{
		dir.y = 1;
		if(out_bounced) { *out_bounced = true; }
	}
	return dir;
}

func s_v2 get_text_size(char* text, e_font font_id)
{
	s_font* font = &g_font_arr[font_id];
	int len = (int)strlen(text);
	assert(len > 0);

	s_v2 size = zero;

	for(int char_i = 0; char_i < len; char_i++)
	{
		char c = text[char_i];
		s_glyph glyph = font->glyph_arr[c];
		if(char_i == len - 1)
		{
			size.x += glyph.width;
		}
		else
		{
			size.x += glyph.advance_width * font->scale;
		}
	}

	return size;
}

func void draw_text(char* text, s_v2 in_pos, s_v4 color, e_font font_id, b8 centered, s_transform t)
{
	s_font* font = &g_font_arr[font_id];

	int len = (int)strlen(text);
	assert(len > 0);
	s_v2 pos = in_pos;
	if(centered)
	{
		s_v2 size = get_text_size(text, font_id);
		pos.x -= size.x / 2;
	}
	pos.y += font->ascent * font->scale;
	for(int char_i = 0; char_i < len; char_i++)
	{
		char c = text[char_i];

		s_glyph glyph = font->glyph_arr[c];
		s_v2 glyph_pos = pos;
		glyph_pos.y -= glyph.y0 * font->scale;

		t.use_texture = true;
		t.pos = glyph_pos;
		t.draw_size = v2(glyph.width, glyph.height);
		t.color = color;
		// t.texture_size = texture.size;
		t.uv_min = glyph.uv_min;
		t.uv_max = glyph.uv_max;
		t.origin_offset = c_origin_bottomleft;
		text_arr[font_id].add(t);

		pos.x += glyph.advance_width * font->scale;

	}
}

func s_font load_font(char* path, float font_size)
{
	s_font font = zero;
	font.size = font_size;

	u8* file_data = (u8*)read_file(path);
	assert(file_data);

	stbtt_fontinfo info = zero;
	stbtt_InitFont(&info, file_data, 0);

	stbtt_GetFontVMetrics(&info, &font.ascent, &font.descent, &font.line_gap);

	font.scale = stbtt_ScaleForPixelHeight(&info, font_size);
	constexpr int max_chars = 128;
	s_sarray<u8*, max_chars> bitmap_arr;
	constexpr int padding = 10;
	int total_width = padding;
	int total_height = 0;
	for(int char_i = 0; char_i < max_chars; char_i++)
	{
		s_glyph glyph = zero;
		u8* bitmap = stbtt_GetCodepointBitmap(&info, 0, font.scale, char_i, &glyph.width, &glyph.height, 0, 0);
		stbtt_GetCodepointBox(&info, char_i, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
		stbtt_GetGlyphHMetrics(&info, char_i, &glyph.advance_width, null);

		total_width += glyph.width + padding;
		total_height = max(glyph.height + padding * 2, total_height);

		font.glyph_arr[char_i] = glyph;
		bitmap_arr.add(bitmap);
	}

	u8* gl_bitmap = (u8*)malloc(sizeof(u8) * 4 * total_width * total_height);

	int current_x = padding;
	for(int char_i = 0; char_i < max_chars; char_i++)
	{
		s_glyph* glyph = &font.glyph_arr[char_i];
		u8* bitmap = bitmap_arr[char_i];
		for(int y = 0; y < glyph->height; y++)
		{
			for(int x = 0; x < glyph->width; x++)
			{
				u8 src_pixel = bitmap[x + y * glyph->width];
				u8* dst_pixel = &gl_bitmap[((current_x + x) + (padding + y) * total_width) * 4];
				dst_pixel[0] = src_pixel;
				dst_pixel[1] = src_pixel;
				dst_pixel[2] = src_pixel;
				dst_pixel[3] = src_pixel;
			}
		}

		glyph->uv_min.x = current_x / (float)total_width;
		glyph->uv_max.x = (current_x + glyph->width) / (float)total_width;

		glyph->uv_min.y = padding / (float)total_height;
		glyph->uv_max.y = (padding + glyph->height) / (float)total_height;

		current_x += glyph->width + padding;
	}

	font.texture = load_texture_from_data(gl_bitmap, total_width, total_height, GL_LINEAR);

	foreach_raw(bitmap_i, bitmap, bitmap_arr)
	{
		stbtt_FreeBitmap(bitmap, null);
	}

	free(gl_bitmap);

	return font;
}

func void remove_projectiles_of_chatter(int index)
{
	foreach(entity_i, entity, game->entity_arr)
	{
		if(entity->type == e_entity_type_projectile)
		{
			if(entity->owner == index)
			{
				entity->dead = true;
			}
		}
	}
}

func void draw_chatter(s_entity entity, s_v2 size, b8 flip_x)
{
	draw_texture(
		entity.pos, size, v4(1), g_texture, v2(entity.sprite_index.x * 16, entity.sprite_index.y * 16), g_texture.sub_size,
		{.flip_x = flip_x, .mix_weight = entity.mix_weight}
	);

	if(game->num_chatters_alive < c_chatters_alive_threshold)
	{
		s_v2 name_pos = entity.pos;
		name_pos.y += c_chatter_size.y / 2;
		draw_text(entity.name, name_pos, v4(1), e_font_medium, true);
	}
}

DWORD WINAPI read_chat_messages(void* user_data)
{
	while(true)
	{
		char* data = read_file(c_chat_messages_file_path);
		if(data)
		{
			maybe_delete_file(c_chat_messages_file_path);
			int start = 0;
			int current = 0;
			while(true)
			{
				char c = data[current];
				if(c == 0) { break; }
				else if(c == '\n')
				{
					s_str<600> str;
					int char_count = current - start;
					assert(char_count > 0);
					// @Note(tkap, 10/03/2023): Minus 1 because python writes \r\n instead of just \n. We don't want that \r
					str.from_data(&data[start], char_count - 1);

					g_message_thread.lock();
					g_chat_messages.add(str);
					g_message_thread.unlock();
				}
				current += 1;
			}

		}
		Sleep(10);
	}
}

func m_skill_func(nova_skill)
{
	s_entity entity = game->entity_arr[entity_index];
	add_timed_message(format_text("%s used %s", entity.name, g_skill_data[e_skill_nova].display_name));

	for(int proj_i = 0; proj_i < c_nova_projectile_count; proj_i++)
	{
		s_entity p = zero;
		p.pos = entity.pos;
		p.owner = entity_index;
		p.type = e_entity_type_projectile;
		p.damage = c_nova_damage;
		float angle = (float)proj_i / (c_nova_projectile_count - 1) * tau;
		p.dir = v2_from_angle(angle);
		game->entity_arr.add(p);
	}
}

func m_skill_func(shield_skill)
{
	s_entity* entity = &game->entity_arr[entity_index];
	add_timed_message(format_text("%s used %s", entity->name, g_skill_data[e_skill_shield].display_name));
	entity->shield_active = true;
	entity->shield_timer = 0;
}

func void add_timed_message(char* text)
{
	s_timed_message m = zero;
	m.text.from_cstr(text);
	m.y = get_timed_message_target_y_based_on_array_index(game->timed_message_arr.count);
	game->timed_message_arr.maybe_add(m);
}

func float get_timed_message_target_y_based_on_array_index(int index)
{
	float target_y = 20 + index * g_font_arr[e_font_small].size;
	return target_y;
}

func m_skill_func(move_left_skill)
{
	s_entity* entity = &game->entity_arr[entity_index];
	entity->dir.x = -1;
}

func m_skill_func(move_right_skill)
{
	s_entity* entity = &game->entity_arr[entity_index];
	entity->dir.x = 1;
}

func m_skill_func(move_up_skill)
{
	s_entity* entity = &game->entity_arr[entity_index];
	entity->dir.y = -1;
}

func m_skill_func(move_down_skill)
{
	s_entity* entity = &game->entity_arr[entity_index];
	entity->dir.y = 1;
}

func int get_num_chatters_alive()
{
	int result = 0;
	foreach(entity_i, entity, game->entity_arr)
	{
		if(entity->type != e_entity_type_chatter) { break; }
		if(!entity->dead) { result++; }
	}
	return result;
}

func s_spatial_thing make_spatial_thing()
{
	s_spatial_thing bs = zero;
	foreach(entity_i, entity, game->entity_arr)
	{
		if(entity->type != e_entity_type_chatter) { break; }
		if(entity->dead) { continue; }

		int c_cell_size = roundfi(g_window_size.x / c_cells_right);
		int x_index = clamp(floorfi(entity->pos.x / c_cell_size), 0, c_cells_right - 1);
		int y_index = clamp(floorfi(entity->pos.y / c_cell_size), 0, c_cells_right - 1);
		int index = x_index + y_index * c_cells_right;

		s_cell* cell = &bs.cell_arr[index];
		if(cell->count == 0)
		{
			cell->capacity = 128;
			cell->entity_index_arr = (int*)malloc(sizeof(int) * cell->capacity);
		}
		else if(cell->count + 1 > cell->capacity)
		{
			cell->capacity *= 2;
			cell->entity_index_arr = (int*)realloc(cell->entity_index_arr, sizeof(int) * cell->capacity);
		}
		cell->entity_index_arr[cell->count] = entity_i;
		cell->count += 1;
	}
	return bs;
}

func s_sarray<s_cell*, 9> query_spatial_thing(s_v2 pos)
{
	s_sarray<s_cell*, 9> result;
	s_spatial_thing* st = &game->spatial_thing;

	s_v2i offsets[] = {
		v2i(-1, -1), v2i(0, -1), v2i(1, -1),
		v2i(-1, 0), v2i(0, 0), v2i(1, 0),
		v2i(-1, 1), v2i(0, 1), v2i(1, 1),
	};

	int c_cell_size = roundfi(g_window_size.x / c_cells_right);
	int x_index = floorfi(pos.x / c_cell_size);
	int y_index = floorfi(pos.y / c_cell_size);
	for(int offset_i = 0; offset_i < array_count(offsets); offset_i++)
	{
		s_v2i offset = offsets[offset_i];
		int x = x_index + offset.x;
		int y = y_index + offset.y;

		if(x < 0 || x >= c_cells_right) { continue; }
		if(y < 0 || y >= c_cells_right) { continue; }

		int index = x + y * c_cells_right;
		result.add(&st->cell_arr[index]);
	}
	return result;
}
