
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef s8 b8;
typedef s32 b32;

#define func static
#define global static
#define local_persist static
#define zero {}
#define null NULL
#define array_count(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define invalid_default_case default: { assert(false); }
#define invalid_else else { assert(false); }

#ifdef m_debug
#define assert(cond) if(!(cond)) { on_failed_assert(#cond, __FILE__, __LINE__); }
#else
#define assert(cond)
#endif

global constexpr s64 c_max_s8 = INT8_MAX;
global constexpr s64 c_max_s16 = INT16_MAX;
global constexpr s64 c_max_s32 = INT32_MAX;
global constexpr s64 c_max_s64 = INT64_MAX;
global constexpr s64 c_max_u8 = UINT8_MAX;
global constexpr s64 c_max_u16 = UINT16_MAX;
global constexpr s64 c_max_u32 = UINT32_MAX;
global constexpr u64 c_max_u64 = UINT64_MAX;

global constexpr float tau = 6.2831853071f;

func void on_failed_assert(char* cond, char* file, int line);


struct s_v2
{
	float x;
	float y;
};


template <typename T>
constexpr func s_v2 v2(T x, T y)
{
	return {(float)x, (float)y};
}

template <typename T>
constexpr func s_v2 v2(T v)
{
	return {(float)v, (float)v};
}

func float v2_angle(s_v2 v)
{
	return atan2f(v.y, v.x);
}

struct s_v4
{
	float x;
	float y;
	float z;
	float w;
};

template <typename T>
func s_v4 v4(T v)
{
	return {(float)v, (float)v, (float)v, (float)v};
}

template <typename T>
func s_v4 v4(T x, T y, T z)
{
	return {(float)x, (float)y, (float)z, 1.0f};
}

template <typename T>
func s_v4 v4(T x, T y, T z, T w)
{
	return {(float)x, (float)y, (float)z, (float)w};
}


template <typename T>
struct s_maybe
{
	b8 valid;
	T value;
};

template <typename T>
func s_maybe<T> maybe(T value)
{
	s_maybe<T> maybe;
	maybe.valid = true;
	maybe.value = value;
	return maybe;
}

template <typename T>
func s_maybe<T> ignore(T value)
{
	s_maybe<T> maybe = zero;
	return maybe;
}

struct s_random
{
	u32 seed;

	u32 randu()
	{
		seed = seed * 2147001325 + 715136305;
		return 0x31415926 ^ ((seed >> 16) + (seed << 16));
	}

	f64 randf()
	{
		return (f64)randu() / (f64)4294967295;
	}

	float randf32()
	{
		return (float)randu() / (float)4294967295;
	}

	f64 randf2()
	{
		return randf() * 2 - 1;
	}

	u64 randu64()
	{
		return (u64)(randf() * (f64)c_max_u64);
	}


	// min inclusive, max inclusive
	int rand_range_ii(int min, int max)
	{
		if(min > max)
		{
			int temp = min;
			min = max;
			max = temp;
		}

		return min + (randu() % (max - min + 1));
	}

	// min inclusive, max exclusive
	int rand_range_ie(int min, int max)
	{
		if(min > max)
		{
			int temp = min;
			min = max;
			max = temp;
		}

		return min + (randu() % (max - min));
	}

	float randf_range(float min_val, float max_val)
	{
		if(min_val > max_val)
		{
			float temp = min_val;
			min_val = max_val;
			max_val = temp;
		}

		float r = (float)randf();
		return min_val + (max_val - min_val) * r;
	}

	b8 chance100(float chance)
	{
		assert(chance >= 0);
		assert(chance <= 100);
		return chance / 100 >= randf();
	}

	b8 chance1(float chance)
	{
		assert(chance >= 0);
		assert(chance <= 1);
		return chance >= randf();
	}

	int while_chance1(float chance)
	{
		int result = 0;
		while(chance1(chance))
		{
			result += 1;
		}
		return result;
	}

};

template <typename T, int N>
struct s_carray
{
	T elements[N];

	T& operator[](int index)
	{
		assert(index >= 0);
		assert(index < N);
		return elements[index];
	}

	constexpr int max_elements()
	{
		return N;
	}
};

func char* format_text(char* text, ...)
{
	constexpr int max_format_text_buffers = 16;
	constexpr int max_text_buffer_length = 256;

	local_persist char buffers[max_format_text_buffers][max_text_buffer_length] = zero;
	local_persist int index = 0;

	char* current_buffer = buffers[index];
	memset(current_buffer, 0, max_text_buffer_length);

	va_list args;
	va_start(args, text);
	#ifdef m_debug
	int written = vsnprintf(current_buffer, max_text_buffer_length, text, args);
	assert(written > 0 && written < max_text_buffer_length);
	#else
	vsnprintf(current_buffer, max_text_buffer_length, text, args);
	#endif
	va_end(args);

	index += 1;
	if(index >= max_format_text_buffers) { index = 0; }

	return current_buffer;
}

func void on_failed_assert(char* cond, char* file, int line)
{
	char* text = format_text("FAILED ASSERT IN %s (%i)\n%s\n", file, line, cond);
	printf("%s\n", text);
	int result = MessageBox(null, text, "Assertion failed", MB_RETRYCANCEL | MB_TOPMOST);
	if(result != IDRETRY)
	{
		if(IsDebuggerPresent())
		{
			__debugbreak();
		}
		else
		{
			ExitProcess(1);
		}
	}
}

func float range_lerp(float v, float amin, float amax, float bmin, float bmax)
{
	float p = ((v - amin) / (amax - amin));
	return bmin + (bmax - bmin) * p;
}

func float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

func s_v2 lerp(s_v2 a, s_v2 b, float t)
{
	s_v2 result;
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	return result;
}

func s_v4 lerp(s_v4 a, s_v4 b, float t)
{
	s_v4 result;
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	result.z = lerp(a.z, b.z, t);
	result.w = lerp(a.w, b.w, t);
	return result;
}

func s_v4 rand_color(s_random* rng)
{
	s_v4 result;
	result.x = rng->randf32();
	result.y = rng->randf32();
	result.z = rng->randf32();
	result.w = 1;
	return result;
}

func s_v2 v2_from_angle(float angle)
{
	return v2(
		cosf(angle),
		sinf(angle)
	);
}

func s_v4 multiply_rgb(s_v4 v4, float v)
{
	s_v4 result;
	result.x = v4.x * v;
	result.y = v4.y * v;
	result.z = v4.z * v;
	result.w = v4.w;
	return result;
}

template <typename T>
[[nodiscard]]
func T at_most(T high, T current)
{
	return current > high ? high : current;
}

template <typename T>
[[nodiscard]]
func T at_least(T low, T current)
{
	return current < low ? low : current;
}

template <typename T>
[[nodiscard]]
func T clamp(T current, T low, T high)
{
	return at_least(low, at_most(high, current));
}

[[nodiscard]]
func b8 rect_collides_rect(s_v2 pos1, s_v2 size1, s_v2 pos2, s_v2 size2)
{
	return pos1.x + size1.x >= pos2.x && pos1.x <= pos2.x + size2.x &&
		pos1.y + size1.y >= pos2.y && pos1.y <= pos2.y + size2.y;
}

[[nodiscard]]
// @Note(tkap, 07/03/2023): Top left
func s_v2 random_point_in_rect(s_v2 size, s_random* rng)
{
	return v2(
		rng->randf32() * size.x,
		rng->randf32() * size.y
	);
}

[[nodiscard]]
func int circular_index(int index, int size)
{
	assert(size > 0);
	if(index >= 0)
	{
		return index % size;
	}
	return (size - 1) - ((-index - 1) % size);
}

func s_v4 make_color(float v)
{
	return v4(v, v, v, 1.0f);
}

func void make_process_close_when_app_closes(HANDLE process)
{
	HANDLE job = CreateJobObjectA(null, null);
	assert(job);

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info = zero;
	job_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	BOOL set_info_result = SetInformationJobObject(job, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info));
	assert(set_info_result);

	BOOL assign_result = AssignProcessToJobObject(job, process);
	assert(assign_result);
}

func s_v4 hsv_to_rgb(float hue, float saturation, float value)
{
	s_v4 color;
	color.w = 1;

	// Red channel
	float k = fmodf((5.0f + hue/60.0f), 6);
	float t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.x = (value - value*saturation*k);

	// Green channel
	k = fmodf((3.0f + hue/60.0f), 6);
	t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.y = (value - value*saturation*k);

	// Blue channel
	k = fmodf((1.0f + hue/60.0f), 6);
	t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.z = (value - value*saturation*k);

	return color;
}