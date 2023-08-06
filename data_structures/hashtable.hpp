#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <optional>
#include <type_traits>
#include <thread>
#include <mutex>

// hashtable node, embedded into payload
namespace miniredis
{
class hashtable final
{

public:
  struct node
  {
    std::unique_ptr<node> next = nullptr;
    uint64_t hashcode = 0;
  };

  using node_pointer = std::unique_ptr <node>;
  std::vector <node_pointer> table;
  
  hashtable (size_t n);
  hashtable (const hashtable&) = delete;
  hashtable& operator=(const hashtable&) = delete;
  ~hashtable ();


  void decrement_size ();
  void increment_size ();
  void set_size(std::size_t n);
  void set_mask(std::size_t n);
  void clear ();
  void clear_segment (std::size_t start, size_t end);
  size_t get_size() const;
  size_t get_mask() const;
  bool insert (node_pointer& node);
  bool remove (node_pointer& node);
  node* find (const node_pointer& key);
  bool compare_nodes (const node_pointer& node, const node_pointer& node2);

private:
  std::size_t mask = 0;
  std::size_t size = 0;
  std::mutex mutex;

  bool initialise_hashtable (size_t n);
};


class hashmap final
{

public:
  /*
  main is used as the active hashtable for the server to interact with
  backup is ....
  final design still needs to be worked through - this is more a proof of concept
  */

public:
  hashmap ();
  hashmap (std::size_t n);
  bool power_of_two (std::size_t n);
  bool insert (hashtable::node_pointer node);
  bool pop (hashtable::node_pointer& key);
  hashtable::node* find (const hashtable::node_pointer& key);
  void adjust_size (std::size_t new_size);
  void pop_all ();



private:
  const std::size_t default_size = 200;
  const std::size_t max_nodes_per_resize = 128;
  const double max_fill_ratio = 0.75;
  
  hashtable main;
  std::optional <hashtable> backup;
  hashtable& backup_table = backup.value ();

  //std::unique_ptr <hashtable> ht1;
  //std::unique_ptr <hashtable> ht2;

  std::size_t resizing_pos = 0;

  bool initialise_main (std::size_t n);
  bool initialise_backup (std::size_t n);
  void check_load_factor ();
  void resize_main_table ();
  void parallel_redistribute (hashtable& source, hashtable& destination);
  void redistribute_segment (hashtable& source, hashtable& destination, std::size_t start, std::size_t end);
  void redistribute_to_backup ();

  
};

}