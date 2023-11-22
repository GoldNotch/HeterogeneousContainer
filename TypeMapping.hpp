#pragma once
#include <type_traits> 


namespace Core::metaprogramming
{
    /*
    * type_table is compile-time datastructure to indexing types with numbers from 0 to N
    * 
    */
    template<typename... Ts>
    class type_table final
    {
        /// node of linked list
        template<size_t Index, typename T, typename... Ts>
        struct type_table_row;

        /// special structure to find type by index
        template<typename Table, size_t idx>
        struct row_indexer;

        /// first row in table
        using data = type_table_row<0, Ts...>;

    public:
        /// Size of table
        static constexpr size_t size = sizeof...(Ts);
        /// Get index of type
        template<typename T>
        static constexpr size_t index_of = data::template index_of<T>();
        /// Get type by index
        template<size_t idx>
        using type_of = typename row_indexer<data, idx>::indexed_type;
        /// First Type in table
        using FirstT = type_of<0>;
        /// last type in table
        using LastT = type_of<size - 1>;
    };

    // --------------------- Private implementation -------------------------

    /// linked list of rows in type table
    template<typename... _Ts>
    template<size_t Index, typename T, typename... Ts>
    struct type_table<_Ts...>::type_table_row final
    {
        static constexpr size_t index = Index;
        using type = T;
        using next = type_table_row<index + 1, Ts...>;

        template<typename U>
        static constexpr size_t index_of()
        {
            if constexpr (std::is_same_v<U, T>) return index;
            else return next::template index_of<U>();
        }
    };

    /// last row in type table
    template<typename... _Ts>
    template<size_t Index, typename T>
    struct type_table<_Ts...>::type_table_row<Index, T> final
    {
        static constexpr size_t index = Index;
        using type = T;
        template<typename U>
        static constexpr size_t index_of()
        {
            if constexpr (std::is_same_v<U, T>) return index;
            else throw "No type in table";
        }
    };

    /// structure to get type by index in compile-time
    template< typename... _Ts>
    template<typename Table, size_t idx>
    struct type_table<_Ts...>::row_indexer final 
    {
        using indexed_type = typename row_indexer<typename Table::next, idx - 1>::indexed_type;
    };

    /// final state of searching type by index
    template< typename... _Ts>
    template<typename Table>
    struct type_table<_Ts...>::row_indexer<Table, 0> final
    {
        using indexed_type = typename Table::type;
    };

}

  