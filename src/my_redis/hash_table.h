#ifndef _HASH_TABLE
#define _HASH_TABLE

#include <stdlib.h>
#include <assert.h>

#include <vector>

namespace HashTable {
  struct HNode {
    uint64_t hcode = 0;
    HNode* next = nullptr;
  };

  struct HTab {
    std::vector<HNode*> tab;
    size_t mask = 0;
    size_t size = 0;
  };

  struct HMap {
    HTab ht1;
    HTab ht2;
    size_t resizing_pos = 0;
  };

  const size_t k_resizing_work = 128;

  void h_init(HTab& htab, size_t n);
  void h_insert(HTab& htab, HNode* node);
  // since the book doesn't implement doubly linked list, we resort to double pointers
  HNode** h_lookup(HTab& htab, HNode* key, bool (*cmp)(HNode*, HNode*));
  HNode* h_detach(HTab& htab, HNode** from);
  HNode* hm_lookup(HMap* hmap, HNode* key, bool (*cmp)(HNode*, HNode*));

  void hm_help_resizing(HMap* hmap);
}

#endif
