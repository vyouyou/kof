/******************************************************************************* 
author : ljc
version : <1.0> 
-------------------------------------------------------------------------------- 
info : 
-------------------------------------------------------------------------------- 
*******************************************************************************/
#include "ccCArray.h"


/** Allocates and initializes a new array with specified capacity */
ccArray* ccArrayNew(unsigned int capacity) 
{
	if (capacity == 0)
		capacity = 1;
	
	ccArray *arr = (ccArray*)malloc( sizeof(ccArray) );
	arr->num = 0;
	arr->arr =  (CObject**)calloc(capacity, sizeof(CObject*));
	arr->max = capacity;
	
	return arr;
}

/** Frees array after removing all remaining objects. Silently ignores NULL arr. */
void ccArrayFree(ccArray*& arr)
{
    if( arr == NULL ) 
    {
        return;
    }
	ccArrayRemoveAllObjects(arr);
	
	free(arr->arr);
	free(arr);

    arr = NULL;
}

void ccArrayDoubleCapacity(ccArray *arr)
{
	arr->max *= 2;
	CObject** newArr = (CObject**)realloc( arr->arr, arr->max * sizeof(CObject*) );
	// will fail when there's not enough memory
    CAssert(newArr != 0, "ccArrayDoubleCapacity failed. Not enough memory");
	arr->arr = newArr;
}

void ccArrayEnsureExtraCapacity(ccArray *arr, unsigned int extra)
{
	while (arr->max < arr->num + extra)
    {
		ccArrayDoubleCapacity(arr);
    }
}

void ccArrayShrink(ccArray *arr)
{
    unsigned int newSize = 0;
	
	//only resize when necessary
	if (arr->max > arr->num && !(arr->num==0 && arr->max==1))
	{
		if (arr->num!=0)
		{
			newSize=arr->num;
			arr->max=arr->num;
		}
		else
		{//minimum capacity of 1, with 0 elements the array would be free'd by realloc
			newSize=1;
			arr->max=1;
		}
		
		arr->arr = (CObject**)realloc(arr->arr,newSize * sizeof(CObject*) );
		CAssert(arr->arr!=NULL,"could not reallocate the memory");
	}
}

/** Returns index of first occurrence of object, CC_INVALID_INDEX if object not found. */
unsigned int ccArrayGetIndexOfObject(ccArray *arr, CObject* object)
{
    const unsigned int arrNum = arr->num;
    CObject** ptr = arr->arr;
	for(unsigned int i = 0; i < arrNum; ++i, ++ptr)
    {
		if( *ptr == object ) return i;
    }
    
	return CC_INVALID_INDEX;
}

/** Returns a Boolean value that indicates whether object is present in array. */
bool ccArrayContainsObject(ccArray *arr, CObject* object)
{
	return ccArrayGetIndexOfObject(arr, object) != CC_INVALID_INDEX;
}

/** Appends an object. Behavior undefined if array doesn't have enough capacity. */
void ccArrayAppendObject(ccArray *arr, CObject* object)
{
    CAssert(object != NULL, "Invalid parameter!");
    object->retain();
	arr->arr[arr->num] = object;
	arr->num++;
}

/** Appends an object. Capacity of arr is increased if needed. */
void ccArrayAppendObjectWithResize(ccArray *arr, CObject* object)
{
	ccArrayEnsureExtraCapacity(arr, 1);
	ccArrayAppendObject(arr, object);
}

/** Appends objects from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ccArrayAppendArray(ccArray *arr, ccArray *plusArr)
{
	for(unsigned int i = 0; i < plusArr->num; i++)
    {
		ccArrayAppendObject(arr, plusArr->arr[i]);
    }
}

/** Appends objects from plusArr to arr. Capacity of arr is increased if needed. */
void ccArrayAppendArrayWithResize(ccArray *arr, ccArray *plusArr)
{
	ccArrayEnsureExtraCapacity(arr, plusArr->num);
	ccArrayAppendArray(arr, plusArr);
}

/** Inserts an object at index */
void ccArrayInsertObjectAtIndex(ccArray *arr, CObject* object, unsigned int index)
{
	CAssert(index<=arr->num, "Invalid index. Out of bounds");
	CAssert(object != NULL, "Invalid parameter!");

	ccArrayEnsureExtraCapacity(arr, 1);
	
	unsigned int remaining = arr->num - index;
	if( remaining > 0)
    {
		memmove((void *)&arr->arr[index+1], (void *)&arr->arr[index], sizeof(CObject*) * remaining );
    }

    object->retain();
	arr->arr[index] = object;
	arr->num++;
}

/** Swaps two objects */
void ccArraySwapObjectsAtIndexes(ccArray *arr, unsigned int index1, unsigned int index2)
{
	CAssert(index1 < arr->num, "(1) Invalid index. Out of bounds");
	CAssert(index2 < arr->num, "(2) Invalid index. Out of bounds");
	
	CObject* object1 = arr->arr[index1];
	
	arr->arr[index1] = arr->arr[index2];
	arr->arr[index2] = object1;
}

/** Removes all objects from arr */
void ccArrayRemoveAllObjects(ccArray *arr)
{
	while( arr->num > 0 )
    {
		(arr->arr[--arr->num])->release();
    }
}

/** Removes object at specified index and pushes back all subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ccArrayRemoveObjectAtIndex(ccArray *arr, unsigned int index, bool bReleaseObj/* = true*/)
{
    CAssert(arr && arr->num > 0 && index < arr->num, "Invalid index. Out of bounds");
    if (bReleaseObj)
    {
        CC_SAFE_RELEASE(arr->arr[index]);
    }
    
	arr->num--;
	
	unsigned int remaining = arr->num - index;
	if(remaining>0)
    {
		memmove((void *)&arr->arr[index], (void *)&arr->arr[index+1], remaining * sizeof(CObject*));
    }
}

/** Removes object at specified index and fills the gap with the last object,
 thereby avoiding the need to push back subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ccArrayFastRemoveObjectAtIndex(ccArray *arr, unsigned int index)
{
	CC_SAFE_RELEASE(arr->arr[index]);
	unsigned int last = --arr->num;
	arr->arr[index] = arr->arr[last];
}

void ccArrayFastRemoveObject(ccArray *arr, CObject* object)
{
	unsigned int index = ccArrayGetIndexOfObject(arr, object);
	if (index != CC_INVALID_INDEX)
    {
		ccArrayFastRemoveObjectAtIndex(arr, index);
    }
}

/** Searches for the first occurrence of object and removes it. If object is not
 found the function has no effect. */
void ccArrayRemoveObject(ccArray *arr, CObject* object, bool bReleaseObj/* = true*/)
{
	unsigned int index = ccArrayGetIndexOfObject(arr, object);
	if (index != CC_INVALID_INDEX)
    {
		ccArrayRemoveObjectAtIndex(arr, index, bReleaseObj);
    }
}

/** Removes from arr all objects in minusArr. For each object in minusArr, the
 first matching instance in arr will be removed. */
void ccArrayRemoveArray(ccArray *arr, ccArray *minusArr)
{
	for(unsigned int i = 0; i < minusArr->num; i++)
    {
		ccArrayRemoveObject(arr, minusArr->arr[i]);
    }
}

/** Removes from arr all objects in minusArr. For each object in minusArr, all
 matching instances in arr will be removed. */
void ccArrayFullRemoveArray(ccArray *arr, ccArray *minusArr)
{
	unsigned int back = 0;
	unsigned int i = 0;
	
	for( i = 0; i < arr->num; i++) 
    {
		if( ccArrayContainsObject(minusArr, arr->arr[i]) ) 
        {
			CC_SAFE_RELEASE(arr->arr[i]);
			back++;
		} 
        else
        {
			arr->arr[i - back] = arr->arr[i];
        }
	}
	
	arr->num -= back;
}

// #pragma mark -
// #pragma mark ccCArray for Values (c structures)

/** Allocates and initializes a new C array with specified capacity */
ccCArray* ccCArrayNew(unsigned int capacity)
{
	if (capacity == 0)
    {
		capacity = 1;
    }

	ccCArray *arr = (ccCArray*)malloc( sizeof(ccCArray) );
	arr->num = 0;
	arr->arr = (void**)malloc( capacity * sizeof(void*) );
	arr->max = capacity;
	
	return arr;
}

/** Frees C array after removing all remaining values. Silently ignores NULL arr. */
void ccCArrayFree(ccCArray *arr)
{
    if( arr == NULL ) 
    {
        return;
    }
	ccCArrayRemoveAllValues(arr);
	
	free(arr->arr);
	free(arr);
}

/** Doubles C array capacity */
void ccCArrayDoubleCapacity(ccCArray *arr)
{
    ccArrayDoubleCapacity((ccArray*)arr);
}

/** Increases array capacity such that max >= num + extra. */
void ccCArrayEnsureExtraCapacity(ccCArray *arr, unsigned int extra)
{
    ccArrayEnsureExtraCapacity((ccArray*)arr,extra);
}

/** Returns index of first occurrence of value, CC_INVALID_INDEX if value not found. */
unsigned int ccCArrayGetIndexOfValue(ccCArray *arr, void* value)
{
	unsigned int i;
	
	for( i = 0; i < arr->num; i++)
    {
		if( arr->arr[i] == value ) return i;
    }
	return CC_INVALID_INDEX;
}

/** Returns a Boolean value that indicates whether value is present in the C array. */
bool ccCArrayContainsValue(ccCArray *arr, void* value)
{
	return ccCArrayGetIndexOfValue(arr, value) != CC_INVALID_INDEX;
}

/** Inserts a value at a certain position. Behavior undefined if array doesn't have enough capacity */
void ccCArrayInsertValueAtIndex( ccCArray *arr, void* value, unsigned int index)
{
	CAssert( index < arr->max, "ccCArrayInsertValueAtIndex: invalid index");
	
	unsigned int remaining = arr->num - index;
    // make sure it has enough capacity
    if (arr->num + 1 == arr->max)
    {
        ccCArrayDoubleCapacity(arr);
    }
	// last Value doesn't need to be moved
	if( remaining > 0) {
		// tex coordinates
		memmove((void *)&arr->arr[index+1], (void *)&arr->arr[index], sizeof(void*) * remaining );
	}
	
	arr->num++;
	arr->arr[index] = value;
}

/** Appends an value. Behavior undefined if array doesn't have enough capacity. */
void ccCArrayAppendValue(ccCArray *arr, void* value)
{
	arr->arr[arr->num] = value;
	arr->num++;
    // double the capacity for the next append action
    // if the num >= max
    if (arr->num >= arr->max)
    {
        ccCArrayDoubleCapacity(arr);
    }
}

/** Appends an value. Capacity of arr is increased if needed. */
void ccCArrayAppendValueWithResize(ccCArray *arr, void* value)
{
	ccCArrayEnsureExtraCapacity(arr, 1);
	ccCArrayAppendValue(arr, value);
}


/** Appends values from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ccCArrayAppendArray(ccCArray *arr, ccCArray *plusArr)
{
	unsigned int i;
	
	for( i = 0; i < plusArr->num; i++)
    {
		ccCArrayAppendValue(arr, plusArr->arr[i]);
    }
}

/** Appends values from plusArr to arr. Capacity of arr is increased if needed. */
void ccCArrayAppendArrayWithResize(ccCArray *arr, ccCArray *plusArr)
{
	ccCArrayEnsureExtraCapacity(arr, plusArr->num);
	ccCArrayAppendArray(arr, plusArr);
}

/** Removes all values from arr */
void ccCArrayRemoveAllValues(ccCArray *arr)
{
	arr->num = 0;
}

/** Removes value at specified index and pushes back all subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ccCArrayRemoveValueAtIndex(ccCArray *arr, unsigned int index)
{
	unsigned int last;
	
	for( last = --arr->num; index < last; index++)
    {
		arr->arr[index] = arr->arr[index + 1];
    }
}

/** Removes value at specified index and fills the gap with the last value,
 thereby avoiding the need to push back subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ccCArrayFastRemoveValueAtIndex(ccCArray *arr, unsigned int index)
{
	unsigned int last = --arr->num;
	arr->arr[index] = arr->arr[last];
}

/** Searches for the first occurrence of value and removes it. If value is not found the function has no effect.
 @since v0.99.4
 */
void ccCArrayRemoveValue(ccCArray *arr, void* value)
{
	unsigned int index = ccCArrayGetIndexOfValue(arr, value);
	if (index != CC_INVALID_INDEX)
    {
		ccCArrayRemoveValueAtIndex(arr, index);
    }
}

/** Removes from arr all values in minusArr. For each Value in minusArr, the first matching instance in arr will be removed.
 @since v0.99.4
 */
void ccCArrayRemoveArray(ccCArray *arr, ccCArray *minusArr)
{
	for(unsigned int i = 0; i < minusArr->num; i++)
    {
		ccCArrayRemoveValue(arr, minusArr->arr[i]);
    }
}

/** Removes from arr all values in minusArr. For each value in minusArr, all matching instances in arr will be removed.
 @since v0.99.4
 */
void ccCArrayFullRemoveArray(ccCArray *arr, ccCArray *minusArr)
{
	unsigned int back = 0;
	
	for(unsigned int i = 0; i < arr->num; i++) 
    {
		if( ccCArrayContainsValue(minusArr, arr->arr[i]) ) 
        {
			back++;
		} 
        else
        {
			arr->arr[i - back] = arr->arr[i];
        }
	}
	
	arr->num -= back;
}
