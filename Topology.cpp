#include "Topology.hpp"

std::list<struct node> nodes;

int node_check(int n) {
  if (nodes.size() + n > MAX)
    return 0;
  return 1;
}

void error_message(std::string msg) {
  std::cerr << msg;
  exit(1);
}

/**************************************************************/
/*	initialize_node - function takes a node to which a    */
/*			neighbour needs to be added, and the  */
/*			name of the neighbour, along with     */
/*			a suffix, if necessary. The suffix is */
/*			a default argument, set at -1. If no  */
/*			suffix is given, the neighbour is     */
/*			added as is.			      */
/**************************************************************/

struct node initialize_node(std::string name, int suffix=-1) {
  struct node new_node;

  /* Initialize name */
  if (suffix != -1)
    name = name + "_" + std::to_string(suffix);

  new_node.name = name;

  /* Clear adjacency list */
  new_node.adj_list.clear();

  debug(std::cout << "\nName: \n" << new_node.name);

  return new_node;
}

/**************************************************************/
/*	add_neighbour - function takes a node to which a      */
/*			neighbour needs to be added, and the  */
/*			name of the neighbour, along with     */
/*			a suffix, if necessary. The suffix is */
/*			a default argument, set at -1. If no  */
/*			suffix is given, the neighbour is     */
/*			added as is.			      */
/**************************************************************/
void add_neighbour(struct node &curr_node, std::string name, int suffix=-1) {
  std::string neighbour_string;

  if (suffix == -1)
    neighbour_string = name;

  else {
    neighbour_string = name + "_" + std::to_string(suffix);
  }

  //TODO: Check for dups in the adjacency list
  curr_node.adj_list.push_back(neighbour_string);
}

void delete_neighbour(struct node &curr_node, std::string name, int suffix=-1) {
  if (suffix == -1)
    name = name;

  else {
    name = name + "_" + std::to_string(suffix);
  }

  curr_node.adj_list.erase(std::remove_if(curr_node.adj_list.begin(),
					  curr_node.adj_list.end(),
					  [=](std::string neighbour) {
						return neighbour == name;
					   }),
			   curr_node.adj_list.end());
}

/**************************************************************/
/*	add_ring - function takes a family_name	and the       */
/*		   number of nodes in the ring, and adds all  */
/*		   nodes to the global nodes list	      */
/**************************************************************/

int add_ring(std::string family_name, int n, char mode) {
  int i;				/* index for nodes */
  struct node new_node;			/* struct to hold new node */

  debug(std::cout << "family: " << family_name);

  if (mode != BIDIREC && mode != UNIDIREC) {
    std::cout << "Invalid mode" << std::endl;
    return 1;
  }

  /* Loop through nodes in ring */

  for (i = 0; i < n; i++) {

    /* Check for node limit */
    //if(!node_check(nodes.size()))
    //  error_message("Node limit exceeded");

    /* Initialize node */
    new_node = initialize_node(family_name, i);

    switch (mode) {
    case BIDIREC:
      /* Add first neighbour */
      add_neighbour(new_node, family_name, (((i-1) % n) + n)%n);

    case UNIDIREC:
      /* Add second neighbour */
      add_neighbour(new_node, family_name, (i+1) % n);
    }

    /* Push node on to global nodes list */
    nodes.push_back(new_node);

    /* Print details of node just added */
    print_node(nodes.back());
  }
  return 0;
}

/**************************************************************/
/*	add_linear - function takes a family_name and the     */
/*		   number of nodes in the bus, and adds all   */
/*		   nodes to the global nodes list	      */
/**************************************************************/

int add_linear(std::string family_name, int n, char mode) {
  int i = 0;				/* index for nodes */
  struct node new_node;			/* struct to hold new node */

  debug(std::cout << "family: " << family_name);

  if (mode != BIDIREC && mode != UNIDIREC) {
    std::cout << "Invalid mode" << std::endl;
    return 1;
  }

  /* For the first node */

  /* Check for node limit */
  //if(!node_check(nodes.size()))
  //  error_message("Node limit exceeded");

  /* Initialize node */
  new_node = initialize_node(family_name, i);

  /* Add first neighbour */
  add_neighbour(new_node, family_name, ((i+1) % n));

  /* Push node on to global nodes list */
  nodes.push_back(new_node);

  /* Print details of node just added */
  print_node(nodes.back());

  /* Loop through nodes in bus */

  for (i = 1; i < n-1; i++) {

    /* Check for node limit */
    //if(!node_check(nodes.size()))
    //  error_message("Node limit exceeded");

    /* Initialize next node */
    new_node = initialize_node(family_name, i);

    switch (mode) {
    case BIDIREC:
      /* Add first neighbour */
      add_neighbour(new_node, family_name, (((i-1) % n) + n) % n);

    case UNIDIREC:
      /* Add second neighbour */
      add_neighbour(new_node, family_name, (i+1) % n);

    }
    /* Push node on to global nodes list */
    nodes.push_back(new_node);

    /* Print details of node just added */
    print_node(nodes.back());
  }

  /* Last node */

  /* Check for node limit */
  //if(!node_check(nodes.size()))
  //  error_message("Node limit exceeded");

  /* Initialize node */
  new_node = initialize_node(family_name, i);

  if (mode == BIDIREC)
    /* Add first neighbour */
    add_neighbour(new_node, family_name, (((i-1) % n) + n) % n);

  /* Push node on to global nodes list */
  nodes.push_back(new_node);

  /* Print details of node just added */
  print_node(nodes.back());
  return 0;
}

/**************************************************************/
/*	add_complete - function takes a family_name and the   */
/*		       number of nodes in the complete graph, */
/*		       and adds all nodes to the global nodes */
/*		       list.				      */
/**************************************************************/

void add_complete(std::string family_name, int n) {
  int i, j;				/* indices for nodes */
  struct node new_node;			/* struct to hold new node */

  debug(std::cout << "family: " << family_name);

  /* Loop through nodes in ring */

  for (i = 0; i < n; i++) {

    /* Check for node limit */
    //if(!node_check(nodes.size()))
    //  error_message("Node limit exceeded");

    /* Insert name of next node */
    new_node = initialize_node(family_name, i);

    /* Add all the neighbours */

    for (j = 0; j < n; j++) {
      /* Don't add a node to its own adjacency list */
      if (i == j)
	continue;

      add_neighbour(new_node, family_name, j);
    }
    /* Push node on to global nodes list */
    nodes.push_back(new_node);

    /* Print details of node just added */
    print_node(nodes.back());
  }
}

/**************************************************************/
/*	add_star - function takes a family_name	and the       */
/*		   number of nodes in the star, and adds all  */
/*		   nodes to the global nodes list	      */
/**************************************************************/

int add_star(std::string family_name, int n, char mode) {
  int i = 0, j;				/* index for nodes */
  struct node new_node;			/* struct to hold new node */

  debug(std::cout << "family: " << family_name);

  if (mode != STAR_BI && mode != STAR_IN && mode != STAR_OUT) {
    std::cout << "Invalid option" << std::endl;
    return 1;
  }
  /* Central node */

  /* Check for node limit */
  //if(!node_check(nodes.size()))
  //  error_message("Node limit exceeded");

  /* Insert name of next node */
  new_node = initialize_node(family_name, i);


  switch(mode) {
  case STAR_BI:
  case STAR_OUT:
    /* Add neighbours */

    for (j = 1; j < n; j++) {
      add_neighbour(new_node, family_name, j);
    }
    break;
  case STAR_IN:
    /* do nothing */
    break;
  default:
    std::cout << "Invalid option";
  }
  /* Push node on to global nodes list */
  nodes.push_back(new_node);

  /* Print details of node just added */
  print_node(nodes.back());

  /* Loop through spokes */

  for (i = 1; i < n; i++) {

    /* Check for node limit */
    //if(!node_check(nodes.size()))
    //  error_message("Node limit exceeded");

    /* Insert name of next node */
    new_node = initialize_node(family_name, i);

    switch(mode) {
    case STAR_BI:
    case STAR_IN:
      /* Add first neighbour */
      add_neighbour(new_node, family_name, 0);

      break;
    case STAR_OUT:
      /* do nothing */
      break;
    }
    /* Push node on to global nodes list */
    nodes.push_back(new_node);

    /* Print details of node just added */
    print_node(nodes.back());
  }
  return 0;
}

/**************************************************************/
/*	print_node - Print the details of a single node       */
/**************************************************************/

void print_node(struct node curr_node) {
  std::cout << "\nNode name: " << curr_node.name;
  std::cout << "\nNumber of adjacent nodes: " << curr_node.adj_list.size();
  std::cout << "\nAdjacent nodes: ";

  for(std::list<std::string>::iterator iter = curr_node.adj_list.begin(); iter != curr_node.adj_list.end(); iter++){
	std::cout << *iter << " ";;
  }

  std::cout << std::endl << std::endl;
}

/**************************************************************/
/*	output_to_file - Write the adjacency lists to file    */
/**************************************************************/

void output_to_file() {
	std::string filename = "output_file";

	std::ofstream fout(filename);

	for(std::list<struct node>::iterator curr_node = nodes.begin(); curr_node != nodes.end(); curr_node++) {
		fout << (*curr_node).name << ":";
		for(std::list<std::string>::iterator curr_element = (*curr_node).adj_list.begin(); curr_element != (*curr_node).adj_list.end(); curr_element++){
			 fout << " " << *curr_element;
		}
		fout << std::endl;
	}
	fout.close();
}


/****************************************************************/
/*	node_lookup - accepts a node's name and checks if it is */
/*		      already there, and if it is, returns the  */
/*		      node, otherwise returns NULL.		*/
/****************************************************************/

struct node* node_lookup(std::string name) {
	for(std::list<struct node>::iterator curr_node = nodes.begin(); curr_node != nodes.end(); curr_node++) {
	  if ((*curr_node).name == name)
	    return &(*curr_node);
	}
	return NULL;
}

/****************************************************************/
/*	add_connection - adds receiver as a neighbour to sender	*/
/*			 Takes sender and receiver as arguments	*/
/*			 along with a symmetry option		*/
/****************************************************************/

void add_connection(struct node &sender, struct node &receiver, int symmetric) {
  add_neighbour(sender, receiver.name);
  if(symmetric) {
	add_neighbour(receiver, sender.name);
  }
}


/****************************************************************/
/*	delete_connection - deletes receiver as a neighbour to sender	*/
/*			 Takes sender and receiver as arguments	*/
/*			 along with a symmetry option		*/
/****************************************************************/

void delete_connection(struct node &sender, struct node &receiver, int symmetric) {
  delete_neighbour(sender, receiver.name);
  if(symmetric) {
	delete_neighbour(receiver, sender.name);
  }
}
