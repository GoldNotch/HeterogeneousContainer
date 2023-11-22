#pragma once
#include <list>
#include <memory_resource>
#include <optional>
#include <cassert>
#include <type_traits>
#include <variant>
#include <numeric>
#include "TypeMapping.hpp"

namespace Core
{
	/*
	* Storage is container of same-type objects.
	* It has really fast insertion and deletion complexity (O(1) - complexity)
	* Also all objects are close to each other in memory, so it's cache friendly
	* API partially copied from std::list
	*/
	template <class T, size_t BufSize = 4096>
	class Storage final : private std::pmr::list<T>
	{
		using container = std::pmr::list<T>;
	public:
		using container::begin;
		using container::end;
		using container::rbegin;
		using container::rend;
		using container::cbegin;
		using container::cend;
		using container::crbegin;
		using container::crend;
		using container::size;
		using container::clear;
		using container::empty;
		using container::swap;
		using container::operator=;
		using container::assign;
		using container::iterator;
		using container::const_iterator;
		using container::reverse_iterator;
		using container::const_reverse_iterator;

		Storage() = default;
		/// copy constructor
		Storage(const Storage& other)
			: container(other) {}
		Storage(Storage&&) = default;
		
		/// ObjectPointer - handler or reference of object in storage. 
		struct ObjectPointer;

		/// construct object in storage
		template<typename... Args>
		ObjectPointer emplace(Args&& ...args)
		{
			T val(std::forward<Args>(args)...);
			return ObjectPointer(this, container::emplace(end(), std::move(val)));
		}

		/// delete object from storage
		void erase(ObjectPointer&& ptr)
		{
			container::erase(ptr.it);
			ptr = ObjectPointer(this, end());
		}

	private:
		/*
		* Storage is built as list of objects. We use list becuase it's really fast insertion and deletion - O(1) - complexity
		* But list has one drawback - nodes are scattered in memory. It's cache-unfriendly, so we use std::pmr::list with monotonic buffer
		* Actually it can be any memory_resource
		*/
		std::pmr::monotonic_buffer_resource pool{ BufSize };
		// TODO: make memory resource as template argument
	};


	/*
	* Problem: you must have std::list iterator to erase object for O(1)
	* ObjectPointer is solution of the problem. 
	* ObjectPointer is an std::list iterator without move operators (increment, decrement)
	* You can think about it as a reference or pointer on object in memory.
	*/
	template<typename T, size_t BufSize>
	struct Storage<T, BufSize>::ObjectPointer final
	{
		ObjectPointer() = default;
		ObjectPointer(const Storage<T, BufSize>* container, container::iterator it)
			: cont(container), it(it) {}

		constexpr T& operator*()& { return it.operator*(); }
		constexpr const T& operator*() const& { return it.operator*(); }
		constexpr T* operator->()& { return it.operator->(); }
		constexpr const T* operator->() const& { return it.operator->(); }

		constexpr bool operator==(ObjectPointer other) const { return it == other.it; }
		constexpr operator bool() const noexcept { return cont && it != cont->end(); }
		constexpr const Storage<T, BufSize>* GetStorage() const& { return cont; }
	private:
		friend Storage<T, BufSize>;
		container::iterator it; ///< std::list iterator on object in list
		const Storage<T, BufSize>* cont = nullptr; ///< pointer on owning storage
	};


	/*
	* HeterogeneousStorage is multi-type container.
	* It provides O(1) complexity for insertion, deletion
	* Also it's cache-friendly (all objects are dense packed in memory)
	*/
	template<typename... Ts>
	class HeterogeneousStorage final
	{
		template<typename ObjT>
		using StorageBacket = Storage<ObjT>; ///< Typed storage, one of them
		template<typename ObjT>
		using ObjPtr = typename StorageBacket<ObjT>::ObjectPointer; ///< short alias for object 

		/// aliases for storage iterators
		template<typename ObjT>
		using typed_iterator = typename StorageBacket<ObjT>::iterator; 
		template<typename ObjT>
		using typed_const_iterator = typename StorageBacket<ObjT>::const_iterator;
		template<typename ObjT>
		using typed_reverse_iterator = typename StorageBacket<ObjT>::reverse_iterator;
		template<typename ObjT>
		using typed_const_reverse_iterator = typename StorageBacket<ObjT>::const_reverse_iterator;


	public:
		/// default constructor
		HeterogeneousStorage() = default;
		HeterogeneousStorage(const HeterogeneousStorage&) = default;
		HeterogeneousStorage(HeterogeneousStorage&&) = default;
		/// Untyped/Generic ObjectPointer
		struct GenericObjectPointer;
		friend GenericObjectPointer;

		using iterator = std::variant<typed_iterator<Ts>...>;
		using const_iterator = std::variant<typed_const_iterator<Ts>...>;
		using reverse_iterator = std::variant<typed_reverse_iterator<Ts>...>;
		using const_reverse_iterator = std::variant<typed_const_reverse_iterator<Ts>...>;

		/// construct object of type ObjT in storage
		template <typename ObjT, typename... Args>
		ObjPtr<ObjT> emplace(Args &&...args)
		{
			return Get<ObjT>().emplace(std::forward<Args>(args)...);
		}

		/// remove object from container
		template <typename ObjT>
		void erase(ObjPtr<ObjT>&& ptr)
		{
			return Get<ObjT>().erase(std::forward<ObjPtr<ObjT>>(ptr));
		}

		/// remove object from container
		void erase(GenericObjectPointer&& ptr) { erase(ptr); }

		/// returns count of objects in container
		constexpr size_t size() const noexcept
		{
			std::array<size_t, sizeof...(Ts)> sizes = { Get<Ts>().size()... };
			return std::reduce(sizes.begin(), sizes.end());
		}

		/// check if no objects in container
		constexpr bool empty() const noexcept{ return size() == 0; }

		/// ------------------- Begin/End -----------------
		template<typename ObjT>
		constexpr decltype(auto) begin() noexcept { return Get<ObjT>().begin(); }

		template<typename ObjT>
		constexpr decltype(auto) end() noexcept { return Get<ObjT>().end(); }

		template<typename ObjT>
		constexpr decltype(auto) cbegin() const noexcept { return Get<ObjT>().cbegin(); }

		template<typename ObjT>
		constexpr decltype(auto) cend() const noexcept { return Get<ObjT>().cend(); }

		template<typename ObjT>
		constexpr decltype(auto) rbegin() noexcept { return Get<ObjT>().rbegin(); }

		template<typename ObjT>
		constexpr decltype(auto) rend() noexcept { return Get<ObjT>().rend(); }

		template<typename ObjT>
		constexpr decltype(auto) crbegin() const noexcept { return Get<ObjT>().crbegin(); }

		template<typename ObjT>
		constexpr decltype(auto) crend() const noexcept { return Get<ObjT>().crend(); }

	private:
		using TypeIndexer = metaprogramming::type_table<Ts...>;

		using VariadicContainer = std::variant<StorageBacket<Ts>...>;
		std::array<VariadicContainer, TypeIndexer::size> containers = { StorageBacket<Ts>()... };

	private:
		/// Get typed StorageBacket
		template<typename ObjT>
		constexpr StorageBacket<ObjT>& Get()&
		{
			return const_cast<StorageBacket<ObjT>&>(
					const_cast<const HeterogeneousStorage*>(this)->Get<ObjT>()
				   );
		}

		/// Get typed StorageBacket
		template<typename ObjT>
		constexpr const StorageBacket<ObjT>& Get() const&
		{
			constexpr size_t idx = TypeIndexer::template index_of<ObjT>;
			try
			{
				return std::get<StorageBacket<ObjT>>(containers[idx]);
			}
			catch (const std::bad_variant_access&)
			{
				throw std::runtime_error("Type doesn't stored in this storage");
			}
		}
	};

	/*
	* TypedView - is adapter for HeterogeneousStorage which is provide for(auto ...) syntax 
	* Example: for(auto && val : TypedAdapter<int, decltype(storage)>(storage)) - iterate over all int values in container
	*/
	template<typename ObjT, typename HStorageT>
	struct TypedView final
	{
		HStorageT& storage;
		TypedView(HStorageT& storage) : storage(storage) {}

		constexpr decltype(auto) begin() noexcept { return storage.begin<ObjT>(); }
		constexpr decltype(auto) end() noexcept { return storage.end<ObjT>(); }
		constexpr decltype(auto) cbegin() const noexcept { return storage.cbegin<ObjT>(); }
		constexpr decltype(auto) cend() const noexcept { return storage.cend<ObjT>(); }
		constexpr decltype(auto) rbegin() noexcept { return storage.rbegin<ObjT>(); }
		constexpr decltype(auto) rend() noexcept { return storage.rend<ObjT>(); }
		constexpr decltype(auto) crbegin() const noexcept { return storage.crbegin<ObjT>(); }
		constexpr decltype(auto) crend() const noexcept { return storage.rend<ObjT>(); }
	};

	/*
	* GenericObjectPointer is multi-type pointer
	*/
	template<typename... Ts>
	struct HeterogeneousStorage<Ts...>::GenericObjectPointer final
	{
		GenericObjectPointer() = default;

		template<typename ObjPtrT>
		GenericObjectPointer(ObjPtrT ptr) : ptr(ptr) {}

		/// static cast to ObjectPointer. Throw exception if type is wrong
		template<typename ObjPtrT>
		constexpr operator ObjPtrT() const
		{
			try {
				return std::get<ObjPtrT>(ptr);
			}
			catch (const std::bad_variant_access&)
			{
				throw std::runtime_error("Type doesn't stored in this storage");
			}
		}

		/// equal operator
		constexpr bool operator==(const GenericObjectPointer& other) const noexcept
		{
			return other.ptr == ptr;
		}

		/// check if pointer if valid (points on storage-places object)
		constexpr operator bool() const
		{
			return ptr.valueless_by_exception &&
				std::visit(ptr, [](auto&& ptr) { return ptr.operator bool(); });
		}

	private:
		std::variant<ObjPtr<Ts>...> ptr; ///< pointer
	};
}