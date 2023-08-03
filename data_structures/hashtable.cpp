#include "hashtable.hpp"

/*
hashtable::hashtable (std::size_t n)
{
  initialise_hashtable (n);
}


bool hashtable::initialise_hashtable (size_t n)
{
  if (n == 0 || ((n - 1) & n) != 0)
  {
    return false;
  }

  htab.table.resize (n);
  htab.mask = n - 1;
  htab.size = 0;

  return true;
}
*/
std::size_t hashtable::get_size () const
{
  return size;
}

std::size_t hashtable::get_mask () const
{
  return mask;
}

void hashtable::increment_size ()
{
  size++;
}

void hashtable::decrement_size ()
{
  if (size > 0) { size--; }
}

bool hashtable::compare_nodes (const node_pointer& node, const node_pointer& node2)
{
  return node->hashcode == node2->hashcode;
}

bool hashtable::insert (node_pointer& node)
{
  size_t pos = node->hashcode & mask;

  if (pos >= size)
  {
    return false;
  }

  node->next = std::move(table[pos]);
  table[pos] = std::move(node);
  size++;

  return true;
}

bool hashtable::remove (node_pointer& from)
{
  if (!from)
  {
    return false;
  }

  node_pointer node = std::move (from);
  from = std::move(node->next);
  size--;

  return true;
}

hashtable::node* hashtable::find (const node_pointer& key)
{
  if (table.empty())
  {
    size_t pos = key->hashcode & mask;
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

bool hashmap::insert (hashtable& hash_table, hashtable::node_pointer node)
{
  return true;
}

bool hashmap::remove (hashtable& hash_table, hashtable::node_pointer& from)
{

  return true;
}



void hashmap::resize ()
{
  if (main.table.empty())
  {
    return; //error message
  }

  size_t nodes_moved = 0;
  while (nodes_moved < max_nodes_per_resize && main.get_size() > 0)
  {
    hashtable::node_pointer& from = main.table[resizing_pos];

    if (!from)
    {
      resizing_pos++;
      continue;
    }

    hashtable::node_pointer node = std::move (from);
    from = std::move (node->next);
    insert (backup, std::move(from));
    nodes_moved++;
  }

  if (main.get_size() == 0)
  {
    main.table.clear ();
  }
}




hashtable::node* hashmap::find (hashtable& hash_table, const hashtable::node_pointer& key)
{
  return nullptr;
}