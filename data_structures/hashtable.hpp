#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>

// hashtable node, embedded into payload

class hashtable final
{

public:
  struct node
  {
    std::unique_ptr<node> next = nullptr;
    uint64_t hashcode = 0;
  };

  using node_pointer = std::unique_ptr <node>;
  std::vector <std::unique_ptr <node>> table;
  
  hashtable (size_t n);
  ~hashtable ();


  void decrement_size ();
  void increment_size ();
  size_t get_size() const;
  size_t get_mask() const;
  bool insert (node_pointer& node);
  bool remove (node_pointer& node);
  node* find (const node_pointer& key);
  bool compare_nodes (const node_pointer& node, const node_pointer& node2);

private:
  std::size_t mask = 0;
  std::size_t size = 0;
};


class hashmap final
{

public:
  /*
  main is used as the active hashtable for the server to interact with
  backup is what would be written to disk
  final design still needs to be worked through - this is more a proof of concept
  */
  hashtable backup;
  hashtable main;
  std::size_t resizing_pos = 0;

public:
  hashmap (std::size_t n);
  bool insert (hashtable& hash_table, hashtable::node_pointer node);
  void resize ();
  bool remove (hashtable& hash_table, hashtable::node_pointer& from);
  hashtable::node* find (hashtable& hash_table, const hashtable::node_pointer& key);

private:
  const std::size_t max_nodes_per_resize = 128;
  const double max_fill_ratio = 0.75;

  //std::unique_ptr <hashtable> ht1;
  //std::unique_ptr <hashtable> ht2;

  std::size_t resizing_pos = 0;

  bool initialise_hashmap (size_t n);

  
};