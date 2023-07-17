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

HashTable::HNode** HashTable::h_lookup(
  HTab& htab,
  const HNode* key,
  bool (*cmp)(const HNode*, const HNode*)
) {
  if (htab.tab.empty()) {
    return nullptr;
  }

  size_t pos = key->hcode & htab.mask;
  HNode** from = &htab.tab[pos]; // from is a pointer to a const HNode*

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

HashTable::HNode* HashTable::hm_lookup(
  HMap& hmap,
  const HNode* key,
  bool (*cmp)(const HNode*, const HNode*)
) {
  hm_help_resizing(hmap);

  HNode** from = h_lookup(hmap.ht1, key, cmp);

  if (!from) {
    from = h_lookup(hmap.ht2, key, cmp);
  }

  return from ? *from : nullptr;
}

void HashTable::hm_help_resizing(HMap& hmap) {
  if (hmap.ht2.tab.empty()) {
    return;
  }

  size_t nwork = 0;
  while (nwork < k_resizing_work && hmap.ht2.size > 0) {
    HNode** from = &hmap.ht2.tab[hmap.resizing_pos];

    if (!*from) {
      hmap.resizing_pos++;
      continue;
    }

    h_insert(hmap.ht1, h_detach(hmap.ht2, from));
    nwork++;
  }

  if (hmap.ht2.size == 0) {
    hmap.ht2.tab.clear();
    hmap.ht2.mask = 0;
    hmap.ht2.size = 0;
  }
}

void HashTable::hm_insert(HMap& hmap, HNode* node) {
  if (hmap.ht1.tab.empty()) {
    h_init(hmap.ht1, 4);
  }

  h_insert(hmap.ht1, node);

  if (hmap.ht2.tab.empty()) {
    size_t load_factor = hmap.ht1.size / (hmap.ht1.mask + 1);

    if (load_factor >= k_max_load_factor) {
      hm_start_resizing(hmap);
    }
  }

  hm_help_resizing(hmap);
}

void HashTable::hm_start_resizing(HMap& hmap) {
  assert(hmap.ht2.tab.empty());

  hmap.ht2 = std::move(hmap.ht1);
  h_init(hmap.ht1, (hmap.ht1.mask + 1) * 2);
  hmap.resizing_pos = 0;
}

HashTable::HNode* HashTable::hm_pop(
  HMap& hmap,
  const HNode* key,
  bool (*cmp)(const HNode*, const HNode*)
) {
  hm_help_resizing(hmap);
  HNode** from = h_lookup(hmap.ht1, key, cmp);
  if (from) {
    return h_detach(hmap.ht1, from);
  }

  from = h_lookup(hmap.ht2, key, cmp);
  return from ? h_detach(hmap.ht2, from) : nullptr;
}
