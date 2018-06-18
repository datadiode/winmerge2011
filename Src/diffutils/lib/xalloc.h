extern void die (...);
#define xalloc_die die
extern void *xmalloc (size_t);
extern void *xrealloc (void *, size_t);

#define new because_new_is_not_a_keyword_in_c99
#define this because_this_is_not_a_keyword_in_c99
