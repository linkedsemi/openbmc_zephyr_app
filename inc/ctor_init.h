#ifndef CTOR_INIT_H_
#define CTOR_INIT_H_
#include <zephyr/sys/iterable_sections.h>
typedef void (*ctor_fn_t)();
#define CTOR_SECTION_ITERABLE(secname,func) \
    const static TYPE_SECTION_ITERABLE(ctor_fn_t,func##_ptr,secname,func##_ptr) = func;

#endif
