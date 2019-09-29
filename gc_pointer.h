#include <iostream>
#include <list>
#include <typeinfo>
#include <cstdlib>
#include "gc_details.h"
#include "gc_iterator.h"
/*
    Pointer implements a pointer type that uses
    garbage collection to release unused memory.
    A Pointer must only be used to point to memory
    that was dynamically allocated using new.
    When used to refer to an allocated array,
    specify the array size.
*/
template <class T, int size = 0>
class Pointer {
private:
	// --------------- private member variables ------------------

    // refContainer maintains the garbage collection list.
    static std::list<PtrDetails<T> > refContainer;

    // addr points to the allocated memory to which
    // this Pointer pointer currently points.
    T *addr = nullptr;

    /*  isArray is true if this Pointer points
        to an allocated array. It is false
        otherwise. 
    */
    bool isArray = false; 

    // If this Pointer is pointing to an allocated
    // array, then arraySize contains its size.
    unsigned int arraySize = 0; // size of the array

    static bool first; // true when first Pointer is created

	// --------------- private member functions ------------------

    // Return an iterator to pointer details in refContainer.
    typename std::list<PtrDetails<T> >::iterator findPtrInfo(T *ptr);

	void setLocalData(const typename std::list<PtrDetails<T>>::iterator & p);

public:
	// ---------------- public member variables ------------------

    // Define an iterator type for Pointer<T>.
    typedef Iter<T> GCiterator;

	// ---------------- public member functions ------------------

    // Empty constructor
    // NOTE: templates aren't able to have prototypes with default arguments
    // this is why constructor is designed like this:
    Pointer()
	{
        Pointer(NULL);
    }

    Pointer(T*);

    // Copy constructor.
    Pointer(const Pointer &);

    // Destructor for Pointer.
    ~Pointer();

    // Collect garbage. Returns true if at least
    // one object was freed.
    static bool collect();

    // Overload assignment of pointer to Pointer.
    T *operator=(T *t);

    // Overload assignment of Pointer to Pointer.
    Pointer &operator=(Pointer &rv);

    // Return a reference to the object pointed
    // to by this Pointer.
    T &operator*()
	{
        return *addr;
    }

    // Return the address being pointed to.
    T *operator->()
	{
		return addr;
	}

    // Return a reference to the object at the
    // index specified by i.
    T &operator[](int i)
	{
		return addr[i];
	}

    // Conversion function to T *.
    operator T *()
	{
		return addr;
	}

    // Return an Iter to the start of the allocated memory.
    Iter<T> begin()
	{
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr, addr, addr + _size);
    }

    // Return an Iter to one past the end of an allocated array.
    Iter<T> end()
	{
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr + _size, addr, addr + _size);
    }

    // Return the size of refContainer for this type of Pointer.
    static int refContainerSize()
	{
		return refContainer.size();
	}

    // A utility function that displays refContainer.
    static void showlist();

    // Clear refContainer when program exits.
    static void shutdown();
};

// STATIC INITIALIZATION
// Creates storage for the static variables
template <class T, int size>
std::list<PtrDetails<T> > Pointer<T, size>::refContainer;

template <class T, int size>
bool Pointer<T, size>::first = true;

// Constructor for both initialized and uninitialized objects. -> see class interface
template<class T, int size>
Pointer<T, size>::Pointer(T *t)
{
	// Register shutdown() as an exit function.
	if (first)
		atexit(shutdown);
	first = false;

	// This object might be in the list already.. checking
	typename std::list<PtrDetails<T> >::iterator p;
	p = findPtrInfo(t);

	// Case: It IS in the list
	if (p != refContainer.end()) {
		// increment ref count
		p->refCount++;
	} else {
		// If it is not in the list create new PtrDetails object
		PtrDetails<T> temp(t, size); // Note: refCount updated in PtrDetails

		// add to the container
		refContainer.push_back(temp);

		// set other variables for this Pointer object
		//this->addr = temp.memPtr;
		//this->isArray = temp.isArray;
		//this->arraySize = temp.arraySize;
	}

	// find again to set
	p = findPtrInfo(t);
	// set other variables for this Pointer object
	setLocalData(p);

}

// Copy constructor.
template< class T, int size>
Pointer<T, size>::Pointer(const Pointer &ob)
{
	// Find the object in the list
	typename std::list<PtrDetails<T> >::iterator p;
	p = findPtrInfo(ob.addr);

	// It will always be in the list!!
	// increment ref count
	p->refCount++;

	// set local vars...
	setLocalData(p);

}

// Destructor for Pointer.
template <class T, int size>
Pointer<T, size>::~Pointer()
{
	typename std::list<PtrDetails<T> >::iterator p;
	p = findPtrInfo(this->addr);

	// If for some reason it is not in the list there is no memory to clean up
	if (p != refContainer.end()) {
		// decrement ref count
		if (p->refCount) {
			p->refCount--;
		}
	}
	
	// Collect garbage when a pointer goes out of scope.
	collect();

}

// Collect garbage. Returns true if at least
// one object was freed.
template <class T, int size>
bool Pointer<T, size>::collect()
{
	bool memfreed = false;
	typename std::list<PtrDetails<T> >::iterator p;
	do {
		// Scan refContainer looking for unreferenced pointers.
		for (p = refContainer.begin(); p != refContainer.end(); p++) {
			// If in-use, skip.
			if (p->refCount > 0) {
				continue;
			}
			memfreed = true;
			
			// Free memory unless the Pointer is null.
			if (p->memPtr != nullptr) {
				if (p->isArray) {
					delete[] p->memPtr; // delete array
				}
				else {
					delete p->memPtr; // delete single element
				}
			}

			// Remove unused entry from refContainer.
			p = refContainer.erase(p);
			//refContainer.remove(*p);
			// Restart the search.
			break;
		}
	} while (p != refContainer.end());

	return memfreed;
}

// Overload assignment of pointer to Pointer.
template <class T, int size>
T *Pointer<T, size>::operator=(T *t)
{
	typename std::list<PtrDetails<T> >::iterator p;

	// decrement the reference count of this object
	p = findPtrInfo(addr);
	p->refCount--;

	// check to see if the pointer is in the list already
	p = findPtrInfo(t);

	if (p != refContainer.end()) {
		// it IS in the list; increment count
		p->refCount++;
	} else {
		// it is not in the list; create new
		PtrDetails<T> temp(t, size); // Note: refCount incremented in PtrDetails

		// add to the container
		refContainer.push_back(temp);
	}

	p = findPtrInfo(t);
	setLocalData(p);

	return *this;
}

// Overload assignment of Pointer to Pointer.
template <class T, int size>
Pointer<T, size> &Pointer<T, size>::operator=(Pointer &rv)
{
	typename std::list<PtrDetails<T> >::iterator p;

	// First, decrement the reference count
	// for the memory currently being pointed to.
	p = findPtrInfo(addr);
	p->refCount--;

	// Then, increment the reference count of
	// the new address.
	p = findPtrInfo(rv.addr);

	// set local data with new addr data
	setLocalData(p);

	return *this;
}


// A utility function that sets the class data members
template<class T, int size>
void Pointer<T, size>::setLocalData(const typename std::list<PtrDetails<T>>::iterator &p)
{
	this->addr = p->memPtr;
	this->isArray = p->isArray;
	this->arraySize = p->arraySize;
}

// A utility function that displays refContainer.
template <class T, int size>
void Pointer<T, size>::showlist()
{
    typename std::list<PtrDetails<T> >::iterator p;
    std::cout << "refContainer<" << typeid(T).name() << ", " << size << ">:\n";
    std::cout << "memPtr refcount value\n ";
    if (refContainer.begin() == refContainer.end())
    {
        std::cout << " Container is empty!\n\n ";
    }
    for (p = refContainer.begin(); p != refContainer.end(); p++)
    {
        std::cout << "[" << (void *)p->memPtr << "]"
             << " " << p->refCount << " ";
        if (p->memPtr)
            std::cout << " " << *p->memPtr;
        else
            std::cout << "---";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// Find a pointer in refContainer.
template <class T, int size>
typename std::list<PtrDetails<T> >::iterator
Pointer<T, size>::findPtrInfo(T *ptr)
{
    typename std::list<PtrDetails<T> >::iterator p;
    // Find ptr in refContainer.
    for (p = refContainer.begin(); p != refContainer.end(); p++)
        if (p->memPtr == ptr)
            return p;
    return p;
}

// Clear refContainer when program exits.
template <class T, int size>
void Pointer<T, size>::shutdown()
{
    if (refContainerSize() == 0)
        return; // list is empty
    typename std::list<PtrDetails<T> >::iterator p;
    for (p = refContainer.begin(); p != refContainer.end(); p++)
    {
        // Set all reference counts to zero
        p->refCount = 0;
    }
    collect();
}