#include <plog/Log.h>

#include "LinkedList.h"

LinkedList::LinkedList(int x)
  : LL(new LinkedListNode[x]),
    Size(x) {}

LinkedList::~LinkedList() {
  delete[] this->LL;
}

void LinkedList::Clear() {
  this->Count = 0;
  this->Head = 0;
  this->Tail = 0;
  this->First = 0;
  this->Last = 0;
  this->LL[0].next = -1;
  this->LL[0].prev = -2;
}

void LinkedList::move_queue(bool movehead, bool movetail) {
  if (movehead) this->Head = (this->Head + 1) % this->Size;
  if (movetail) this->Tail = (this->Tail + 1) % this->Size;
  if (movehead && !movetail) this->Count++;
  if (!movehead && movetail) this->Count--;
}

int LinkedList::Next(int j) { return LL[j].next; }

int LinkedList::Prev(int j) { return LL[j].prev; }

void LinkedList::Add(node node, int pos, bool remove) {
  int tn = -3, tp = -3;
  bool replace = false;
  if (remove) {
    tn = this->LL[this->Tail].next;
    tp = this->LL[this->Tail].prev;
    replace = (this->Tail == pos) || (this->LL[this->Tail].prev == pos);
  }

  this->LL[this->Head].Value = node;  // by value

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
  this->LL[this->Head].Value = node;
  move_queue(true, false);
}

void LinkedList::AddFirst(node node, bool remove) {
  int tp = this->LL[this->Tail].prev;
  int tn = this->LL[this->Tail].next;
  bool removing_first = (tp == -2) && remove;
  if (removing_first) tp = this->Head;

  this->LL[this->Head].Value = node;
  this->LL[this->Head].prev = -2;
  if (!removing_first) {
    this->LL[this->Head].next = this->First;
    this->LL[this->First].prev = this->Head;
  }
  if (remove) removelinks(tp, tn);  // remove first
  this->First = this->Head;
  move_queue(true, remove);
}

void LinkedList::AddLast(node node, bool remove) {
  int tp = this->LL[this->Tail].prev;
  int tn = this->LL[this->Tail].next;
  bool removing_last = (tn == -1) && remove;
  if (removing_last) tn = this->Head;

  this->LL[this->Head].Value = node;
  this->LL[this->Head].next = -1;
  if (!removing_last) {
    this->LL[this->Head].prev = this->Last;
    this->LL[this->Last].next = this->Head;
  }
  if (remove) removelinks(tp, tn);  // remove first
  this->Last = this->Head;
  move_queue(true, remove);
}

void LinkedList::Remove_one() {
  int tp = this->LL[this->Tail].prev;
  int tn = this->LL[this->Tail].next;
  removelinks(tp, tn);
  move_queue(false, true);
}

void LinkedList::simpleinsert(int pos) {
  this->LL[this->Head].prev = pos;
  this->LL[this->Head].next = this->LL[pos].next;
  this->LL[pos].next = this->Head;
  this->LL[this->LL[this->Head].next].prev = this->Head;
}

void LinkedList::removelinks(int prv, int nxt) {
  if (prv != -2)
    this->LL[prv].next = nxt;
  else
    this->First = nxt;
  if (nxt != -1)
    this->LL[nxt].prev = prv;
  else
    this->Last = prv;
}
