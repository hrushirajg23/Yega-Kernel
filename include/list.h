#ifndef _GLIST_H
#define _GLIST_H

#include <stddef.h>


#define offset_of(a,b)     ((int)(&(((a*)(0))->b)))

#define container_of(ptr, type, member) ({\
        (type*)( (char*)(ptr) - offset_of(type, member));\
        })


struct list_head{
    struct list_head* next;
    struct list_head* prev;
};

static inline int list_is_head(const struct list_head* list, const struct list_head* head)
{
    return list == head;
}
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/* this is the dummy node initialization*/
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define list_for_each(pos, head) \
	for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)

#define list_for_each_del(temp, head) \
    struct list_head *save = NULL ;\
    for (temp = (head)->next, save = temp->next; !list_is_head(temp, (head)); temp = save, save = save->next)

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_entry_is_head(pos, head, member) \
    list_is_head(&pos->member, (head))

#define list_for_each_entry(pos, head, member) \
    for (pos = list_first_entry(head, typeof(*pos), member); \
            !list_entry_is_head(pos, head, member); \
            pos = list_next_entry(pos, member))

void generic_add(struct list_head* beg, struct list_head* mid, struct list_head* end);

void generic_del(struct list_head *mid);

void list_add(struct list_head *head, struct list_head *new);

void list_add_tail(struct list_head *head, struct list_head *new);

void list_del(struct list_head* entry);

void list_del_init(struct list_head* entry);

int list_is_empty(struct list_head* head);

void INIT_LIST_HEAD(struct list_head* list);

void INIT_LIST_NULL(struct list_head* node);


// void INIT_LIST_HEAD(struct list_head* list)
// {
//     list->next = list;
//     list->prev = list;
// }

// void INIT_LIST_NULL(struct list_head* node)
// {
//     node->next = NULL;
//     node->prev = NULL;
// }
// void generic_add(struct list_head* beg, struct list_head* mid, struct list_head* end)
// {
//     mid->next = end;
//     mid->prev = beg;
//     beg->next = mid;
//     end->prev = mid;
// }

// void generic_del(struct list_head *mid)
// {
//     mid->next->prev = mid->prev;
//     mid->prev->next = mid->next;
// }

// void list_add(struct list_head *head, struct list_head *new)
// {
//     generic_add(head, new, head->next);
// }
// void list_add_tail(struct list_head *head, struct list_head *new)
// {
//     generic_add(head->next, new, head);
// }

// void list_del(struct list_head* entry)
// {
//     generic_del(entry);
// }

// void list_del_init(struct list_head* entry)
// {
//     generic_del(entry);
//     INIT_LIST_HEAD(entry);
// }

// int list_is_empty(struct list_head* head)
// {
//     if(head->next == head && head->prev == head){
//         return 1;
//     }
//     else{
//         return 0;
//     }
// }

#endif
