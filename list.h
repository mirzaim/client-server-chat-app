
#define initHead() = {.next = NULL, .prev = NULL};

//add
#define addNode(node, head)                 \
        node->next = head->next;            \
        node->prev = head;                  \
        head->next->prev = &node;           \
        head->next = &node;

//delete
#define deleteNode(node, head)              \
        node->prev->next = node->next;      \
        node->next->prev = node->prev;

//foreach
#define foreach(head, entry)                                          \
        for(entry = head->next; entry; entry = entry->next)

