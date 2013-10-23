// http://www.cplusplus.com/reference/algorithm/rotate/

namespace eastl
{
	template <class ForwardIterator>
	void rotate(ForwardIterator first, ForwardIterator middle, ForwardIterator last)
	{
		ForwardIterator next = middle;
		while (first != next)
		{
			swap(*first++, *next++);
			if (next == last)
				next = middle;
			else if (first == middle)
				middle = next;
		}
	}
}
