#ifndef DNC_DEBUG_H
#define DNC_DEBUG_H


#ifdef ENABLE_ASSERT
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) ;
#endif

#ifdef ENABLE_UNIMPLEMENTED_ASSERT
#define UNIMPLEMENTED() ASSERT(false)
#else
#define UNIMPLEMENTED() ;
#endif

#endif //DNC_DEBUG_H
