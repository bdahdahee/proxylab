#include <stdlib.h>
#include "cache.h"
void cache_print(CacheList *list) {
  CachedItem *p;
  printf("list size: %zu, first: %p, last: %p\n",
      list->size, list->first, list->last);
  printf("**** forward ****\n"); 
  p = list->first;
  while(p) {
    printf("url: %s\n", p->url);
    printf("headers: %s\n", p->headers);
    printf("size: %zu\n", p->size);
    p = p->next;
  }
  printf("**** backward ****\n"); 
  p = list->last;
  while(p) {
    printf("url: %s\n", p->url);
    printf("headers: %s\n", p->headers);
    printf("size: %zu\n", p->size);
    p = p->prev;
  }
  return;
}
int main(int argc, char *argv[]) {
  CacheList cache;
  cache_init(&cache);
  cache_print(&cache);
  cache_URL("a", "aa", malloc(1000000), 1000000, &cache);
  cache_print(&cache);
  cache_URL("b", "bb", malloc(999999), 999999, &cache);
  cache_print(&cache);
  cache_URL("c", "cc", malloc(1000000), 1000000, &cache);
  cache_print(&cache);
  cache_destruct(&cache);
  return 0;
}
    
