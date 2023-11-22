# HeterogeneousContainer
This library implements 3 containers:
- TypeTable - compile-time array of types with indexing from 0
- Storage - general-purpose container to store objects of one type.
- HeterogeneousStorage - general-purpose container to store objects of several types.

## Type Table
You can think it's an array of types. Every type in the array has its index (indexing from 0).  
And it provides two things: get type by index and get index of type.
Examples:  

## Storage
Storage is a container to store objects. you can insert, delete and read stored objects. And it's fast, it's O(1) complexity  
Insertion works like std::vector. When you're inserting an object, list creates node in pre-allocated memory. If there is no free memory, it will make realloc with block of 2x size. So it's amortized O(1) complexity.  
Insertion returns StoragePointer object. It's std::list iterator, only you can't increment and decrement it. With this pointer you can get access to the stored object.
Deletion accepts StoragePointer and just removes std::list node from list.  
Reading it's just dereference for iterator.  
Examples:  

## Heterogeneous Storage
It's a mixing of two previous structures. It's creates an std::array of different typed Storages. Indices for std::array are calculated in type table. So we get one storage for each type. And you can do all common operations like insertion, deletion, reading, etc.  
Insertion returns typed StoragePointer, like Storage insertion does. But Heterogeneous Storage has one thing more - Generic Object Pointer. It's like void* but checks types. It can points on every object in this container, but has one restriction - you can't just dereference it because stored type is unknown. You can cast it to typed ObjectPointer and then make dereference. Now if cast failed, it will throw exception (maybe it will change in fiture).  
Also heterogeneous storage don't provide iteration through all objects (of any type). But you can iterate through all objects of one choosen type.  


