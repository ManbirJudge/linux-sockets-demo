#ifndef LIST_H
#define LIST_H

#define DECLARE_LIST(T, name)                                   \
typedef struct {                                                \
    T *data;                                                    \
    size_t size;                                                \
    size_t cap;                                                 \
} name;                                                         \
                                                                \
void name##_init(name *list) {                                  \
    list->data = NULL;                                          \
    list->size = 0;                                             \
    list->cap = 0;                                              \
}                                                               \
                                                                \
void name##_push(name *list, T val) {                           \
    if (list->size == list->cap) {                              \
        size_t new_cap = list->cap ? list->cap * 2 : 8;         \
        /* TODO: add null check */                              \
        list->data = realloc(list->data, new_cap * sizeof(T));  \
        list->cap = new_cap;                                    \
    }                                                           \
                                                                \
    list->data[list->size++] = val;                             \
}                                                               \
                                                                \
T name##_at(name *list, size_t i) {                             \
    return list->data[i];                                       \
}                                                               \
                                                                \
const T* name##_ptr_at(name *list, size_t i) {                  \
    return list->data + i;                                      \
}                                                               \
                                                                \
T name##_pop(name *list) {                                      \
    T val = {0};                                                \
                                                                \
    if (list->size == 0) return val;                            \
                                                                \
    val = list->data[list->size - 1];                           \
    list->size--;                                               \
                                                                \
    return val;                                                 \
}                                                               \
                                                                \
void name##_clear(name *list) {                                 \
    list->size = 0;                                             \
}                                                               \
                                                                \
void name##_free(name *list) {                                  \
    if (list) {                                                 \
        if (list->data) {                                       \
            free(list->data);                                   \
            list->data = NULL;                                  \
            list->cap = 0;                                      \
            list->size = 0;                                     \
        }                                                       \
    }                                                           \
}                                                               \

#endif