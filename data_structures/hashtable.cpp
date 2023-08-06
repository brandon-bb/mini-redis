#include "hashtable.hpp"




miniredis::hashtable::hashtable (std::size_t n)
{
  initialise_hashtable (n);
}

bool miniredis::hashtable::initialise_hashtable (std::size_t n)
{
  std::lock_guard <std::mutex> lock(mutex);
  
  if (n == 0 || ((n - 1) & n) != 0) { return false; }

  table.resize (n);
  mask = n-1;
  size = 0;
}

std::size_t miniredis::hashtable::get_size () 
{ 
  std::lock_guard <std::mutex> lock (mutex);
  return size; 
}

std::mutex& miniredis::hashtable::get_mutex ()
{
  std::lock_guard <std::mutex> lock (mutex);
  return mutex;
}

std::size_t miniredis::hashtable::get_mask () 
{
  std::lock_guard <std::mutex> lock (mutex); 
  return mask; 
}

void miniredis::hashtable::set_size (std::size_t n) 
{ 
  std::lock_guard <std::mutex> lock (mutex);
  size = n; 
}

void miniredis::hashtable::set_mask (std::size_t n) 
{ 
  std::lock_guard <std::mutex> lock (mutex);
  mask = n;
}

void miniredis::hashtable::increment_size () 
{ 
  std::lock_guard <std::mutex> lock (mutex);
  size++; 
}

void miniredis::hashtable::decrement_size () 
{
  std::lock_guard <std::mutex> lock (mutex);
  if (size > 0) { size--; } 
}

void miniredis::hashtable::clear_segment (std::size_t start, std::size_t end)
{
  for (std::size_t i = start ; i < end; ++i)
  {
    std::lock_guard <std::mutex> lock (locks[i]);
    
    if (table[i]) { table[i] = nullptr; }
  }
}

void miniredis::hashtable::clear ()
{
  std::size_t num_threads = std::thread::hardware_concurrency ();
  std::size_t segment_size = table.size () / num_threads;

  std::vector <std::thread> threads;

  for (std::size_t i = 0; i < num_threads; ++i)
  {
    threads.emplace_back ([this, i, segment_size]()
    {
      std::size_t start = i * segment_size;
      std::size_t end = (i + 1) * segment_size;
      clear_segment (start, end);
    });
  }

  for (auto& thread : threads) { thread.join(); }
}


bool miniredis::hashtable::compare_nodes (const node_pointer& node, const node_pointer& node2)
{
  return node->hashcode == node2->hashcode;
}

bool miniredis::hashtable::insert (node_pointer& node)
{
  size_t pos = node->hashcode & mask;
  std::lock_guard<std::mutex> lock(locks[pos]);

  if (pos >= size)
  {
    return false;
  }

  node->next = std::move(table[pos]);
  table[pos] = std::move(node);
  size++;

  return true;
}

bool miniredis::hashtable::remove (node_pointer& from)
{
  if (!from)
  {
    return false;
  }

  std::size_t pos = from->hashcode & mask;
  std::lock_guard<std::mutex> lock (locks[pos]);

  node* current = table[pos].get();
  node* prev = nullptr;

  while (current != nullptr)
  {
    if (compare_nodes (static_cast<node_pointer> (current), from))
    {
      if (prev == nullptr)
      {
        table[pos] = std::move (from->next);
      }

      else
      {
        prev->next = std::move (from->next);
      }

      size--;
      return true;
  }
  prev = current;
  current = std::move (current->next.get());
  }
  
  return false;
}

miniredis::hashtable::node* miniredis::hashtable::find (const node_pointer& key)
{
  if (table.empty())
  {
    size_t pos = key->hashcode & mask;
    std::lock_guard <std::mutex> lock (locks[pos]);
    node_pointer *from = &(table[pos]);

    while (*from)
    {
      if (compare_nodes (*from, key))
      {
        return from->get();
      }

      from = &((*from)->next);
    }
  }

  return nullptr;
}


/*
******hashmap class*******
*/

miniredis::hashmap::hashmap () : main (default_size)
{
  initialise_main (default_size);
}

bool miniredis::hashmap::power_of_two (std::size_t n) 
{ 
  std::lock_guard <std::mutex> lock (mutex);
  return (n == 0 || ((n - 1) & n) != 0); 
}

bool miniredis::hashmap::initialise_main (std::size_t n)
{
  std::lock_guard <std::mutex> lock (mutex);
  if (power_of_two(n))
  {
    main.table.resize (n);
    main.set_mask (n-1);
    main.set_size (0);
    return true;
  }

  return false;
}

bool miniredis::hashmap::insert (hashtable::node_pointer node)
{
  std::size_t pos = node->hashcode & main.get_mask ();
  std::lock_guard <std::mutex> lock (main.locks[pos]);
  
  if (!backup_table.get_size ())
  {
    //initialise it
  }
  main.insert (node);

  if (main.get_size ())
  {
    size_t load_factor = main.get_size () / main.get_mask () + 1;

    if (load_factor >= max_nodes_per_resize)
    {
      resize_main_table ();
    }
  }
  
  redistribute_to_backup ();
}

bool miniredis::hashmap::pop (hashtable::node_pointer& node)
{
  redistribute_to_backup ();

  std::size_t pos = node->hashcode & main.get_mask ();
  std::lock_guard <std::mutex> lock (main.locks[pos]);
  
  hashtable::node* from_main = main.find (node);

  if (from_main) { return main.remove (node);}

  std::size_t backup_pos = node->hashcode & backup_table.get_mask();
  std::lock_guard <std::mutex> lock (backup_table.locks[pos]);
  
  hashtable::node* from_backup = backup_table.find(node);

  if (from_backup)
  {
    return backup_table.remove (node);
  }

  return false; //handle error
}


miniredis::hashtable::node* miniredis::hashmap::find (const hashtable::node_pointer& node)
{
  redistribute_to_backup ();

  std::size_t pos = node->hashcode & main.get_mask ();
  std::lock_guard <std::mutex> lock (main.locks[pos]);

  hashtable::node* found_node = backup_table.find (node);

  if (!found_node)
  {
    found_node = main.find (node);
  }

  
  return found_node;
}


/*
when the main table exceeds a certain load capacity,
we redistribute the nodes in main to the backup and
clear the main table to free memory.

as the nodes are managed using unique_ptr, they are deleted
from memory automatically when we clear the table.
*/

void miniredis::hashmap::redistribute_segment (hashtable& source, hashtable& destination, std::size_t start, std::size_t end)
{
  for (std::size_t pos = start; pos < end; ++pos)
  {
    std::lock_guard <std::mutex> lock (source.locks[pos]);
    hashtable::node_pointer& from = source.table[pos];

    if (!from)
    {
      continue;
    }

    hashtable::node_pointer node = std::move (from);
    from = std::move (node->next);
    destination.insert (node);
  }
}

void miniredis::hashmap::parallel_redistribute (hashtable& source, hashtable& destination)
{
  std::size_t num_threads = std::thread::hardware_concurrency ();
  std::size_t segment_size = source.get_size () / num_threads;
  
  std::vector <std::thread> threads;

  for (std::size_t i = 0; i < num_threads; ++i)
  {
    threads.emplace_back ([this, &source, &destination, i, segment_size]()
    {
      std::size_t start = i * segment_size;
      std::size_t end = (i + 1) * segment_size;
      redistribute_segment (source, destination, start, end);
    });
  }

  for (auto& thread : threads) { thread.join(); }
}

void miniredis::hashmap::redistribute_to_backup ()
{
  if (main.table.empty())
  {
    return; //error message
  }

  parallel_redistribute (main, backup_table);

  if (main.get_size() <= main.get_mask() * 0.25)
  {
    std::size_t new_size = main.get_mask () / 2;
    adjust_size (new_size);
  }
}



void miniredis::hashmap::check_load_factor ()
{
  double load_factor = static_cast <double> (main.get_size () / main.get_mask ());

  if (load_factor >= max_fill_ratio) { resize_main_table(); }
}

void miniredis::hashmap::adjust_size (std::size_t new_size)
{
  if ( power_of_two (new_size))
  {
    std::lock_guard<std::mutex> lock(mutex);
    redistribute_to_backup();
    initialise_main (new_size);
    resize_main_table ();
  }
}


void miniredis::hashmap::resize_main_table ()
{
  if (!backup_table.table.empty())
  {
    //return error that its not empty.
  }

  size_t new_size = (main.get_mask () + 1) * 2;

  if (!power_of_two (new_size))
  {
    //return error
  }

  hashtable new_main (new_size);
  parallel_redistribute (backup_table, new_main);
  main = std::move (new_main);
  resizing_pos = 0;
}