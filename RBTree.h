#pragma once
// std::allocator
#include <memory>
#include <stdexcept>

namespace jvn
{

	// Helper classes ----------------------

	template <class T>
	struct RBTreeNode
	{
		using value_type = T;
		using Node_ptr = RBTreeNode*;
		enum m_RB { m_red, m_black };

		value_type m_data;
		m_RB m_color;
		Node_ptr m_left;
		Node_ptr m_right;
		Node_ptr m_parent;

		RBTreeNode()								= default;
		RBTreeNode(const RBTreeNode&)				= default;
		RBTreeNode& operator=(const RBTreeNode&)	= delete;

		Node_ptr sibling()
		{
			if (m_parent == nullptr)
				return nullptr;
			if (this == m_parent->m_left)
				return m_parent->m_right;
			return m_parent->m_left;
		}

#define CHECK_ALLOC_TYPE static_assert(std::is_same<typename Alloc::value_type, RBTreeNode>::value, "Invalid call");

		template <class Alloc>
		static Node_ptr buy_head_node(Alloc& alloc)
		{
			CHECK_ALLOC_TYPE
			Node_ptr head = alloc.allocate(1);
			alloc.construct(head, RBTreeNode());
			head->m_color = m_black;
			head->m_parent = nullptr;
			head->m_left = nullptr;
			head->m_right = nullptr;
			return head;
		}

		template <class Alloc>
		static Node_ptr buy_node(Alloc& alloc, Node_ptr parent)
		{
			CHECK_ALLOC_TYPE
			Node_ptr node = alloc.allocate(1);
			alloc.construct(node, RBTreeNode());
			node->m_color = m_red;
			node->m_parent = parent;
			node->m_left = nullptr;
			node->m_right = nullptr;
			return node;
		}

		template <class Alloc>
		void destroy_node(Alloc& alloc)
		{
			CHECK_ALLOC_TYPE
			alloc.deallocate(this, 1);
		}
	};

	template <class T>
	class RBTreeIterator
	{
	public:
		using value_type	= T;
	protected:
		using Node			= RBTreeNode<T>;
		using Node_ptr		= Node*;
	public:
		RBTreeIterator(Node_ptr node) : m_pointer(node) {}
		RBTreeIterator(const RBTreeIterator&)				= default;
		RBTreeIterator& operator=(const RBTreeIterator&)	= default;
		~RBTreeIterator()									= default;

		friend bool operator==(const RBTreeIterator& lhs, const RBTreeIterator& rhs)
		{
			return (lhs.m_pointer == rhs.m_pointer)
				|| (lhs.m_pointer != nullptr && rhs.m_pointer != nullptr && lhs.m_pointer->m_data == rhs.m_pointer->m_data);
		}
		friend bool operator!=(const RBTreeIterator& lhs, const RBTreeIterator& rhs) { return !(lhs == rhs); }
		RBTreeIterator& operator++()
		{
			if (m_pointer == nullptr)
				throw std::runtime_error("End of iteration reached");

			if (m_pointer->m_parent == nullptr)
			{
				m_pointer = m_pointer->m_right;
				if (m_pointer == nullptr)
					return *this;
				while (m_pointer->m_left != nullptr)
					m_pointer = m_pointer->m_left;
			}
			else if (m_pointer->m_right != nullptr)
			{
				m_pointer = m_pointer->m_right;
				while (m_pointer->m_left != nullptr)
					m_pointer = m_pointer->m_left;
			}
			else
			{
				while (m_pointer->m_parent != nullptr && m_pointer->m_parent->m_right == m_pointer)
					m_pointer = m_pointer->m_parent;
				m_pointer = m_pointer->m_parent;
			}
			return *this;
		}

		RBTreeIterator operator++(int) { RBTreeIterator tmp(*this); operator++(); return tmp; }
		const value_type operator*() { if (m_pointer == nullptr) throw std::runtime_error("Exceeded boundary"); return m_pointer->m_data; }
	protected:
		Node_ptr m_pointer;
	};

	// -------------------------------------

	template <class T,
		class Cmp = std::less<T>,
		class Alloc = std::allocator<T>>
		class RBTree
	{
	public:
		using value_type			= T;
		using value_compare			= Cmp;
		using allocator_type		= Alloc;
		using size_type				= typename std::size_t;
		using difference_type		= typename std::ptrdiff_t;
		using iterator				= RBTreeIterator<T>;
	protected:
		using Node					= RBTreeNode<T>;
		using Node_ptr				= Node*;
		using node_allocator_type	= typename allocator_type::template rebind<Node>::other;

	public:
		RBTree() : m_root(nullptr), m_size(0) {}
		RBTree(const RBTree& t) : RBTree() { operator=(t); }
		RBTree(RBTree&& t) : RBTree() { swap(*this, t); }
		RBTree& operator=(const RBTree& t);
		RBTree& operator=(RBTree&& t) { swap(*this, t); return *this; }

		~RBTree() { destroy_tree(m_root); }

		void clear() { destroy_tree(m_root); m_root = nullptr; m_size = 0; }
		void insert(const value_type& val);
		void erase(const value_type& val);
		iterator find(const value_type& val) const { return iterator(find_node(val)); }

		iterator begin() const;
		iterator end() const;

		size_type size() const { return m_size; }
		bool empty() const { return !m_size; }

		friend void swap(RBTree& lhs, RBTree& rhs);
	private:
		node_allocator_type m_node_allocator;
		Node_ptr m_root;
		size_type m_size;

		void destroy_tree(Node_ptr node)
		{
			if (node != nullptr)
			{
				destroy_tree(node->m_left);
				destroy_tree(node->m_right);
				node->destroy_node(m_node_allocator);
			}
		}

		static void copy_tree(Node_ptr& root_dst, Node_ptr root_src, node_allocator_type& allocator, Node_ptr parent = nullptr)
		{
			if (root_src == nullptr)
				root_dst = nullptr;
			else
			{
				root_dst = Node::buy_node(allocator, parent);
				root_dst->m_data = root_src->m_data;
				root_dst->m_color = root_src->m_color;
				copy_tree(root_dst->m_left, root_src->m_left, allocator, root_dst);
				copy_tree(root_dst->m_right, root_src->m_right, allocator, root_dst);
			}
		}

		Node_ptr find_node(const value_type& val) const
		{
			Node_ptr search = m_root;
			while (search != nullptr && search->m_data != val)
				search = value_compare{}(val, search->m_data) ? search->m_left : search->m_right;
			return search;
		}

		//	DELETION ---------------------

		void delete_node(Node_ptr node)
		{
			Node_ptr y = BST_replace(node);
			bool double_black = ((y == nullptr || y->m_color == Node::m_black) &&
				(node->m_color == Node::m_black));
			Node_ptr parent = node->m_parent;

			if (y == nullptr)
			{
				if (node == m_root)
					m_root = nullptr;
				else
				{
					if (double_black)
						double_black_fix(node);
					else if (node->sibling() != nullptr)
						node->sibling()->m_color = Node::m_red;
					if (node->m_parent->m_left == node)
						parent->m_left = nullptr;
					else
						parent->m_right = nullptr;
				}
				node->destroy_node(m_node_allocator);
				--m_size;
				return;
			}

			if (node->m_left == nullptr || node->m_right == nullptr)
			{
				if (node == m_root)
				{
					node->m_data = y->m_data;
					node->m_left = node->m_right = nullptr;
					y->destroy_node(m_node_allocator);
				}
				else
				{
					if (node->m_parent->m_left == node)
						parent->m_left = y;
					else
						parent->m_right = y;
					node->destroy_node(m_node_allocator);
					y->m_parent = parent;
					if (double_black)
						double_black_fix(y);
					else
						y->m_color = Node::m_black;
				}
				--m_size;
				return;
			}
			std::swap(y->m_data, node->m_data);
			delete_node(y);
		}

		void double_black_fix(Node_ptr node)
		{
			if (node == m_root)
				return;

			Node_ptr sibling = node->sibling();
			Node_ptr parent = node->m_parent;
			if (sibling == nullptr)
				double_black_fix(parent);
			else
			{
				if (sibling->m_color == Node::m_red)
				{
					parent->m_color = Node::m_red;
					sibling->m_color = Node::m_black;
					if (sibling->m_parent->m_left == sibling)
						right_rotate(parent);
					else
						left_rotate(parent);
					double_black_fix(node);
				}
				else
					if ((sibling->m_left != nullptr && sibling->m_left->m_color == Node::m_red) ||
						(sibling->m_right != nullptr && sibling->m_right->m_color == Node::m_red))
					{
						if (sibling->m_left != nullptr && sibling->m_left->m_color == Node::m_red)
							if (sibling->m_parent->m_left == sibling)
							{
								sibling->m_left->m_color = sibling->m_color;
								sibling->m_color = parent->m_color;
								right_rotate(parent);
							}
							else
							{
								sibling->m_left->m_color = parent->m_color;
								right_rotate(sibling);
								left_rotate(parent);
							}
						else
							if (sibling->m_parent->m_left == sibling)
							{
								sibling->m_right->m_color = parent->m_color;
								left_rotate(sibling);
								right_rotate(parent);
							}
							else
							{
								sibling->m_right->m_color = sibling->m_color;
								sibling->m_color = parent->m_color;
								left_rotate(parent);
							}
						parent->m_color = Node::m_black;
					}
					else
					{
						sibling->m_color = Node::m_red;
						if (parent->m_color == Node::m_black)
							double_black_fix(parent);
						else
							parent->m_color = Node::m_black;
					}
			}
		}

		Node_ptr BST_replace(Node_ptr node)
		{
			if (node->m_left != nullptr && node->m_right != nullptr)
			{
				Node_ptr tmp = node->m_right;
				while (tmp->m_left != nullptr)
					tmp = tmp->m_left;
				return tmp;
			}
			if (node->m_left == nullptr && node->m_right == nullptr)
				return nullptr;
			if (node->m_left != nullptr)
				return node->m_left;
			return node->m_right;
		}

		// -------------------------------

		// INSERTION ---------------------

		Node_ptr BST_insert(const value_type& val)
		{
			Node_ptr search = m_root;
			if (empty())
			{
				m_root = Node::buy_head_node(m_node_allocator);
				m_root->m_data = val;
				search = m_root;
				++m_size;
			}
			else
			{
				Node_ptr parent = nullptr;
				while (search != nullptr && search->m_data != val)
				{
					parent = search;
					search = value_compare{}(val, search->m_data) ? search->m_left : search->m_right;
				}
				if (search == nullptr)
				{
					search = Node::buy_node(m_node_allocator, parent);
					search->m_data = val;
					if (value_compare{}(val, parent->m_data))
						parent->m_left = search;
					else
						parent->m_right = search;
					++m_size;
				}
			}
			return search;
		}

		void insert_fix(Node_ptr node)
		{
			while (node != m_root && node->m_color != Node::m_black
				&& node->m_parent->m_color == Node::m_red)
			{
				Node_ptr parent = node->m_parent;
				Node_ptr grand_parent = node->m_parent->m_parent;
				Node_ptr uncle = parent->sibling();
				if (parent == grand_parent->m_left)
				{
					if (uncle != nullptr && uncle->m_color == Node::m_red)
					{
						grand_parent->m_color = Node::m_red;
						parent->m_color = Node::m_black;
						uncle->m_color = Node::m_black;
						node = grand_parent;
					}
					else
					{
						if (node == parent->m_right)
						{
							left_rotate(parent);
							node = parent;
							parent = node->m_parent;
						}
						right_rotate(grand_parent);
						std::swap(parent->m_color, grand_parent->m_color);
						node = parent;
					}
				}
				else
				{
					if (uncle != nullptr && uncle->m_color == Node::m_red)
					{
						grand_parent->m_color = Node::m_red;
						parent->m_color = Node::m_black;
						uncle->m_color = Node::m_black;
						node = grand_parent;
					}
					else
					{
						if (node == parent->m_left)
						{
							right_rotate(parent);
							node = parent;
							parent = node->m_parent;
						}
						left_rotate(grand_parent);
						std::swap(parent->m_color, grand_parent->m_color);
						node = parent;
					}
				}
			}
			m_root->m_color = Node::m_black;
		}

		// -------------------------------

		void left_rotate(Node_ptr node)
		{
			Node* right = node->m_right;
			node->m_right = right->m_left;
			if (node->m_right != NULL)
				node->m_right->m_parent = node;
			right->m_parent = node->m_parent;
			if (node->m_parent == NULL)
				m_root = right;
			else if (node == node->m_parent->m_left)
				node->m_parent->m_left = right;
			else
				node->m_parent->m_right = right;
			right->m_left = node;
			node->m_parent = right;
		}

		void right_rotate(Node_ptr node)
		{
			Node_ptr left = node->m_left;
			node->m_left = left->m_right;
			if (node->m_left != nullptr)
				node->m_left->m_parent = node;
			left->m_parent = node->m_parent;
			if (node->m_parent == NULL)
				m_root = left;
			else if (node == node->m_parent->m_left)
				node->m_parent->m_left = left;
			else
				node->m_parent->m_right = left;
			left->m_right = node;
			node->m_parent = left;
		}
	};

	template <class T, class Cmp, class Alloc>
	void RBTree<T, Cmp, Alloc>::insert(const value_type& val)
	{
		Node_ptr added = BST_insert(val);
		insert_fix(added);
	}

	template <class T, class Cmp, class Alloc>
	void RBTree<T, Cmp, Alloc>::erase(const value_type& val)
	{
		Node_ptr search = find_node(val);
		if (search == nullptr)
			throw "Value has not been inserted";

		delete_node(search);
	}

	template <class T, class Cmp, class Alloc>
	typename RBTree<T, Cmp, Alloc>::iterator RBTree<T, Cmp, Alloc>::begin() const
	{
		if (m_root == nullptr)
			return end();
		Node_ptr leftmost_node = m_root;
		while (leftmost_node->m_left != nullptr)
			leftmost_node = leftmost_node->m_left;
		return iterator(leftmost_node);
	}

	template <class T, class Cmp, class Alloc>
	typename RBTree<T, Cmp, Alloc>::iterator RBTree<T, Cmp, Alloc>::end() const
	{
		return iterator(nullptr);
	}

	template <class T, class Cmp, class Alloc>
	RBTree<T, Cmp, Alloc>& RBTree<T, Cmp, Alloc>::operator=(const RBTree& t)
	{
		m_size = t.m_size;
		destroy_tree(m_root);
		copy_tree(m_root, t.m_root, m_node_allocator);
		return *this;
	}

	template <class T, class Cmp, class Alloc>
	void swap(RBTree<T, Cmp, Alloc>& lhs, RBTree<T, Cmp, Alloc>& rhs)
	{
		using std::swap;
		swap(lhs.m_node_allocator, rhs.m_node_allocator);
		swap(lhs.m_root, rhs.m_root);
		swap(lhs.m_size, rhs.m_root);
	}

}	// namespace jvn