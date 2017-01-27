#include "LinkedList.h"
#include <plog/Log.h>

LinkedList::LinkedList() {}

LinkedList::LinkedList(int x) {
  LL = new LinkedListNode[x];
  Size = x;
  LinkedList::Clear();
  Head = 0;
  Tail = 0;
  Count = 0;
  First = 0;
  Last = 0;
  LL[0].next = -1;
  LL[0].prev = -2;
}

LinkedList::~LinkedList() {
  // TODO: Find out who is depending on LL apart from Sector::band_of_sight.
  //delete [] LL;
}

void LinkedList::Clear() {
  Head = 0;
  Tail = 0;
  Count = 0;
  First = 0;
  Last = 0;
  LL[0].next = -1;
  LL[0].prev = -2;
}

void LinkedList::move_queue(bool movehead, bool movetail) {
  if (movehead) Head = (Head + 1) % Size;
  if (movetail) Tail = (Tail + 1) % Size;
  if (movehead && !movetail) Count++;
  if (!movehead && movetail) Count--;
}

int LinkedList::Next(int j) { return LL[j].next; }

int LinkedList::Prev(int j) { return LL[j].prev; }

void LinkedList::Add(node node, int pos, bool remove) {
  int tn = -3, tp = -3;
  bool removing_first, removing_last, replace = false;
  if (remove) {
    tn = LL[Tail].next;
    tp = LL[Tail].prev;
    removing_first = (tp == -2);
    removing_last = (tn == -1);
    replace = (Tail == pos) || (LL[Tail].prev == pos);
  }

  LL[Head].Value = node;  // by value

  if (!remove) {
    simpleinsert(pos);
    move_queue(true, false);
  } else {
    if (replace)  // Position of newnode is removed node
    {
      move_queue(true, true);
    } else {
      simpleinsert(pos);
      removelinks(tp, tn);
      move_queue(true, true);
    }
  }
}

void LinkedList::FirstNode(node node) {
  LL[Head].Value = node;
  move_queue(true, false);
}

void LinkedList::AddFirst(node node, bool remove) {
  int tp = -3;
  int tn = -3;
  tn = LL[Tail].next;
  tp = LL[Tail].prev;
  bool removing_first = (tp == -2) && remove;
  if (removing_first) tp = Head;

  LL[Head].Value = node;
  LL[Head].prev = -2;
  if (!removing_first) {
    LL[Head].next = First;
    LL[First].prev = Head;
  }
  if (remove) removelinks(tp, tn);  // remove first
  First = Head;
  move_queue(true, remove);
}

void LinkedList::AddLast(node node, bool remove) {
  int tp = -3;
  int tn = -3;
  tn = LL[Tail].next;
  tp = LL[Tail].prev;
  bool removing_last = (tn == -1) && remove;
  if (removing_last) tn = Head;

  LL[Head].Value = node;
  LL[Head].next = -1;
  if (!removing_last) {
    LL[Head].prev = Last;
    LL[Last].next = Head;
  }
  if (remove) removelinks(tp, tn);  // remove first
  Last = Head;
  move_queue(true, remove);
}

void LinkedList::Remove_one() {
  int tp = LL[Tail].prev;
  int tn = LL[Tail].next;
  removelinks(tp, tn);
}

void LinkedList::Remove_two() {
  Remove_one();
  move_queue(false, true);
  Remove_one();
  move_queue(false, true);
}

void LinkedList::simpleinsert(int pos) {
  LL[Head].prev = pos;
  LL[Head].next = LL[pos].next;
  LL[pos].next = Head;
  LL[LL[Head].next].prev = Head;
}

void LinkedList::removelinks(int prv, int nxt) {
  if (prv != -2)
    LL[prv].next = nxt;
  else
    First = nxt;
  if (nxt != -1)
    LL[nxt].prev = prv;
  else
    Last = prv;
}
