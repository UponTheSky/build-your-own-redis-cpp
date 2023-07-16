#include "hash_table.h"

void HashTable::h_init(HTab& htab, size_t n) {
  assert(n > 0 && ((n - 1) & n) == 0);

  htab.tab = std::vector<HNode*>(n);
  htab.mask = n - 1;
  htab.size = 0;
}

void HashTable::h_insert(HTab& htab, HNode* node) {
  size_t pos = node->hcode & htab.mask; // same as % htab.mask if n = 2^x
  HNode* next = htab.tab[pos];

  node->next = next;
  htab.tab[pos] = node;
  htab.size++;
}

HashTable::HNode** HashTable::h_lookup(HTab& htab, HNode* key, bool (*cmp)(HNode*, HNode*)) {
  if (htab.tab.empty()) {
    return nullptr;
  }

  size_t pos = key->hcode & htab.mask;
  HNode** from = &htab.tab[pos];

  while (*from) {
    if (cmp(*from, key)) {
      return from;
    }
    from = &((*from)->next);
  }

  return nullptr;
}

HashTable::HNode* HashTable::h_detach(HTab& htab, HNode** from) {
  HNode* node = *from;

  *from = (*from)->next;
  htab.size--;

  return node;
}

