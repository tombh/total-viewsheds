#ifndef LINKEDLIST_H
#define LINKEDLIST_H

class LinkedList {
 public:
  struct node {
    // Identifier of point.
    int idx;
    // Position of node relative to rotated sector y-axis. Used when
    // deciding the position of a new node in the Band of Sight.
    int sight_ordered_index;
    // Height of point in DEM (scaled relative to 1 unit of each DEM)
    float h;
    // Distance from sector's base x-axis
    float d;
  };

  struct LinkedListNode {
   public:
    LinkedList::node Value;
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
  void Clear();       // Initializing the Linked list and the first node
  void move_queue(bool movehead, bool movetail);  // Circular queue behavior
  int Next(int j);  // Return which is the next node
  int Prev(int j);  // Return which is the Previous node
  void Add(node node, int pos, bool remove);  // Add new node to linked list
  void AddFirst(node node,
                bool remove);  // Add new node to linked list, at the beginning
  void AddLast(node node,
               bool remove);  // Add new node to linked list, at the end
  void FirstNode(node node);  // Add new node to linked list
  void Remove_one();  // Remove two node of list when we are exit the list
  void removelinks(int prv, int nxt);
  void simpleinsert(int pos);
};

#endif
