

#define foreach__(a, index_name, element_name, array) if(0) finished##a: ; else for(auto element_name = &(array).elements[0];;) if(1) goto body##a; else while(1) if(1) goto finished##a; else body##a: for(int index_name = 0; index_name < (array).count && (bool)(element_name = &(array)[index_name]); index_name++)
#define foreach_(a, index_name, element_name, array) foreach__(a, index_name, element_name, array)
#define foreach(index_name, element_name, array) foreach_(__LINE__, index_name, element_name, array)

#define foreach_raw__(a, index_name, element_name, array) if(0) finished##a: ; else for(auto element_name = (array).elements[0];;) if(1) goto body##a; else while(1) if(1) goto finished##a; else body##a: for(int index_name = 0; index_name < (array).count && (void*)&(element_name = (array)[index_name]); index_name++)
#define foreach_raw_(a, index_name, element_name, array) foreach_raw__(a, index_name, element_name, array)
#define foreach_raw(index_name, element_name, array) foreach_raw_(__LINE__, index_name, element_name, array)

template <typename T, int N>
struct s_sarray
{
	int count = 0;
	T elements[N];

	T& operator[](int index)
	{
		assert(index >= 0);
		assert(index < count);
		return elements[index];
	}

	constexpr void add(T element)
	{
		assert(count < N);
		elements[count++] = element;
	}

	constexpr void maybe_add(T element)
	{
		if(count < N)
		{
			add(element);
		}
	}

	constexpr void remove_and_swap(int index)
	{
		assert(index >= 0);
		assert(index < count);
		elements[index] = elements[--count];
	}

	constexpr void remove_and_shift(int index)
	{
		assert(index >= 0);
		assert(index < count);
		count -= 1;

		int to_move = count - index;
		if(to_move > 0)
		{
			memcpy(elements + index, elements + index + 1, to_move * sizeof(T));
		}
	}

	constexpr int max_elements()
	{
		return N;
	}

	constexpr b8 is_last(int index)
	{
		assert(index >= 0);
		assert(index < count);
		return index == count - 1;
	}

	constexpr b8 is_full()
	{
		return count >= N;
	}
};
