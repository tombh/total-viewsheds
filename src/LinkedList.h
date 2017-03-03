#ifndef LINKEDLIST_H
#define LINKEDLIST_H

class LinkedList {
 public:
  struct LinkedListNode {
   public:
    int dem_id;
    short next;  // Identifier the next node in Linked List.
    short prev;  // Identifier the Previous node in Linked List.
  };

  int Size;   // Size of list
  int First;  // First node list
  int Last;   // Last node of list
  int Head;   // Node head list
  int Tail;   // Node Tail list

  int Count;  //=0;

  LinkedListNode *LL;  // array of node

  LinkedList(int x);  // Create de Linked List with size x
  ~LinkedList();
  void Clear();                                   // Initializing the Linked list and the first node
  void move_queue(bool movehead, bool movetail);  // Circular queue behavior
  int Next(int j);                                // Return which is the next node
  int Prev(int j);                                // Return which is the Previous node
  void Add(int dem_id, int pos, bool remove);     // Add new node to linked list
  void AddFirst(int dem_id, bool remove);         // Add new node to linked list, at the beginning
  void AddLast(int dem_id, bool remove);          // Add new node to linked list, at the end
  void FirstNode(int dem_id);                     // Add new node to linked list
  void Remove_one();                              // Remove two node of list when we are exit the list
  void removelinks(int prv, int nxt);
  void simpleinsert(int pos);
};

#endif
