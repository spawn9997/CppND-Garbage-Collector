// This class defines an element that is stored
// in the garbage collection information list.
//
template <class T>
class PtrDetails
{
  public:
    unsigned int refCount = 0; // current reference count

	T *memPtr = nullptr;         // pointer to allocated memory

    /* isArray is true if memPtr points
to an allocated array. It is false
otherwise. */
    bool isArray = false; // true if pointing to array

    /* If memPtr is pointing to an allocated
array, then arraySize contains its size */
	unsigned int arraySize = 0; // size of array

    // Here, mPtr points to the allocated memory.
    // If this is an array, then size specifies
    // the size of the array.

    PtrDetails(T *ptr, int size = 0):memPtr(ptr), arraySize(size), refCount(1)
    {
		if (arraySize > 0) {
			isArray = true;
		}
    }
};
// Overloading operator== allows two class objects to be compared.
// This is needed by the STL list class.
template <class T>
bool operator==(const PtrDetails<T> &obj_1,
                const PtrDetails<T> &obj_2)
{
	return (obj_1.memPtr == obj_2.memPtr);
}