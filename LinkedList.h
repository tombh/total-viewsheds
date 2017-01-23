/*
AUTHORS: 
*SIHAM TABIK
*LUIS FELIPE ROMERO GOMEZ
*ANTONIO MANUEL RODRIGUEZ CERVILLA 
DEPT: ARQUITECTURA DE LOS COMPUTADORES
UNIVERSIDAD DE MALAGA
 
*/

/*
THIS CODE CREATE THE STRUCT LINKEDLIST 
*/


#ifndef LINKEDLIST_H
#define LINKEDLIST_H


class LinkedList
{ //List linked
			
	public:
	int Size; // Size of list
	int First;// First node list
	int Last; // Last node of list
	int Head; // Node head list  
	int Tail; // Node Tail list
	
	int Count; //=0;
			
	LinkedListNode *LL; // array of node 

	LinkedList();					// Create de Linked List 
	LinkedList(int x);				// Create de Linked List with size x
	void Clear();					// Initializing the Linked list and the first node
	void move_queue(bool movehead, bool movetail);	// Circular queue behavior
	int Next(int j);				// Return which is the next node
	int Prev(int j);				// Return which is the Previous node
	void Add(node node, int pos, bool remove);	// Add new node to linked list 
	void AddFirst(node node, bool remove);		// Add new node to linked list, at the beginning
	void AddLast(node node, bool remove);		// Add new node to linked list, at the end
	void FirstNode(node node);			// Add new node to linked list
	void Remove_two();				// Remove two node of list when we are exit the list 
	void Remove_one();				// Remove two node of list when we are exit the list 
	void removelinks(int prv, int nxt);
	void simpleinsert(int pos);

};

#endif
