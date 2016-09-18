#ifndef TOPOLOGY_H_
#define TOPOLOGY_H_

#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <list>
#include <string>

#define NAMLEN 128
#define MAX 45

#define STAR_IN 'i'
#define STAR_OUT 'o'
#define STAR_BI 's'

#define BIDIREC 's'
#define UNIDIREC 'a'

#define YYDEBUG 1

#ifdef DEBUG
#  define debug(x) x
#else
#  define debug(x)
#endif

struct node {
	std::string name;
	std::list<std::string> adj_list;
};

void add_neighbour(struct node *curr_node, std::string neighbour);
int add_ring(std::string family_name, int n, char mode=BIDIREC);
int add_linear(std::string family_name, int n, char mode=BIDIREC);
void add_complete(std::string family_name, int n);
int add_star(std::string family_name, int n, char mode=STAR_BI);
void print_node(struct node curr_node);
void output_to_file();
struct node* node_lookup(std::string name);
void add_connection(struct node &sender, struct node &receiver, int symmetric=0);
void delete_connection(struct node &sender, struct node &receiver, int symmetric=0);
int node_check(int n);

#endif
