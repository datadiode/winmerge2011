#define UNI_SUR_MIN         0xD800
#define UNI_SUR_HIGH_START  0xD800
#define UNI_SUR_HIGH_END    0xDBFF
#define UNI_SUR_LOW_START   0xDC00
#define UNI_SUR_LOW_END     0xDFFF
#define UNI_SUR_MAX         0xDFFF

EXTERN_C int mk_wcwidth(UINT ucs);
EXTERN_C int mk_wcwidth_cjk(UINT ucs);
