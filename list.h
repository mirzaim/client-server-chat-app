
#define initHead(name) {.next = (&name), .prev = (&name)};


//add
#define addNode(node, head)                     \
        (node)->next = (head)->next;            \
        (node)->prev = (head);                  \
        (head)->next->prev = (node);            \
        (head)->next = (node);

//delete
#define deleteNode(node, head)                  \
        node->prev->next = node->next;          \
        node->next->prev = node->prev;

//foreach
#define foreachOnNode(entry, head)                                          \
        for(entry = (head)->next; entry != head; entry = (entry)->next)

