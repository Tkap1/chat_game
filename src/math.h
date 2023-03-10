
global constexpr float epsilon = 0.000001f;

struct s_v2i
{
	int x;
	int y;
};

template <typename T>
func s_v2i v2i(T x, T y)
{
	return {(int)x, (int)y};
}

template <typename T>
func s_v2i v2i(T v)
{
	return {(int)v, (int)v};
}

func s_v2 operator+(s_v2 a, s_v2 b)
{
	return {
		a.x + b.x,
		a.y + b.y
	};
}

func s_v2 operator-(s_v2 a, s_v2 b)
{
	return {
		a.x - b.x,
		a.y - b.y
	};
}

func s_v2 operator*(s_v2 a, s_v2 b)
{
	return {
		a.x * b.x,
		a.y * b.y
	};
}

func s_v2 operator*(s_v2 a, float b)
{
	return {
		a.x * b,
		a.y * b
	};
}

func s_v2 operator+=(s_v2& a, s_v2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

func s_v2 operator-=(s_v2& a, s_v2 b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

func s_v2 operator*=(s_v2& a, s_v2 b)
{
	a.x *= b.x;
	a.y *= b.y;
}

func s_v2 operator*=(s_v2& a, float b)
{
	a.x *= b;
	a.y *= b;
}

func float sinf2(float t)
{
	return sinf(t) * 0.5f + 0.5f;
}

[[nodiscard]]
func constexpr b8 floats_equal(float a, float b)
{
	return (a >= b - epsilon && a <= b + epsilon);
}

func float v2_length_squared(s_v2 v)
{
	return v.x * v.x + v.y * v.y;
}

func float v2_length(s_v2 v)
{
	return sqrtf(v2_length_squared(v));
}

func float v2_distance(s_v2 a, s_v2 b)
{
	s_v2 c = a - b;
	return v2_length(c);
}

func float v2_distance_squared(s_v2 a, s_v2 b)
{
	s_v2 c;
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	return v2_length_squared(c);
}

func s_v2 v2_normalized(s_v2 v)
{
	s_v2 result = v;
	float length = v2_length(v);
	if(!floats_equal(length, 0))
	{
		result.x /= length;
		result.y /= length;
	}
	return result;
}
