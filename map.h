#pragma once
#include "RBTree.h"

namespace jvn
{

	template <class T,
		class Cmp = std::less<T>,
		class Alloc = std::allocator<T>>
	class map : public RBTree<T, Cmp, Alloc>
	{
	public:
		using MyBase			= RBTree<T, Cmp, Alloc>;
		using value_type		= typename MyBase::value_type;
		using size_type			= typename MyBase::size_type;
		using difference_type	= typename MyBase::difference_type;
		using value_compare		= typename MyBase::value_compare;
		using allocator_type	= typename MyBase::allocator_type;
		using iterator			= typename MyBase::iterator;
	};

}	// namespace jvn